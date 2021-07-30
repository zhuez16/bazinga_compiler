//
// Created by 顾超 on 2021/7/30.
//
#include "ASMIR/ASValue.hpp"

const std::string pre_spacing = "  ";

std::string ASBranchInst::print() {
    auto lbl = getLabel();
    if (lbl == nullptr) {
        return pre_spacing + "bx lr";
    }

}