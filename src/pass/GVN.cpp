//
// Created by 顾超 on 2021/7/24.
//

#include "pass/GVN.h"
#define IN(y,x) (x.find(y) != x.end())

std::string GVN::hash(Instruction *i) {
    // 简单的哈希算法，使用 OpName + LeftOpName + RightOpName作为哈希值
    auto ret = i->get_instr_op_name();
    if (i->is_alloca() || i->is_load() || i->is_call() || i->is_void()) {
        return ret + i->get_name();
    }
    ret += std::to_string(i->get_num_operand());
    if (auto cmp = dynamic_cast<CmpInst *>(i)) {
        ret += "C" + std::to_string(cmp->get_cmp_op()) + "M";
    }
    for (auto op: i->get_operands()) {
        if (auto c = dynamic_cast<ConstantInt *>(op)) {
            ret += "CO" + std::to_string(c->get_value());
        } else if (auto gv = dynamic_cast<GlobalVariable *>(op)) {
            ret += "GV" + gv->get_name();
        } else if (auto is = dynamic_cast<Instruction *>(op)) {
            ret += is->get_name();
        } else if (auto arg = dynamic_cast<Argument *>(op)){
            ret += arg->get_name();
        } else if (auto bb = dynamic_cast<BasicBlock *>(op)){
            ret += "BB" + bb->get_name();
        } else {
            assert(0 && "Unsupported");
        }
    }
    return ret;
}

// 随便写的算法，真的很慢
void GVN::globalValueNumbering(Function *f) {
    f->print(); // 初始化所有指令编号
    bool changed;
    do {
        _map.clear();
        changed = false;
        // 遍历所有BB
        std::set<Instruction *> tbd;
        for (auto bb: f->get_basic_blocks()) {
            for (auto inst: bb->get_instructions()) {
                // 如果有返回值且不是call/load则计算哈希值
                if (inst->is_call() || inst->is_store() || inst->is_load() || inst->is_br() || inst->is_ret()) {
                    continue;
                }
                auto inst_hash = hash(inst);
                // std::cout <<inst_hash <<std::endl;
                if (IN(inst_hash, _map)) {
                    bool replaced = false;
                    for (auto ii: _map[inst_hash]) {
                        // 检查是否可以进行替换
                        if (dom->isDominatedBy(inst->get_parent(), ii->get_parent())) {
                            // 后者被前者支配，直接进行替换
                            replaced = true;
                            inst->replace_all_use_with(ii);
                            tbd.insert(inst);
                            break;
                        } else if (dom->isDominatedBy(ii->get_parent(), inst->get_parent())) {
                            // 由于遍历顺序问题，反向替换
                            replaced = true;
                            ii->replace_all_use_with(inst);
                            tbd.insert(ii);
                            _map[inst_hash].erase(ii);
                            _map[inst_hash].insert(inst);
                            break;
                        }
                    }
                    if (!replaced) {
                        _map[inst_hash].insert(inst);
                    } else {
                        changed = true;
                    }
                } else {
                    // 直接加入
                    _map[inst_hash].insert(inst);
                }
            }
        }
        // 删除所有被替换的指令
        for (auto inst: tbd) {
            inst->get_parent()->delete_instr(inst);
        }
    } while (changed);
}

void GVN::run() {
    dom = new dominator(m_);
    dom->run();
    for (auto f: m_->get_functions()) {
        if (f->is_declaration()) continue;
        globalValueNumbering(f);
    }
}