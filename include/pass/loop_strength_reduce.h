//
// Created by 顾超 on 2021/7/27.
//

#ifndef BAZINGA_COMPILER_LOOP_STRENGTH_REDUCE_H
#define BAZINGA_COMPILER_LOOP_STRENGTH_REDUCE_H

#include "pass_manager.h"
#include "pass/loop_search.h"

struct Trace {
    std::vector<Instruction *> queue;
    Instruction *entryPhi;

    bool operator== (const Trace &rhs) const {
        if (rhs.queue.size() != queue.size()) return false;
        auto lpre = entryPhi;
        auto rpre = rhs.entryPhi;
        bool eq = true;
        for (int i = queue.size() - 1; i >= 0; --i) {
            auto l_inst = queue[i];
            auto r_inst = rhs.queue[i];
            if (l_inst == r_inst) continue;
            if (l_inst->get_instr_type() == r_inst->get_instr_type()) {
                if (l_inst->is_add()) {
                    auto lpro = l_inst->get_operand(0);
                    auto rpro = r_inst->get_operand(0);
                    if (lpro == lpre) lpro = l_inst->get_operand(1);
                    if (rpro == rpre) rpro = l_inst->get_operand(1);
                    if (lpro == rpro) {
                        lpre = l_inst;
                        rpre = r_inst;
                        continue;
                    }
                    auto c1 = dynamic_cast<ConstantInt *>(lpro);
                    auto c2 = dynamic_cast<ConstantInt *>(rpro);
                    if (c1 && c2 && c1->get_value() == c2->get_value()) {
                        lpre = l_inst;
                        rpre = r_inst;
                        continue;
                    }
                    eq = false;
                    break;
                } else {
                    auto lpro = l_inst->get_operand(1);
                    auto rpro = r_inst->get_operand(1);
                    if (lpro == rpro) {
                        lpre = l_inst;
                        rpre = r_inst;
                        continue;
                    }
                    auto c1 = dynamic_cast<ConstantInt *>(lpro);
                    auto c2 = dynamic_cast<ConstantInt *>(rpro);
                    if (c1 && c2 && c1->get_value() == c2->get_value()) {
                        lpre = l_inst;
                        rpre = r_inst;
                        continue;
                    }
                    eq = false;
                    break;
                }
            }
        }
        return eq;
    }

    std::string print_trace() {
        std::string ret = "Phi = Phi ";
        Instruction *preInst = entryPhi;
        for (int i = queue.size() - 1; i >= 0; --i) {
            Instruction *inst = queue[i];
            if (inst->is_add()) {
                auto op = inst->get_operand(0);
                if (op == preInst) {
                    op = inst->get_operand(1);
                }
                if (auto c = dynamic_cast<ConstantInt *>(op)) {
                    ret += "+ Constant[" + std::to_string(c->get_value()) + "] ";
                } else {
                    ret += "+ %" + op->get_name();
                }
            } else {
                auto op = inst->get_operand(1);
                if (auto c = dynamic_cast<ConstantInt *>(op)) {
                    ret += "- Constant[" + std::to_string(c->get_value()) + "] ";
                } else {
                    ret += "- %" + op->get_name();
                }
            }
        }
        return ret;
    }
};

/**
 * This pass focus on optimizing the testcase "transpose.sy"
 * This pass transform the form of
 * @code
 *      while (j < rowsize){
 *          if (i < j){
 *              j = j + 1;
 *              continue;
 *          }
 *          int curr = matrix[i * rowsize + j];
 *          matrix[j * colsize + i] = matrix[i * rowsize + j];
 *          matrix[i * rowsize + j] = curr;
 *          j = j + 1;
 *      }
 * @endcode
 * identifying the upperbound and change the * op into + op
 */
class LoopStrengthReduce : public Pass {
public:
    explicit LoopStrengthReduce(Module *m) : Pass(m), lp(new LoopSearch(m)) {}

    void run() final;

private:
    void runOnFunction(Function *f);

    /**
     * 获取循环中的归纳变量
     * TODO: 可能有多个，但是此处不进行处理，只针对找到的第一个
     * @param loop 搜索的循环
     * @return 找到返回指针，否则返回nullptr
     */
    static Instruction *getInferVariable(Loop *loop, Trace &t);

    /**
     * 修改循环上界
     * @param loop 循环
     * @param inferValue 归纳变量
     */
    void modifyLowerUpperBound(Loop *loop, Instruction *inferValue);

    LoopSearch *lp{};

};

#endif //BAZINGA_COMPILER_LOOP_STRENGTH_REDUCE_H
