#pragma once
#include <memory>
#include <string>
#include <unordered_map>

#include "GatekeeperState.hpp"

extern "C" {
#include <relic/relic.h>
};
class GateKeeper {
public:
    GateKeeper();
    ~GateKeeper();

    int Setup();

private:
    bn_t Ks, Ky, Km;
    std::unordered_map<std::string, std::string> UpdateCnt, TSet;
    std::unique_ptr<GatekeeperState> state;
};
