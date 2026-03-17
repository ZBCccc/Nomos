#include <gmp.h>

#include <iostream>
#include <string>
#include <vector>

extern "C" {
#include <relic/relic.h>
}

#include "benchmark/BenchmarkExperiment.hpp"
#include "benchmark/ClientSearchFixedW1Experiment.hpp"
#include "benchmark/ClientSearchFixedW2Experiment.hpp"
#include "benchmark/DatasetLoader.hpp"
#include "core/ExperimentFactory.hpp"
#include "mc-odxt/McOdxtExperiment.hpp"
#include "nomos/NomosSimplifiedExperiment.hpp"
#include "vq-nomos/VQNomosExperiment.hpp"

void registerExperiments() {
  auto& factory = core::ExperimentFactory::instance();
  factory.registerExperiment("nomos-simplified", []() {
    return std::unique_ptr<nomos::NomosSimplifiedExperiment>(
        new nomos::NomosSimplifiedExperiment());
  });
  factory.registerExperiment("mc-odxt", []() {
    return std::unique_ptr<mcodxt::McOdxtExperiment>(
        new mcodxt::McOdxtExperiment());
  });
  factory.registerExperiment("verifiable", []() {
    return std::unique_ptr<vqnomos::VQNomosExperiment>(
        new vqnomos::VQNomosExperiment());
  });
  factory.registerExperiment("vq-nomos", []() {
    return std::unique_ptr<vqnomos::VQNomosExperiment>(
        new vqnomos::VQNomosExperiment());
  });
  factory.registerExperiment("benchmark", []() {
    return std::unique_ptr<nomos::benchmark::BenchmarkExperiment>(
        new nomos::benchmark::BenchmarkExperiment());
  });
  factory.registerExperiment("chapter4-client-search-fixed-w1", []() {
    return std::unique_ptr<nomos::benchmark::ClientSearchFixedW1Experiment>(
        new nomos::benchmark::ClientSearchFixedW1Experiment());
  });
  factory.registerExperiment("chapter4-client-search-fixed-w2", []() {
    return std::unique_ptr<nomos::benchmark::ClientSearchFixedW2Experiment>(
        new nomos::benchmark::ClientSearchFixedW2Experiment());
  });
}

void configureClientSearchFixedW1(
    nomos::benchmark::ClientSearchFixedW1Experiment* exp,
    const std::vector<std::string>& args) {
  std::string dataset_name = "all";
  std::string output_dir = "results/ch4/";
  std::string scheme = "all";

  for (size_t i = 0; i < args.size(); ++i) {
    if (args[i] == "--dataset" && i + 1 < args.size()) {
      dataset_name = args[++i];
    } else if (args[i] == "--output-dir" && i + 1 < args.size()) {
      output_dir = args[++i];
    } else if (args[i] == "--scheme" && i + 1 < args.size()) {
      scheme = args[++i];
    }
  }

  if (dataset_name == "all") {
    exp->setRunAllDatasets(true);
  } else {
    exp->setRunAllDatasets(false);
    exp->setDataset(nomos::benchmark::stringToDataset(dataset_name));
  }
  exp->setOutputDir(output_dir);
  exp->setSchemeFilter(scheme);

  std::cout << "Configuration:" << std::endl;
  std::cout << "  --dataset: " << dataset_name << std::endl;
  std::cout << "  --output-dir: " << output_dir << std::endl;
  std::cout << "  --scheme: " << scheme << std::endl;
}

void configureClientSearchFixedW2(
    nomos::benchmark::ClientSearchFixedW2Experiment* exp,
    const std::vector<std::string>& args) {
  std::string dataset_name = "all";
  size_t max_points = 0;
  std::string output_dir = "results/ch4/";
  std::string scheme = "all";

  for (size_t i = 0; i < args.size(); ++i) {
    if (args[i] == "--dataset" && i + 1 < args.size()) {
      dataset_name = args[++i];
    } else if (args[i] == "--max-points" && i + 1 < args.size()) {
      max_points = std::stoul(args[++i]);
    } else if (args[i] == "--output-dir" && i + 1 < args.size()) {
      output_dir = args[++i];
    } else if (args[i] == "--scheme" && i + 1 < args.size()) {
      scheme = args[++i];
    }
  }

  if (dataset_name == "all") {
    exp->setRunAllDatasets(true);
  } else if (dataset_name == "random" || dataset_name == "none") {
    exp->setRunAllDatasets(false);
    exp->setDataset(nomos::benchmark::DatasetLoader::Dataset::None);
  } else {
    exp->setRunAllDatasets(false);
    exp->setDataset(nomos::benchmark::stringToDataset(dataset_name));
  }
  exp->setMaxPoints(max_points);
  exp->setOutputDir(output_dir);
  exp->setSchemeFilter(scheme);

  std::cout << "Configuration:" << std::endl;
  std::cout << "  --dataset: " << dataset_name << std::endl;
  std::cout << "  --max-points: " << max_points << std::endl;
  std::cout << "  --output-dir: " << output_dir << std::endl;
  std::cout << "  --scheme: " << scheme << std::endl;
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

  std::string experimentName = "nomos-simplified";  // Default
  std::vector<std::string> args;

  // Parse command line arguments
  if (argc > 1) {
    experimentName = argv[1];
    // Collect remaining arguments
    for (int i = 2; i < argc; ++i) {
      args.push_back(argv[i]);
    }
  }

  try {
    auto experiment =
        core::ExperimentFactory::instance().createExperiment(experimentName);
    std::cout << "Starting experiment: " << experiment->getName() << std::endl;

    if (experimentName == "chapter4-client-search-fixed-w1") {
      auto* ch4_exp =
          dynamic_cast<nomos::benchmark::ClientSearchFixedW1Experiment*>(
              experiment.get());
      if (ch4_exp) {
        configureClientSearchFixedW1(ch4_exp, args);
      }
    } else if (experimentName == "chapter4-client-search-fixed-w2") {
      auto* ch4_exp =
          dynamic_cast<nomos::benchmark::ClientSearchFixedW2Experiment*>(
              experiment.get());
      if (ch4_exp) {
        configureClientSearchFixedW2(ch4_exp, args);
      }
    }

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
