#include <iostream>
#include <string>

#include <gmp.h>

extern "C" {
#include <relic/relic.h>
}

#include "core/ExperimentFactory.hpp"
#include "mc-odxt/McOdxtExperiment.hpp"
#include "nomos/NomosExperiment.hpp"
#include "nomos/NomosSimplifiedExperiment.hpp"
#include "verifiable/VerifiableExperiment.hpp"

void registerExperiments() {
  auto& factory = core::ExperimentFactory::instance();
  factory.registerExperiment(
      "nomos", []() { return std::unique_ptr<nomos::NomosExperiment>(new nomos::NomosExperiment()); });
  factory.registerExperiment(
      "nomos-simplified", []() { return std::unique_ptr<nomos::NomosSimplifiedExperiment>(new nomos::NomosSimplifiedExperiment()); });
  factory.registerExperiment(
      "mc-odxt", []() { return std::unique_ptr<mcodxt::McOdxtExperiment>(new mcodxt::McOdxtExperiment()); });
  factory.registerExperiment(
      "verifiable", []() { return std::unique_ptr<verifiable::VerifiableExperiment>(new verifiable::VerifiableExperiment()); });
}

int main(int argc, char* argv[]) {
  if (core_init() != 0) {
    core_clean();
    return 1;
  }
  if (pc_param_set_any() != 0) {
    core_clean();
    return 1;
  }

  registerExperiments();

  std::string experimentName = "nomos";  // Default
  if (argc > 1) {
    experimentName = argv[1];
  }

  try {
    auto experiment =
        core::ExperimentFactory::instance().createExperiment(experimentName);
    std::cout << "Starting experiment: " << experiment->getName() << std::endl;

    if (experiment->setup() != 0) {
      std::cerr << "Setup failed." << std::endl;
      return 1;
    }

    experiment->run();
    experiment->teardown();
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}