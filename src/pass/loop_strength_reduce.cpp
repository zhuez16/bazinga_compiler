//
// Created by 顾超 on 2021/7/28.
//

#include "pass/loop_strength_reduce.h"



PhiInst *LoopStrengthReduce::getInferVariable(Loop *loop, Trace &t) {
    // 获取入口块，归纳变量在入口块必定出现
    auto entry = loop->get_loop_entry();
    PhiInst *inferPhi = nullptr;
    Value *increaseStep = nullptr;
    Value *initialValue = nullptr;
    for (auto inst: entry->get_instructions()) {
        if (auto phi = dynamic_cast<PhiInst *>(inst)) {
            // We promise that phi's initial value comes from outer loop or modification to this variable
            // through all path in the loop and back to entry block is same. Just trace the modification to phi
            std::vector<Trace> tracing;
            bool success = true;
            for (auto pair: phi->getValueBBPair()) {
                // From out of loop, assume it to be a initial value
                if (!loop->contain_bb(pair.second)) {
                    if (initialValue == nullptr) {
                        initialValue = pair.first;
                    } else {
                        continue;
                    }
                } else {
                    // 保证所有循环内部反向边到达入口块对其改变是一致的。可以是固定长度的加法
                    // 反向跟踪Phi的Value直到离开循环或回到Phi指令
                    Value *v = pair.first;
                    bool valid = true;
                    Trace T;
                    while (true) {
                        auto inst = dynamic_cast<Instruction *>(v);
                        if (inst == nullptr) {
                            // 死循环，无法处理
                            valid = false;
                            break;
                        }
                        if (inst->is_load() || inst->is_call()) {
                            // 可能收到内存变化影响的情况，我们无法处理
                            valid = false;
                            break;
                        }
                        if (inst->is_add() || inst->is_sub()) {
                            // 线性变化的归纳变量
                            // 保证其中一边是常量或者来自循环外部，循环不变量外提对这个Pass有一定帮助
                            auto lhs = dynamic_cast<Instruction *>(inst->get_operand(0));
                            auto rhs = dynamic_cast<Instruction *>(inst->get_operand(1));
                            if (lhs && rhs) {
                                if (loop->contain_bb(lhs->get_parent()) && loop->contain_bb(rhs->get_parent())) {
                                    // 两边都是循环相关的，无法处理
                                    valid = false;
                                    break;
                                }
                                if (loop->contain_bb(lhs->get_parent()) && !loop->contain_bb(rhs->get_parent())) {
                                    T.var.emplace_back(rhs, inst->is_add() ? Trace::ADD : Trace::SUB);
                                    v = lhs;
                                    continue;
                                } else if (inst->is_add() && !loop->contain_bb(lhs->get_parent()) &&
                                           loop->contain_bb(rhs->get_parent())) {
                                    v = rhs;
                                    T.var.emplace_back(lhs, inst->is_add() ? Trace::ADD : Trace::SUB);
                                    continue;
                                } else {
                                    // 可能导致死循环
                                    valid = false;
                                    break;
                                }
                            }
                            auto lc = dynamic_cast<ConstantInt *>(inst->get_operand(0));
                            auto la = dynamic_cast<Argument *>(inst->get_operand(0));
                            auto rc = dynamic_cast<ConstantInt *>(inst->get_operand(1));
                            auto ra = dynamic_cast<Argument *>(inst->get_operand(1));
                            // 常量 + 迭代量模式
                            if ((lc || la) && inst->is_add() && rhs && loop->contain_bb(rhs->get_parent())) {
                                v = rhs;
                                if (lc) {
                                    T.constant += lc->get_value();
                                } else {
                                    T.var.emplace_back(la, Trace::ADD);
                                }
                            }
                            else if ((rc || ra) && lhs && loop->contain_bb(lhs->get_parent())) {
                                v = lhs;
                                if (rc) {
                                    T.constant += inst->is_add() ? rc->get_value() : -rc->get_value();
                                } else {
                                    T.var.emplace_back(la, inst->is_add() ? Trace::ADD : Trace::SUB);
                                }
                            } else {
                                valid = false;
                                break;
                            }
                        }
                        if (inst->is_phi()) {
                            if (inst == phi) {
                                // Finish
                            } else {
                                valid = false;
                                // Fail
                            }
                            break;
                        }
                    }
                    if (valid) {
                        T.entryPhi = phi;
                        tracing.push_back(T);
                    } else {
                        // Fail to infer
                        success = false;
                        break;
                    }
                }
            }
            if (!success) continue;
            // 判断是否各路径值相等
            Trace l0 = tracing[0];
            success = true;
            for (int i = 1; i < tracing.size(); ++i) {
                if (l0 == tracing[i]) continue;
                success = false;
                break;
            }
            if (!success) continue;
            t = tracing[0];
            return dynamic_cast<PhiInst *>(inst);
        } else break;
    }
    return nullptr;
}



void LoopStrengthReduce::run() {
    m_->print();
    lp->run();
    for (auto f: m_->get_functions()) {
        if (!f->is_declaration()) {
            runOnFunction(f);
        }
    }
}

void LoopStrengthReduce::runOnFunction(Function *f) {
    for (Loop *loop: lp->get_loop(f)) {
        Trace t;
        auto inst = getInferVariable(loop, t);
        for (auto use:t.entryPhi.get_use_list()){
            if (auto rtype = dynamic_cast<BinaryInst*>(use.val_)){

            }
        }
        if (inst == nullptr) continue;
        std::cout << inst->print() << std::endl;
        std::cout << t.print_trace() << std::endl;
    }
}

void LoopStrengthReduce::modifyLowerUpperBound(Loop *loop, PhiInst *inferValue, Trace &T) {
    // 如果循环的某一条路径什么事情都没做而仅修改循环变量，且保证这条路径只在循环开始或结尾会被执行，则我们可以修改循环的上下界来优化循环
    // 常量折叠与死代码删除可以使得这个Pass获取更好的效果
    // 跟踪循环决定量的值变化。保证递增值为1，否则可能优化导致错误
    if (T.constant != 1 || !T.var.empty()) return;
    // 若归纳变量Phi只有2个入口，则直接跳过。
    if (inferValue->get_num_operand() <= 4) return;
    // 跟踪Phi的值与CFG的关系
    // Entry br[cond < a1] -> br[cond > a2] -> br[no cond] -> entry => Entry[cond < min(a1, a2 + 1)] 修改上界
    std::map<Value *, BasicBlock *> mp;
    for (auto use: inferValue->get_use_list()) {
        // 获取所有基于inferValue的判断语句
        if (auto cmp = dynamic_cast<CmpInst *>(use.val_)) {

        }
    }
}

bool checkBinary(Instruction *inst, unsigned op, Loop *loop) {
    auto val = inst->get_operand(op);
    auto val_inst = dynamic_cast<Instruction *>(val);
    return (dynamic_cast<ConstantInt *>(val) || (val_inst && !loop->contain_bb(val_inst->get_parent())));
}

void LoopStrengthReduce::strengthReduce(Loop *loop, PhiInst *infer, Trace &T) {
    // 搜索循环中利用归纳变量进行计算的，除了固定增量外的变量
    for (auto u: infer->get_use_list()) {
        // 如果是 乘法 或 加/减法 且另一个变量与循环无关则可以优化
        if (auto inst = dynamic_cast<Instruction *>(u.val_)) {
            // 跳过归纳变量计算表达式
            if (T.infer_set.find(inst) != T.infer_set.end()) continue;
            // i +- arg || arg + i，对于 arg - i
            if (inst->is_add()) {
                // 获取初始量以及递增量
                // 循环初始，i + %inc，递增也是 %inc TODO
                continue;
                unsigned op_no = 1 - u.arg_no_;
                if (checkBinary(inst, op_no, loop)) {
                    BinaryInst::create_add(inst->get_operand(op_no), infer, nullptr, m_);
                }
            } else if (inst->is_sub()) {
                // TODO
            } else if (inst->is_mul()) {
                // 乘法
                // BinaryInst::create_add()
            }

        }
    }
}
