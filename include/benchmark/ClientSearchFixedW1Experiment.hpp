#pragma once

#include <string>
#include <vector>

#include "benchmark/DatasetLoader.hpp"
#include "core/Experiment.hpp"

namespace nomos {
namespace benchmark {

class ClientSearchFixedW1Experiment : public core::Experiment {
 public:
  ClientSearchFixedW1Experiment();

  int setup() override;
  void run() override;
  void teardown() override;
  std::string getName() const override;

  void setDataset(DatasetLoader::Dataset dataset);
  void setRunAllDatasets(bool value);
  void setOutputDir(const std::string& output_dir);
  void setSchemeFilter(const std::string& scheme_filter);

 private:
  struct SweepResult {
    std::vector<double> client_times;
    std::vector<double> gatekeeper_times;
    std::vector<double> server_times;
  };

  struct ClientSearchRow {
    std::string dataset;
    std::string scheme;
    size_t upd_w1;
    size_t upd_w2;
    double client_time_ms;
    double server_time_ms;
    double gatekeeper_time_ms;
  };

  struct DatasetSpec {
    DatasetLoader::Dataset dataset;
    std::string w1_keyword;
    size_t upd_w1_fixed;
    std::vector<std::pair<size_t, std::string>> upd_w2_values;
  };

  DatasetLoader::Dataset dataset_;
  bool run_all_datasets_;
  std::string output_dir_;
  std::string scheme_filter_;

  std::vector<DatasetLoader::Dataset> getDatasetsToRun() const;
  bool shouldRunScheme(const std::string& scheme_name) const;
  size_t getEnabledSchemeCount() const;
  DatasetSpec buildDatasetSpec(DatasetLoader::Dataset dataset) const;
  void runDataset(const DatasetSpec& spec) const;
  void writeSchemeCsv(const std::string& output_dir, const std::string& scheme,
                      const std::vector<ClientSearchRow>& rows,
                      const std::string& time_column) const;

  SweepResult runNomosSweep(const DatasetSpec& spec) const;
  SweepResult runMcOdxtSweep(const DatasetSpec& spec) const;
  SweepResult runVQNomosSweep(const DatasetSpec& spec) const;
};

}  // namespace benchmark
}  // namespace nomos
