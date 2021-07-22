//
// Created by 顾超 on 2021/7/22.
//

#ifndef BAZINGA_COMPILER_COMBINING_H
#define BAZINGA_COMPILER_COMBINING_H

#include <map>
#include <queue>
#include "pass/pass.h"
#include "IR/Instruction.h"

/**
 * 常量传播 lattice
 */
class ValueLattice {
public:
    enum ValueConstantTy {
        TopTy,
        DownTy,
        ConstantTy,
        ZeroTy
    };
private:
    ValueConstantTy _ty;
    int _value{};

    explicit ValueLattice(ValueConstantTy ty, int val = 0) : _ty((ty == ConstantTy && val == 0) ? ZeroTy : ty),
                                                             _value(val) {}

public:
    /**
     * 初始化时，将所有变量置为 T 类型
     */
    ValueLattice() {
        _ty = TopTy;
    }

    explicit ValueLattice(int val) : _ty(val == 0 ? ZeroTy : ConstantTy), _value(val) {}

    void setValue(int val) { _value = val; }

    int getValue() const {
        assert(_ty == ConstantTy || _ty == ZeroTy && "Only constant or zero type can use getValue method");
        return _value;
    }

    inline ValueConstantTy getType() const { return _ty; }

    bool isTopType() const { return getType() == TopTy; }

    bool isDownType() const { return getType() == DownTy; }

    bool isConstantType() const { return getType() == ConstantTy; }

    bool isZeroType() const { return getType() == ZeroTy; }

    bool isConstantOrZeroType() const { return getType() == ConstantTy || getType() == ZeroTy; }

    /**
     * Implement for phi node calculation
     * In the lattice calculation this means $$\cap L_c \times L_c \to L_c$$
     * @param lhs
     * @param rhs
     * @return
     */
    static ValueLattice meet(const ValueLattice &lhs, const ValueLattice &rhs) {
        if (lhs.isTopType() && rhs.isTopType()) {
            return ValueLattice(TopTy);
        }
        if (lhs.isDownType() || rhs.isDownType()) {
            return ValueLattice(DownTy);
        }
        if (lhs.isTopType()) {
            return ValueLattice(ConstantTy, rhs.getValue());
        }
        if (rhs.isTopType()) {
            return ValueLattice(ConstantTy, lhs.getValue());
        } else {
            if (lhs.getValue() == rhs.getValue()) {
                return ValueLattice(ConstantTy, lhs.getValue());
            } else {
                return ValueLattice(DownTy);
            }
        }
    }

    /**
     * Implement for binary instruction calculation
     * @param lhs
     * @param rhs
     * @param fp: the function to calculate $c_0 op c_1$
     * Note that in this implement we need to promise this function don't throw a exception such as Overflow / ZeroDivision
     * Just return zero when an exception happened as the inputs are promised to have no Undefined Behaviour
     * @param considerZero: Set true ti consider zero in calculation. Such as the $\times$ operand
     * @return
     */
    static ValueLattice operand(const ValueLattice &lhs, const ValueLattice &rhs,
                                int (*fp)(int lhs, int rhs),
                                bool considerZero) {
        if (lhs.isTopType() || rhs.isTopType()) {
            return ValueLattice(TopTy);
        }
        if (considerZero) {
            if (lhs.isZeroType() || rhs.isZeroType()) {
                return ValueLattice(ZeroTy, 0);
            }
            if (lhs.isDownType() || rhs.isDownType()) {
                return ValueLattice(DownTy);
            }
            return ValueLattice(ConstantTy, fp(lhs.getValue(), rhs.getValue()));
        } else {
            if (lhs.isDownType() || rhs.isDownType()) {
                return ValueLattice(DownTy);
            }
            return ValueLattice(ConstantTy, fp(lhs.getValue(), rhs.getValue()));
        }
    }

    friend bool operator== (const ValueLattice &lhs, const ValueLattice &rhs) {
        if (lhs.getType() != rhs.getType()) return false;
        if (lhs.isConstantOrZeroType()) {
            return lhs.getValue() == rhs.getValue();
        }
        return true;
    }

    friend bool operator!= (const ValueLattice &lhs, const ValueLattice &rhs) {
        return operator==(lhs, rhs);
    }
};


/**
 * 死代码删除 lattice
 */
class InstLattice {
public:
    enum InstReachableType {
        UnreachableTy,
        ReachableTy
    };

private:
    InstReachableType _ty;
    Instruction *_inst;
};


class ConstFoldingDCEliminating : public Pass {
private:
    std::map<Value *, ValueLattice> _map;
    std::queue<Instruction *> _worklist;
    bool existInMap(Value *v) { return _map.find(v) != _map.end(); }
    /**
     * 由值获取Lattice\n
     * 注意：不会检查值是否在map中存在，若不存在返回不可预测
     * @param v
     * @return
     */
    ValueLattice getLatticeByValue(Value *v) { return _map[v]; }
    void setLattice(Value *v, const ValueLattice &l) { _map[v] = l; }
    void pushWorkList(Instruction *v) { _worklist.push(v); }
    Instruction *popWorkList() {
        if(_worklist.empty()) {
            return nullptr;
        }
        else {
            Instruction *ret = _worklist.front();
            _worklist.pop();
            return ret;
        }
    }
public:
    explicit ConstFoldingDCEliminating(Module *m) : Pass(m) {}
    void run() final;
};

#endif //BAZINGA_COMPILER_COMBINING_H
