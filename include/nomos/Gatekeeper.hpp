#pragma once
#include <memory>
#include <string>
#include <unordered_map>

#include "GatekeeperState.hpp"
#include "types.hpp"

#include <gmp.h>

extern "C" {
#include <relic/relic.h>
};
namespace nomos {
enum OP {
    OP_ADD = 0,
    OP_DEL = 1
};

class GateKeeper {
public:
    GateKeeper();
    ~GateKeeper();

    int Setup();
    Metadata Update(OP op, const std::string& id, const std::string& keyword);

private:
    bn_t Ks, Ky, Km;
    std::unordered_map<std::string, std::string> UpdateCnt;
    std::unique_ptr<GatekeeperState> state;
};
}  // namespace nomos
