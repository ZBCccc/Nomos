#pragma once

#include <string>
#include <vector>

#include "benchmark/DatasetLoader.hpp"
#include "core/Experiment.hpp"

namespace nomos {
namespace benchmark {

class ClientSearchFixedW2Experiment : public core::Experiment {
public:
    ClientSearchFixedW2Experiment();

    int setup() override;
    void run() override;
    void teardown() override;
    std::string getName() const override;

    void setDataset(DatasetLoader::Dataset dataset);
    void setRunAllDatasets(bool value);
    void setRepeatCount(size_t repeat_count);
    void setOutputDir(const std::string& output_dir);
    void setMaxPoints(size_t max_points);

private:
    struct ClientSearchRow {
        std::string dataset;
        std::string scheme;
        size_t upd_w1;
        size_t upd_w2;
        double client_search_time_ms;
        size_t repeat;
    };

    struct DatasetSpec {
        DatasetLoader::Dataset dataset;
        std::string w2_keyword;
        size_t upd_w2_fixed;
        std::vector<std::pair<size_t, std::string>> upd_w1_values;
    };

    DatasetLoader::Dataset dataset_;
    bool run_all_datasets_;
    size_t repeat_count_;
    size_t max_points_;
    std::string output_dir_;

    std::vector<DatasetLoader::Dataset> getDatasetsToRun() const;
    DatasetSpec buildDatasetSpec(DatasetLoader::Dataset dataset) const;
    void runDataset(const DatasetSpec& spec) const;
    void writeSchemeCsv(const std::string& scheme,
                        const std::vector<ClientSearchRow>& rows) const;

    std::vector<double> runNomosSweep(const DatasetSpec& spec) const;
    std::vector<double> runMcOdxtSweep(const DatasetSpec& spec) const;
    std::vector<double> runVQNomosSweep(const DatasetSpec& spec) const;
};

}  // namespace benchmark
}  // namespace nomos
