#include "nomos/GatekeeperState.hpp"

GatekeeperState::GatekeeperState() {}

GatekeeperState::~GatekeeperState() {}

bool GatekeeperState::Get(std::string& out, const std::string& keyword) {
  auto it = state_map.find(keyword);
  if (it != state_map.end()) {
    out = it->second;
    return true;
  }
  return false;
}

void GatekeeperState::Put(const std::string& in, const std::string& keyword) {
  state_map[keyword] = in;
}

void GatekeeperState::Clear() { state_map.clear(); }
