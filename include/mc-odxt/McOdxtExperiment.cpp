#include "mc-odxt/McOdxtExperiment.hpp"
#include "mc-odxt/McOdxtTypes.hpp"

#include <iostream>

namespace mcodxt {

McOdxtExperiment::McOdxtExperiment() {}

McOdxtExperiment::~McOdxtExperiment() {}

int McOdxtExperiment::setup() {
  std::cout << "[MC-ODXT] Setting up multi-user ODXT experiment..." << std::endl;
  std::cout << "[MC-ODXT] Setup complete!" << std::endl;
  return 0;
}

void McOdxtExperiment::run() {
  std::cout << "[MC-ODXT] Running experiment..." << std::endl;
  std::cout << "[MC-ODXT] MC-ODXT implementation in progress..." << std::endl;
  std::cout << "[MC-ODXT] Experiment complete!" << std::endl;
}

void McOdxtExperiment::teardown() {
  std::cout << "[MC-ODXT] Tearing down..." << std::endl;
}

std::string McOdxtExperiment::getName() const { return "mc-odxt"; }

}  // namespace mcodxt
