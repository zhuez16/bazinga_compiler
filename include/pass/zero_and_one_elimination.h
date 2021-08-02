#ifndef BAZINGA_ZERO_ONE_ELIMINATE_H
#define BAZINGA_ZERO_ONE_ELIMINATE_H

#include "pass_manager.h"

class ZERO_ONE_Eliminate : public Pass {
public:
    explicit ZERO_ONE_Eliminate(Module *m) : Pass(m) {}

    void run() final;

private:
};

#endif