//
// Created by 顾超 on 2021/7/24.
//

#ifndef BAZINGA_COMPILER_GVN_H
#define BAZINGA_COMPILER_GVN_H

#include <map>

#include "dominator.h"
#include "CFG.h"
#include "MemAnalysis.h"
#include "pass_manager.h"

class GVN: public Pass {
private:
    std::map<std::string, std::set<Instruction *>> _map;
    std::string hash(Instruction *i);
    void globalValueNumbering(Function *f);

    dominator *dom;
public:
    GVN(Module *m) : Pass(m) {}

    void run() final;
};

#endif //BAZINGA_COMPILER_GVN_H
