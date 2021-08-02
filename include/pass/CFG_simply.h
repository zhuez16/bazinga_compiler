//
// Created by 万嘉诚 on 2021/7/26.
//

#include "IR/BasicBlock.h"
#include "IR/Function.h"
#include "IR/IRbuilder.h"
#include "IR/Instruction.h"
#include "IR/Module.h"
#include "pass_manager.h"
#include "include/pass/CFG.h"

class CFG_simply : public Pass {
private:
  Function *func_;
  CFG_simply();
  std::vector<BasicBlock *> bb_del;

public:
  CFG_simply(Module *m) : Pass(m) {}
  ~CFG_simply(){};
  void run() override;
  void del_no_pre();
  void merge_single();
  void del_singel_phi();
  void del_uncond();
  void del_self_loop();
  void del_no_pre_(BasicBlock * bb);
  void fix_succ();
  void fix_phi();
  void fix_pre();
};