#ifndef SYSYC_PASS_H
#define SYSYC_PASS_H



#include "IR/Module.h"
#include <vector>
#include <memory>
class Pass{
public:
    explicit Pass(Module* m) : m_(m) {}
    virtual void run()=0;
protected:
    Module* m_;
};

class PassManager{
public:
    explicit PassManager(Module* m) : m_(m){}
    template<typename PassType> void add_pass(bool print_ir=false){
        passes_.push_back(std::pair<Pass*,bool>(new PassType(m_),print_ir));
    }
    void run(){
        for(auto pass : passes_){
            pass.first->run();
            if(pass.second){
                std::cout<<m_->print();
            }
        }
    }
private:
    std::vector<std::pair<Pass*,bool>> passes_;
    Module* m_;
};

#endif