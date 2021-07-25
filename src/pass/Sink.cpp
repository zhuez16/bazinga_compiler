//
// Created by 顾超 on 2021/7/24.
//

#include "pass/Sink.h"

void CodeSinking::run() {
    dom->run();
    lp->run();
    for (auto f: m_->get_functions()) {
        if (!f->is_declaration())
            iterSinkInstruction(f);
    }
}

bool CodeSinking::iterSinkInstruction(Function *f) {
    cfg->runOnFunction(f);
    bool changed = false, ever_changed = false;
    do {
        changed = false;
        for (BasicBlock *bb: f->get_basic_blocks()) {
            changed |= processBB(bb);
        }
        ever_changed |= changed;
    } while (changed);
    return ever_changed;
}

bool CodeSinking::processBB(BasicBlock *bb) {
    // Only an BB with more than 1 successors
    if (cfg->getSuccBB(bb).size() <= 1) return false;
    // We promise that a bb is reachable because we eliminate dead blocks using CFG
    bool changed = false;
    bool begin = false;
    auto &inst = bb->get_instructions();
    auto it = inst.end();
    // Skip terminator
    --it;
    do {
        begin = it == inst.begin();
        if (!begin) {
            --it;
        }
        changed |= SinkInstruction(*it);
        // Since my impl, moving an instruction out of this bb will cause the iterator point to a unknown point, just exit here
    } while (!begin && !changed);
    return changed;
}


static bool isSafeToMove(Instruction *inst) {
    // Skip instructions that may have side-effects
    if (inst->is_store() || inst->is_call() || inst->is_load()) {
        return false;
    }

    // TODO: load of const value/array may be moved
    /*
    if (auto *L = dynamic_cast<LoadInst *>(Inst)) {
        MemoryLocation Loc = MemoryLocation::get(L);
        for (Instruction *S : Stores)
            if (isModSet(AA.getModRefInfo(S, Loc)))
                return false;
    }
    */

    if (inst->isTerminator() || inst->is_phi())
        return false;
    return true;
}

/// IsAcceptableTarget - Return true if it is possible to sink the instruction
/// in the specified basic block.
bool CodeSinking::IsAcceptableTarget(Instruction *Inst, BasicBlock *SuccToSinkTo) {
    assert(Inst && "Instruction to be sunk is null");
    assert(SuccToSinkTo && "Candidate sink target is null");

    // If the block has multiple predecessors, this would introduce computation
    // on different code paths.  We could split the critical edge, but for now we
    // just punt.
    // FIXME: Split critical edges if not backedges.
    if (cfg->getPrevBB(SuccToSinkTo).size() != 1 || *cfg->getPrevBB(SuccToSinkTo).begin() != Inst->get_parent()) {
        // We cannot sink a load across a critical edge - there may be stores in
        // other code paths.
        if (Inst->is_load() || Inst->is_call())
            return false;

        // We don't want to sink across a critical edge if we don't dominate the
        // successor. We could be introducing calculations to new code paths.
        if (!dom->isDominatedBy(SuccToSinkTo, Inst->get_parent()))
            return false;

        // Don't sink instructions into a loop.
        Loop *succ = lp->get_smallest_loop(SuccToSinkTo);
        Loop *cur = lp->get_smallest_loop(Inst->get_parent());
        if (succ != nullptr && succ != cur)
            return false;
    }

    return true;
}

/// SinkInstruction - Determine whether it is safe to sink the specified machine
/// instruction out of its current block into a successor.
bool CodeSinking::SinkInstruction(Instruction *Inst) {

    // Don't sink static alloca instructions.  CodeGen assumes allocas outside the
    // entry block are dynamically sized stack objects.
    if (Inst->is_alloca()) return false;

    // Check if it's safe to move the instruction.
    if (!isSafeToMove(Inst))
        return false;

    // FIXME: This should include support for sinking instructions within the
    // block they are currently in to shorten the live ranges.  We often get
    // instructions sunk into the top of a large block, but it would be better to
    // also sink them down before their first use in the block.  This xform has to
    // be careful not to *increase* register pressure though, e.g. sinking
    // "x = y + z" down if it kills y and z would increase the live ranges of y
    // and z and only shrink the live range of x.

    // SuccToSinkTo - This is the successor to sink this instruction to, once we
    // decide.
    BasicBlock *sink_target = nullptr;

    // Find the nearest common dominator of all users as the candidate.
    BasicBlock *bb = Inst->get_parent();
    for (Use &U : Inst->get_use_list()) {
        auto *UseInst = dynamic_cast<Instruction *>(U.val_);
        BasicBlock *use_ = UseInst->get_parent();
        if (auto PN = dynamic_cast<PhiInst *>(UseInst)) {
            // PHI nodes use the operand in the predecessor block, not the block with
            // the PHI.
            use_ = dynamic_cast<BasicBlock *>(PN->get_operand(((int)(U.arg_no_ / 2)) * 2 + 1));
        }
        if (sink_target)
            sink_target = dom->intersect(sink_target, use_); // DT.findNearestCommonDominator(SuccToSinkTo, UseBlock);
        else
            sink_target = use_;
        // The current basic block needs to dominate the candidate.
        if (!dom->isDominatedBy(sink_target, bb))
            return false;
    }

    if (sink_target) {
        // The nearest common dominator may be in a parent loop of BB, which may not
        // be beneficial. Find an ancestor.
        while (sink_target != bb && !IsAcceptableTarget(Inst, sink_target))
            sink_target = dom->getImmediateDominance(sink_target);// DT.getNode(SuccToSinkTo)->getIDom()->getBlock();
        if (sink_target == bb)
            sink_target = nullptr;
    }

    // If we couldn't find a block to sink to, ignore this instruction.
    if (!sink_target)
        return false;
    // Move the instruction.
    Inst->get_parent()->delete_instr_simple(Inst);
    sink_target->add_instr_after_phi(Inst);
    // Inst->moveBefore(&*SuccToSinkTo->getFirstInsertionPt());
    return true;
}
