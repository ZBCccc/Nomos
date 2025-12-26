#include "mc-odxt/McOdxtExperiment.hpp"

namespace mcodxt {

McOdxtExperiment::McOdxtExperiment() {}

McOdxtExperiment::~McOdxtExperiment() {}

int McOdxtExperiment::setup() {
  std::cout << "[MC-ODXT] Setting up..." << std::endl;
  return 0;
}

void McOdxtExperiment::run() {
  std::cout << "[MC-ODXT] Running experiment..." << std::endl;
  // TODO: Implement MC-ODXT protocol logic here
}

void McOdxtExperiment::teardown() {
  std::cout << "[MC-ODXT] Tearing down..." << std::endl;
}

std::string McOdxtExperiment::getName() const { return "mc-odxt"; }

}  // namespace mcodxt
