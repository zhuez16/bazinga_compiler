//
// Created by 顾超 on 2021/7/27.
//

#ifndef BAZINGA_COMPILER_LOOP_STRENGTH_REDUCE_H
#define BAZINGA_COMPILER_LOOP_STRENGTH_REDUCE_H

#include "pass_manager.h"
#include "pass/loop_search.h"


/**
 * 记录了每一步循环归纳量的变化
 */
struct Trace {
    enum OpType {
        ADD,
        SUB
    };
    std::vector<std::pair<Value *, OpType>> var;
    std::set<Instruction *> infer_set;              // 记录了计算InferValue的语句，这些语句不会被重写
    int constant = 0;
    Instruction *entryPhi = nullptr;

    bool operator== (const Trace &rhs) const {
        if (constant != rhs.constant) return false;
        return var == rhs.var;
    }

    std::string print_trace() {
        std::string ret = "Phi = Phi ";
        for (auto item: var) {
            if (item.second == ADD) {
                ret += "+ ";
            } else {
                ret += "- ";
            }
            ret += item.first->get_name() + " ";
        }
        ret += std::to_string(constant);
        return ret;
    }
};

struct CondCFGNode {
    enum Condition {
        EQ,
        NE,
        LT,
        LE,
        GE,
        GT
    };
    Condition cond;
    Value *rhs;
    CondCFGNode *on_true;
    CondCFGNode *on_false;
};

/**
 * 记录归纳量
 * i + m => initial += m, step = step, bound += m
 * i - m => initial -= m, step = step, bound -= m
 * m - i => initial = m - initial, step = -step, bound = m - bound
 * m * i => initial *= initial, step = m * step, bound = m * bound
 */
struct Infer {
    Value * increase_step;
    Value * initial_value;
    Value * upper_bound;
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
    static PhiInst *getInferVariable(Loop *loop, Trace &t);

    /**
     * 修改循环上界
     * @param loop 循环
     * @param inferValue 归纳变量
     */
    void modifyLowerUpperBound(Loop *loop, PhiInst *inferValue, Trace &T);

    void strengthReduce(Loop *loop, PhiInst *infer, Trace &T);

    LoopSearch *lp{};

};

#endif //BAZINGA_COMPILER_LOOP_STRENGTH_REDUCE_H
