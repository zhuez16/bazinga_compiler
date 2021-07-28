//
// Created by 顾超 on 2021/7/28.
//

#include "pass/loop_strength_reduce.h"



Instruction *LoopStrengthReduce::getInferVariable(Loop *loop, Trace &t) {
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
                        T.queue.push_back(inst);
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
                                    v = lhs;
                                    continue;
                                } else if (inst->is_add() && !loop->contain_bb(lhs->get_parent()) &&
                                           loop->contain_bb(rhs->get_parent())) {
                                    v = rhs;
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
                            }
                            else if ((rc || ra) && lhs && loop->contain_bb(lhs->get_parent())) {
                                v = lhs;
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
                        T.queue.pop_back();
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
            return inst;
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
        std::cout << inst->print() << std::endl;
        std::cout << t.print_trace() << std::endl;
    }
}

void LoopStrengthReduce::modifyLowerUpperBound(Loop *loop, Instruction *inferValue) {
    // 如果循环的某一条路径什么事情都没做而仅修改循环变量，且保证这条路径只在循环开始或结尾会被执行，则我们可以修改循环的上下界来优化循环

}
