//
// Created by 万嘉诚 on 2021/7/26.
//

#include "include/IR/BasicBlock.h"
#include "include/IR/Function.h"
#include "include/IR/IRbuilder.h"
#include "include/IR/Instruction.h"
#include "include/IR/Module.h"
#include "pass_manager.h"

class CFG_simply : public Pass {
private:
  Function *func_;
  CFG_simply();

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
};