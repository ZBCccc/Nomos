#include "benchmark/ClientSearchFixedW2Experiment.hpp"

#include <sys/stat.h>
#include <sys/types.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "mc-odxt/McOdxtClient.hpp"
#include "mc-odxt/McOdxtGatekeeper.hpp"
#include "mc-odxt/McOdxtServer.hpp"
#include "mc-odxt/McOdxtTypes.hpp"
#include "nomos/Client.hpp"
#include "nomos/Gatekeeper.hpp"
#include "nomos/Server.hpp"
#include "vq-nomos/Client.hpp"
#include "vq-nomos/Gatekeeper.hpp"
#include "vq-nomos/Server.hpp"

namespace nomos {
namespace benchmark {
namespace {

const size_t kQTreeCapacity = 1024;

void ensureDirectory(const std::string& path) {
  if (path.empty()) {
    return;
  }

  if (path == "/") {
    return;
  }

  struct stat st;
  if (stat(path.c_str(), &st) == 0) {
    if (!S_ISDIR(st.st_mode)) {
      throw std::runtime_error("Path exists but is not a directory: " + path);
    }
    return;
  }

  std::string parent = path;
  std::string::size_type slash = parent.find_last_of('/');
  if (slash != std::string::npos && slash > 0) {
    parent = parent.substr(0, slash);
    ensureDirectory(parent);
  }

  if (mkdir(path.c_str(), 0755) != 0 && errno != EEXIST) {
    throw std::runtime_error("Failed to create directory: " + path);
  }
}

template <typename ClockDuration>
double durationToMilliseconds(const ClockDuration& duration) {
  return std::chrono::duration<double, std::milli>(duration).count();
}

std::vector<std::string> makeSharedDocIds(size_t count) {
  std::vector<std::string> docs;
  docs.reserve(count);
  for (size_t i = 0; i < count; ++i) {
    docs.push_back("shared_doc_" + std::to_string(i + 1));
  }
  return docs;
}

}  // namespace

ClientSearchFixedW2Experiment::ClientSearchFixedW2Experiment()
    : dataset_(DatasetLoader::Dataset::None),
      run_all_datasets_(true),
      repeat_count_(1),
      max_points_(0),
      output_dir_(
          "/Users/cyan/code/paper/Nomos/results/ch4/"
          "client_search_time_fixed_w2") {}

int ClientSearchFixedW2Experiment::setup() {
  std::cout << "[ClientSearchFixedW2] Setting up..." << std::endl;
  ensureDirectory(output_dir_);
  return 0;
}

void ClientSearchFixedW2Experiment::run() {
  std::cout << "[ClientSearchFixedW2] Running Chapter 4 client search benchmark"
            << std::endl;
  std::cout << "Output directory: " << output_dir_ << std::endl;
  std::cout << "Repeat count: " << repeat_count_ << std::endl;

  const std::vector<DatasetLoader::Dataset> datasets = getDatasetsToRun();
  for (size_t i = 0; i < datasets.size(); ++i) {
    const DatasetSpec spec = buildDatasetSpec(datasets[i]);
    std::cout << "\n[ClientSearchFixedW2] Dataset " << (i + 1) << "/"
              << datasets.size() << ": " << datasetToString(spec.dataset)
              << " (|Upd(w2)| = " << spec.upd_w2_fixed << ", "
              << spec.upd_w1_values.size() << " points)" << std::endl;
    runDataset(spec);
  }

  std::cout << "\n[ClientSearchFixedW2] Benchmark complete." << std::endl;
}

void ClientSearchFixedW2Experiment::teardown() {
  std::cout << "[ClientSearchFixedW2] Tearing down..." << std::endl;
}

std::string ClientSearchFixedW2Experiment::getName() const {
  return "chapter4-client-search-fixed-w2";
}

void ClientSearchFixedW2Experiment::setDataset(DatasetLoader::Dataset dataset) {
  dataset_ = dataset;
  run_all_datasets_ = (dataset == DatasetLoader::Dataset::None);
}

void ClientSearchFixedW2Experiment::setRunAllDatasets(bool value) {
  run_all_datasets_ = value;
}

void ClientSearchFixedW2Experiment::setRepeatCount(size_t repeat_count) {
  repeat_count_ = (repeat_count == 0) ? 1 : repeat_count;
}

void ClientSearchFixedW2Experiment::setOutputDir(
    const std::string& output_dir) {
  if (!output_dir.empty()) {
    output_dir_ = output_dir;
  }
}

void ClientSearchFixedW2Experiment::setMaxPoints(size_t max_points) {
  max_points_ = max_points;
}

std::vector<DatasetLoader::Dataset>
ClientSearchFixedW2Experiment::getDatasetsToRun() const {
  if (!run_all_datasets_ && dataset_ != DatasetLoader::Dataset::None) {
    return std::vector<DatasetLoader::Dataset>(1, dataset_);
  }

  return std::vector<DatasetLoader::Dataset>{DatasetLoader::Dataset::Crime,
                                             DatasetLoader::Dataset::Enron,
                                             DatasetLoader::Dataset::Wiki};
}

ClientSearchFixedW2Experiment::DatasetSpec
ClientSearchFixedW2Experiment::buildDatasetSpec(
    DatasetLoader::Dataset dataset) const {
  DatasetLoader loader(dataset);
  if (!loader.load()) {
    throw std::runtime_error("Failed to load dataset: " +
                             datasetToString(dataset));
  }

  const std::map<size_t, std::string> representatives =
      loader.getRepresentativeKeywordsByFrequency();
  if (representatives.empty()) {
    throw std::runtime_error("Dataset has no keyword frequencies: " +
                             datasetToString(dataset));
  }

  DatasetSpec spec;
  spec.dataset = dataset;

  // Find the maximum frequency (w2)
  std::map<size_t, std::string>::const_reverse_iterator max_it =
      representatives.rbegin();
  spec.upd_w2_fixed = max_it->first;
  spec.w2_keyword = max_it->second;

  // Collect all frequencies for w1 (all keywords in the dataset)
  for (std::map<size_t, std::string>::const_iterator it =
           representatives.begin();
       it != representatives.end(); ++it) {
    if (it->second == spec.w2_keyword) {
      continue;
    }
    spec.upd_w1_values.push_back(std::make_pair(it->first, it->second));
  }

  if (max_points_ > 0 && spec.upd_w1_values.size() > max_points_) {
    spec.upd_w1_values.resize(max_points_);
  }

  return spec;
}

void ClientSearchFixedW2Experiment::runDataset(const DatasetSpec& spec) const {
  std::vector<ClientSearchRow> nomos_rows;
  std::vector<ClientSearchRow> mcodxt_rows;
  std::vector<ClientSearchRow> vqnomos_rows;
  const std::vector<double> nomos_times = runNomosSweep(spec);
  const std::vector<double> mcodxt_times = runMcOdxtSweep(spec);
  const std::vector<double> vqnomos_times = runVQNomosSweep(spec);

  nomos_rows.reserve(spec.upd_w1_values.size());
  mcodxt_rows.reserve(spec.upd_w1_values.size());
  vqnomos_rows.reserve(spec.upd_w1_values.size());

  for (size_t i = 0; i < spec.upd_w1_values.size(); ++i) {
    const size_t upd_w1 = spec.upd_w1_values[i].first;
    const double nomos_time_ms = nomos_times[i];
    const double mcodxt_time_ms = mcodxt_times[i];
    const double vqnomos_time_ms = vqnomos_times[i];

    ClientSearchRow nomos_row;
    nomos_row.dataset = datasetToString(spec.dataset);
    nomos_row.scheme = "Nomos";
    nomos_row.upd_w1 = upd_w1;
    nomos_row.upd_w2 = spec.upd_w2_fixed;
    nomos_row.client_search_time_ms = nomos_time_ms;
    nomos_row.repeat = repeat_count_;
    nomos_rows.push_back(nomos_row);

    ClientSearchRow mcodxt_row = nomos_row;
    mcodxt_row.scheme = "MC-ODXT";
    mcodxt_row.client_search_time_ms = mcodxt_time_ms;
    mcodxt_rows.push_back(mcodxt_row);

    ClientSearchRow vqnomos_row = nomos_row;
    vqnomos_row.scheme = "VQNomos";
    vqnomos_row.client_search_time_ms = vqnomos_time_ms;
    vqnomos_rows.push_back(vqnomos_row);

    if ((i + 1) % 250 == 0 || i + 1 == spec.upd_w1_values.size()) {
      std::cout << "  Processed " << (i + 1) << "/" << spec.upd_w1_values.size()
                << " points" << std::endl;
    }
  }

  writeSchemeCsv("Nomos", nomos_rows);
  writeSchemeCsv("MC-ODXT", mcodxt_rows);
  writeSchemeCsv("VQNomos", vqnomos_rows);
}

void ClientSearchFixedW2Experiment::writeSchemeCsv(
    const std::string& scheme, const std::vector<ClientSearchRow>& rows) const {
  if (rows.empty()) {
    return;
  }

  const std::string filename =
      output_dir_ + "/" + scheme + "_" + rows.front().dataset + ".csv";
  std::ofstream file(filename.c_str());
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open output file: " + filename);
  }

  file << "dataset,scheme,upd_w1,upd_w2,client_search_time_ms,repeat\n";
  for (size_t i = 0; i < rows.size(); ++i) {
    const ClientSearchRow& row = rows[i];
    file << row.dataset << "," << row.scheme << "," << row.upd_w1 << ","
         << row.upd_w2 << "," << row.client_search_time_ms << "," << row.repeat
         << "\n";
  }
}

std::vector<double> ClientSearchFixedW2Experiment::runNomosSweep(
    const DatasetSpec& spec) const {
  std::vector<double> totals(spec.upd_w1_values.size(), 0.0);
  const std::vector<std::string> w2_doc_ids = makeSharedDocIds(spec.upd_w2_fixed);

  for (size_t iteration = 0; iteration < repeat_count_; ++iteration) {
    Gatekeeper gatekeeper;
    Client client;
    Server server;

    gatekeeper.setup(10);
    client.setup();
    server.setup(gatekeeper.getKm());

    // Insert all w2 documents first (fixed)
    for (size_t i = 0; i < spec.upd_w2_fixed; ++i) {
      const UpdateMetadata meta =
          gatekeeper.update(OP_ADD, w2_doc_ids[i], spec.w2_keyword);
      server.update(meta);
    }

    for (size_t point = 0; point < spec.upd_w1_values.size(); ++point) {
      const size_t target_w1_count = spec.upd_w1_values[point].first;
      const std::string w1_keyword = spec.upd_w1_values[point].second;

      // Each representative keyword appears only once in the sweep, so insert
      // exactly |Upd(w1)| updates for the current keyword.
      for (size_t next = 0; next < target_w1_count; ++next) {
        const std::string doc_id =
            (next < spec.upd_w2_fixed) ? w2_doc_ids[next]
                                       : "w1_only_doc_" + std::to_string(next + 1);
        const UpdateMetadata meta =
            gatekeeper.update(OP_ADD, doc_id, w1_keyword);
        server.update(meta);
      }

      // Measure client search time
      const std::chrono::high_resolution_clock::time_point token_start =
          std::chrono::high_resolution_clock::now();
      TokenRequest token_request =
          client.genToken({w1_keyword, spec.w2_keyword}, gatekeeper.getUpdateCounts());
      SearchToken search_token = gatekeeper.genToken(token_request);
      const std::chrono::high_resolution_clock::time_point token_end =
          std::chrono::high_resolution_clock::now();

      const std::chrono::high_resolution_clock::time_point prepare_start =
          std::chrono::high_resolution_clock::now();
      Client::SearchRequest request =
          client.prepareSearch(search_token, token_request);
      const std::chrono::high_resolution_clock::time_point prepare_end =
          std::chrono::high_resolution_clock::now();

      const std::vector<SearchResultEntry> encrypted_results =
          server.search(request);

      const std::chrono::high_resolution_clock::time_point decrypt_start =
          std::chrono::high_resolution_clock::now();
      client.decryptResults(encrypted_results, search_token);
      const std::chrono::high_resolution_clock::time_point decrypt_end =
          std::chrono::high_resolution_clock::now();

      totals[point] += durationToMilliseconds(token_end - token_start);
      totals[point] += durationToMilliseconds(prepare_end - prepare_start);
      totals[point] += durationToMilliseconds(decrypt_end - decrypt_start);
    }
  }

  for (size_t i = 0; i < totals.size(); ++i) {
    totals[i] /= static_cast<double>(repeat_count_);
  }
  return totals;
}

std::vector<double> ClientSearchFixedW2Experiment::runMcOdxtSweep(
    const DatasetSpec& spec) const {
  std::vector<double> totals(spec.upd_w1_values.size(), 0.0);
  const std::vector<std::string> w2_doc_ids = makeSharedDocIds(spec.upd_w2_fixed);

  for (size_t iteration = 0; iteration < repeat_count_; ++iteration) {
    mcodxt::McOdxtGatekeeper gatekeeper;
    mcodxt::McOdxtServer server;
    mcodxt::McOdxtClient client;

    gatekeeper.setup(10);
    client.setup();
    server.setup(gatekeeper.getKm());

    // Insert all w2 documents first (fixed)
    for (size_t i = 0; i < spec.upd_w2_fixed; ++i) {
      const mcodxt::UpdateMetadata meta =
          gatekeeper.update(mcodxt::OpType::ADD, w2_doc_ids[i], spec.w2_keyword);
      server.update(meta);
    }

    for (size_t point = 0; point < spec.upd_w1_values.size(); ++point) {
      const size_t target_w1_count = spec.upd_w1_values[point].first;
      const std::string w1_keyword = spec.upd_w1_values[point].second;

      // Each representative keyword appears only once in the sweep, so insert
      // exactly |Upd(w1)| updates for the current keyword.
      for (size_t next = 0; next < target_w1_count; ++next) {
        const std::string doc_id =
            (next < spec.upd_w2_fixed) ? w2_doc_ids[next]
                                       : "w1_only_doc_" + std::to_string(next + 1);
        const mcodxt::UpdateMetadata meta =
            gatekeeper.update(mcodxt::OpType::ADD, doc_id, w1_keyword);
        server.update(meta);
      }

      // Measure client search time
      const std::chrono::high_resolution_clock::time_point token_start =
          std::chrono::high_resolution_clock::now();
      mcodxt::TokenRequest token_request =
          client.genToken({w1_keyword, spec.w2_keyword}, gatekeeper.getUpdateCounts());
      mcodxt::SearchToken search_token = gatekeeper.genToken(token_request);
      const std::chrono::high_resolution_clock::time_point token_end =
          std::chrono::high_resolution_clock::now();

      const std::chrono::high_resolution_clock::time_point prepare_start =
          std::chrono::high_resolution_clock::now();
      mcodxt::McOdxtClient::SearchRequest request =
          client.prepareSearch(search_token, token_request);
      const std::chrono::high_resolution_clock::time_point prepare_end =
          std::chrono::high_resolution_clock::now();

      const std::vector<mcodxt::SearchResultEntry> encrypted_results =
          server.search(request);

      const std::chrono::high_resolution_clock::time_point decrypt_start =
          std::chrono::high_resolution_clock::now();
      client.decryptResults(encrypted_results, search_token);
      const std::chrono::high_resolution_clock::time_point decrypt_end =
          std::chrono::high_resolution_clock::now();

      totals[point] += durationToMilliseconds(token_end - token_start);
      totals[point] += durationToMilliseconds(prepare_end - prepare_start);
      totals[point] += durationToMilliseconds(decrypt_end - decrypt_start);
    }
  }

  for (size_t i = 0; i < totals.size(); ++i) {
    totals[i] /= static_cast<double>(repeat_count_);
  }
  return totals;
}

std::vector<double> ClientSearchFixedW2Experiment::runVQNomosSweep(
    const DatasetSpec& spec) const {
  std::vector<double> totals(spec.upd_w1_values.size(), 0.0);
  const std::vector<std::string> w2_doc_ids = makeSharedDocIds(spec.upd_w2_fixed);

  for (size_t iteration = 0; iteration < repeat_count_; ++iteration) {
    vqnomos::Gatekeeper gatekeeper;
    vqnomos::Client client;
    vqnomos::Server server;

    gatekeeper.setup(10, kQTreeCapacity);
    const vqnomos::Anchor initial_anchor = gatekeeper.getCurrentAnchor();
    client.setup(gatekeeper.getPublicKeyPem(), initial_anchor, kQTreeCapacity, 10);
    server.setup(gatekeeper.getKm(), initial_anchor, kQTreeCapacity);

    // Insert all w2 documents first (fixed)
    for (size_t i = 0; i < spec.upd_w2_fixed; ++i) {
      const vqnomos::UpdateMetadata meta =
          gatekeeper.update(vqnomos::OP_ADD, w2_doc_ids[i], spec.w2_keyword);
      server.update(meta);
    }

    for (size_t point = 0; point < spec.upd_w1_values.size(); ++point) {
      const size_t target_w1_count = spec.upd_w1_values[point].first;
      const std::string w1_keyword = spec.upd_w1_values[point].second;

      // Each representative keyword appears only once in the sweep, so insert
      // exactly |Upd(w1)| updates for the current keyword.
      for (size_t next = 0; next < target_w1_count; ++next) {
        const std::string doc_id =
            (next < spec.upd_w2_fixed) ? w2_doc_ids[next]
                                       : "w1_only_doc_" + std::to_string(next + 1);
        const vqnomos::UpdateMetadata meta =
            gatekeeper.update(vqnomos::OP_ADD, doc_id, w1_keyword);
        server.update(meta);
      }

      // Measure client search time (token generation + search preparation + decrypt/verify)
      const std::chrono::high_resolution_clock::time_point token_start =
          std::chrono::high_resolution_clock::now();
      vqnomos::TokenRequest token_request =
          client.genToken({w1_keyword, spec.w2_keyword}, gatekeeper.getUpdateCounts());
      vqnomos::SearchToken search_token = gatekeeper.genToken(token_request);
      const std::chrono::high_resolution_clock::time_point token_end =
          std::chrono::high_resolution_clock::now();

      const std::chrono::high_resolution_clock::time_point prepare_start =
          std::chrono::high_resolution_clock::now();
      vqnomos::SearchRequest request =
          client.prepareSearch(search_token, token_request);
      const std::chrono::high_resolution_clock::time_point prepare_end =
          std::chrono::high_resolution_clock::now();

      const vqnomos::SearchResponse response =
          server.search(request, search_token);

      const std::chrono::high_resolution_clock::time_point verify_start =
          std::chrono::high_resolution_clock::now();
      client.decryptAndVerify(response, search_token, token_request);
      const std::chrono::high_resolution_clock::time_point verify_end =
          std::chrono::high_resolution_clock::now();

      totals[point] += durationToMilliseconds(token_end - token_start);
      totals[point] += durationToMilliseconds(prepare_end - prepare_start);
      totals[point] += durationToMilliseconds(verify_end - verify_start);
    }
  }

  for (size_t i = 0; i < totals.size(); ++i) {
    totals[i] /= static_cast<double>(repeat_count_);
  }
  return totals;
}

}  // namespace benchmark
}  // namespace nomos
