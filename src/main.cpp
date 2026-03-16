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
#include "benchmark/ComparativeBenchmarkExperiment.hpp"
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
  factory.registerExperiment("comparative-benchmark", []() {
    return std::unique_ptr<nomos::benchmark::ComparativeBenchmarkExperiment>(
        new nomos::benchmark::ComparativeBenchmarkExperiment());
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

// Helper function to configure comparative benchmark from command line
void configureComparativeBenchmark(
    nomos::benchmark::ComparativeBenchmarkExperiment* exp,
    const std::vector<std::string>& args) {
  // Default values
  exp->setDataset(nomos::benchmark::DatasetLoader::Dataset::None);
  std::string dataset_name = "None";
  size_t num_files = 1000;
  size_t num_keywords = 100;
  size_t num_updates = 100;
  size_t num_searches = 10;
  bool scalability = false;

  // Parse arguments
  for (size_t i = 0; i < args.size(); ++i) {
    if (args[i] == "--dataset" && i + 1 < args.size()) {
      dataset_name = args[++i];
      exp->setDataset(nomos::benchmark::stringToDataset(dataset_name));
    } else if (args[i] == "--N" && i + 1 < args.size()) {
      num_files = std::stoul(args[++i]);
      exp->setNumFiles(num_files);
    } else if (args[i] == "--keywords" && i + 1 < args.size()) {
      num_keywords = std::stoul(args[++i]);
      exp->setNumKeywords(num_keywords);
    } else if (args[i] == "--updates" && i + 1 < args.size()) {
      num_updates = std::stoul(args[++i]);
      exp->setNumUpdates(num_updates);
    } else if (args[i] == "--searches" && i + 1 < args.size()) {
      num_searches = std::stoul(args[++i]);
      exp->setNumSearches(num_searches);
    } else if (args[i] == "--scalability") {
      scalability = true;
      exp->setScalabilityTest(true);
    }
  }

  std::cout << "Configuration:" << std::endl;
  std::cout << "  --dataset: " << dataset_name << std::endl;
  std::cout << "  --N: " << num_files << std::endl;
  std::cout << "  --keywords: " << num_keywords << std::endl;
  std::cout << "  --updates: " << num_updates << std::endl;
  std::cout << "  --searches: " << num_searches << std::endl;
  std::cout << "  --scalability: " << (scalability ? "yes" : "no") << std::endl;
}

void configureClientSearchFixedW1(
    nomos::benchmark::ClientSearchFixedW1Experiment* exp,
    const std::vector<std::string>& args) {
  std::string dataset_name = "all";
  std::string output_dir = "results/ch4/";
  std::string scheme = "all";
  bool search_only = false;

  for (size_t i = 0; i < args.size(); ++i) {
    if (args[i] == "--dataset" && i + 1 < args.size()) {
      dataset_name = args[++i];
    } else if (args[i] == "--output-dir" && i + 1 < args.size()) {
      output_dir = args[++i];
    } else if (args[i] == "--scheme" && i + 1 < args.size()) {
      scheme = args[++i];
    } else if (args[i] == "--search-only") {
      search_only = true;
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
  exp->setSearchOnly(search_only);

  std::cout << "Configuration:" << std::endl;
  std::cout << "  --dataset: " << dataset_name << std::endl;
  std::cout << "  --output-dir: " << output_dir << std::endl;
  std::cout << "  --scheme: " << scheme << std::endl;
  std::cout << "  --search-only: " << (search_only ? "yes" : "no") << std::endl;
}

void configureClientSearchFixedW2(
    nomos::benchmark::ClientSearchFixedW2Experiment* exp,
    const std::vector<std::string>& args) {
  std::string dataset_name = "all";
  size_t max_points = 0;
  std::string output_dir = "results/ch4/";
  std::string scheme = "all";
  bool search_only = false;

  for (size_t i = 0; i < args.size(); ++i) {
    if (args[i] == "--dataset" && i + 1 < args.size()) {
      dataset_name = args[++i];
    } else if (args[i] == "--max-points" && i + 1 < args.size()) {
      max_points = std::stoul(args[++i]);
    } else if (args[i] == "--output-dir" && i + 1 < args.size()) {
      output_dir = args[++i];
    } else if (args[i] == "--scheme" && i + 1 < args.size()) {
      scheme = args[++i];
    } else if (args[i] == "--search-only") {
      search_only = true;
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
  exp->setSearchOnly(search_only);

  std::cout << "Configuration:" << std::endl;
  std::cout << "  --dataset: " << dataset_name << std::endl;
  std::cout << "  --max-points: " << max_points << std::endl;
  std::cout << "  --output-dir: " << output_dir << std::endl;
  std::cout << "  --scheme: " << scheme << std::endl;
  std::cout << "  --search-only: " << (search_only ? "yes" : "no") << std::endl;
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

    // Configure comparative benchmark if needed
    if (experimentName == "comparative-benchmark" && !args.empty()) {
      auto* comp_exp =
          dynamic_cast<nomos::benchmark::ComparativeBenchmarkExperiment*>(
              experiment.get());
      if (comp_exp) {
        configureComparativeBenchmark(comp_exp, args);
      }
    } else if (experimentName == "chapter4-client-search-fixed-w1") {
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
