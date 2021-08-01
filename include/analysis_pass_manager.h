//
// Created by 顾超 on 2021/7/26.
//

#ifndef BAZINGA_COMPILER_ANALYSIS_PASS_MANAGER_H
#define BAZINGA_COMPILER_ANALYSIS_PASS_MANAGER_H

#include <map>
#include <pass_manager.h>

class AAPass : Pass {
private:
    unsigned _pass_id;
public:
    AAPass(unsigned _id, Module *m) : Pass(m), _pass_id(_id) {}

    unsigned getPassID() { return _pass_id; }
};

class APManager {
private:
    static APManager *instance;//TODO
    std::map<int, Pass *> _map;
    std::map<int, bool> _valid;

    Module *_m;

    APManager(Module *m) : _m(m) {}

public:
    static APManager *getAPManager(Module *m) {
        if (instance == nullptr) {
            instance = new APManager(m);
        }
        return instance;
    }

    template<class AAPass> void requireAnalysisPass() {
        unsigned _id = AAPass::getPassID();
        if (_map.find(_id) == _map.end()) {
            _map[_id] = new AAPass(_m);
            _valid[_id] = false;
        }
    }

    template<class AAPass> AAPass *getAnalysisPass() {
        unsigned _id = AAPass::getPassID();
        assert(_map.find(_id) != _map.end() && "Pass haven't register to APM.");
        if (_valid[_id]) {
            _valid[_id] = false;
            return dynamic_cast<AAPass *>(_map[_id]);
        }
        else {
            AAPass * p = dynamic_cast<AAPass *>(_map[_id]);
            p->run();
            return p;
        }
    }

    template<class AAPass> void markAsUnchanged() {
        unsigned _id = AAPass::getPassID();
        assert(_map.find(_id) != _map.end() && "Pass haven't register to APM.");
        _valid[_id] = true;
    }
};

#endif //BAZINGA_COMPILER_ANALYSIS_PASS_MANAGER_H
