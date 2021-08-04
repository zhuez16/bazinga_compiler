//
// Created by 万嘉诚 on 2021/7/26.
//

#include "IR/BasicBlock.h"
#include "IR/Function.h"
#include "IR/IRbuilder.h"
#include "IR/Instruction.h"
#include "IR/Module.h"
#include "pass_manager.h"
#include "pass/CFG.h"

class CFG_simply : public Pass {
private:
    Function *func_{};
    std::vector<BasicBlock *> bb_del;
    bool changed{};
    CFG *_cfg = nullptr;

    void del_no_pre();

    void merge_single();

    void del_singel_phi();

    void del_uncond();

    void del_self_loop();

    void del_no_pre_(BasicBlock *bb);

    void rebuildCFG();

    void fix_phi();

public:
    explicit CFG_simply(Module *m) : Pass(m), _cfg(new CFG(m)) {}

    ~CFG_simply() { delete _cfg; };

    void run() override;

};