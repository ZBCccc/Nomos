#include "benchmark/ClientSearchFixedW1Experiment.hpp"

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

const size_t kFixedUpdW1 = 10;
const size_t kQTreeCapacity = 1024;

std::string joinPath(const std::string& base, const std::string& child) {
  if (base.empty() || base == ".") {
    return child;
  }
  if (child.empty()) {
    return base;
  }
  if (base[base.size() - 1] == '/') {
    return base + child;
  }
  return base + "/" + child;
}

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

size_t getSharedDocumentCount(size_t upd_w2) {
  return std::min(kFixedUpdW1, upd_w2);
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

ClientSearchFixedW1Experiment::ClientSearchFixedW1Experiment()
    : dataset_(DatasetLoader::Dataset::None),
      run_all_datasets_(true),
      repeat_count_(1),
      output_dir_("/Users/cyan/code/paper/Nomos/results/ch4/") {}

int ClientSearchFixedW1Experiment::setup() {
  std::cout << "[ClientSearchFixedW1] Setting up..." << std::endl;
  ensureDirectory(joinPath(output_dir_, "client_search_time_fixed_w1"));
  ensureDirectory(joinPath(output_dir_, "server_search_time_fixed_w1"));
  ensureDirectory(joinPath(output_dir_, "gatekeeper_search_time_fixed_w1"));
  return 0;
}

void ClientSearchFixedW1Experiment::run() {
  std::cout << "[ClientSearchFixedW1] Running Chapter 4 client search benchmark"
            << std::endl;
  std::cout << "Output directory: " << output_dir_ << std::endl;
  std::cout << "Repeat count: " << repeat_count_ << std::endl;

  const std::vector<DatasetLoader::Dataset> datasets = getDatasetsToRun();
  for (size_t i = 0; i < datasets.size(); ++i) {
    const DatasetSpec spec = buildDatasetSpec(datasets[i]);
    std::cout << "\n[ClientSearchFixedW1] Dataset " << (i + 1) << "/"
              << datasets.size() << ": " << datasetToString(spec.dataset)
              << " (" << spec.upd_w2_values.size() << " points)" << std::endl;
    runDataset(spec);
  }

  std::cout << "\n[ClientSearchFixedW1] Benchmark complete." << std::endl;
}

void ClientSearchFixedW1Experiment::teardown() {
  std::cout << "[ClientSearchFixedW1] Tearing down..." << std::endl;
}

std::string ClientSearchFixedW1Experiment::getName() const {
  return "chapter4-client-search-fixed-w1";
}

void ClientSearchFixedW1Experiment::setDataset(DatasetLoader::Dataset dataset) {
  dataset_ = dataset;
  run_all_datasets_ = (dataset == DatasetLoader::Dataset::None);
}

void ClientSearchFixedW1Experiment::setRunAllDatasets(bool value) {
  run_all_datasets_ = value;
}

void ClientSearchFixedW1Experiment::setRepeatCount(size_t repeat_count) {
  repeat_count_ = (repeat_count == 0) ? 1 : repeat_count;
}

void ClientSearchFixedW1Experiment::setOutputDir(
    const std::string& output_dir) {
  if (!output_dir.empty()) {
    output_dir_ = output_dir;
  }
}

std::vector<DatasetLoader::Dataset>
ClientSearchFixedW1Experiment::getDatasetsToRun() const {
  if (!run_all_datasets_ && dataset_ != DatasetLoader::Dataset::None) {
    return std::vector<DatasetLoader::Dataset>(1, dataset_);
  }

  return std::vector<DatasetLoader::Dataset>{DatasetLoader::Dataset::Crime,
                                             DatasetLoader::Dataset::Enron,
                                             DatasetLoader::Dataset::Wiki};
}

ClientSearchFixedW1Experiment::DatasetSpec
ClientSearchFixedW1Experiment::buildDatasetSpec(
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

  const std::map<size_t, std::string>::const_iterator w1_it =
      representatives.find(kFixedUpdW1);
  if (w1_it == representatives.end()) {
    throw std::runtime_error("Dataset has no keyword with |Upd(w1)| = 10: " +
                             datasetToString(dataset));
  }

  DatasetSpec spec;
  spec.dataset = dataset;
  spec.w1_keyword = w1_it->second;

  const std::vector<size_t>& all_frequencies =
      loader.getAllKeywordFrequencies();
  if (all_frequencies.empty()) {
    throw std::runtime_error("Dataset has no keyword frequencies: " +
                             datasetToString(dataset));
  }

  // Requirement: |Upd(w2)| sweeps all keyword update counts from JSON,
  // preserving duplicates and sorted file order.
  spec.upd_w2_values = all_frequencies;

  for (std::map<size_t, std::string>::const_iterator it =
           representatives.begin();
       it != representatives.end(); ++it) {
    if (it->second != spec.w1_keyword) {
      spec.w2_keyword = it->second;
      break;
    }
  }

  if (spec.w2_keyword.empty()) {
    throw std::runtime_error(
        "Failed to select a distinct w2 keyword for dataset: " +
        datasetToString(dataset));
  }

  return spec;
}

void ClientSearchFixedW1Experiment::runDataset(const DatasetSpec& spec) const {
  std::vector<ClientSearchRow> nomos_rows;
  std::vector<ClientSearchRow> mcodxt_rows;
  std::vector<ClientSearchRow> vqnomos_rows;

  const SweepResult nomos_times = runNomosSweep(spec);
  const SweepResult mcodxt_times = runMcOdxtSweep(spec);
  const SweepResult vqnomos_times = runVQNomosSweep(spec);

  nomos_rows.reserve(spec.upd_w2_values.size());
  mcodxt_rows.reserve(spec.upd_w2_values.size());
  vqnomos_rows.reserve(spec.upd_w2_values.size());

  for (size_t i = 0; i < spec.upd_w2_values.size(); ++i) {
    const size_t upd_w2 = spec.upd_w2_values[i];

    ClientSearchRow nomos_row;
    nomos_row.dataset = datasetToString(spec.dataset);
    nomos_row.scheme = "Nomos";
    nomos_row.upd_w1 = kFixedUpdW1;
    nomos_row.upd_w2 = upd_w2;
    nomos_row.client_time_ms = nomos_times.client_times[i];
    nomos_row.server_time_ms = nomos_times.server_times[i];
    nomos_row.gatekeeper_time_ms = nomos_times.gatekeeper_times[i];
    nomos_row.repeat = repeat_count_;
    nomos_rows.push_back(nomos_row);

    ClientSearchRow mcodxt_row = nomos_row;
    mcodxt_row.scheme = "MC-ODXT";
    mcodxt_row.client_time_ms = mcodxt_times.client_times[i];
    mcodxt_row.server_time_ms = mcodxt_times.server_times[i];
    mcodxt_row.gatekeeper_time_ms = mcodxt_times.gatekeeper_times[i];
    mcodxt_rows.push_back(mcodxt_row);

    ClientSearchRow vqnomos_row = nomos_row;
    vqnomos_row.scheme = "VQNomos";
    vqnomos_row.client_time_ms = vqnomos_times.client_times[i];
    vqnomos_row.server_time_ms = vqnomos_times.server_times[i];
    vqnomos_row.gatekeeper_time_ms = vqnomos_times.gatekeeper_times[i];
    vqnomos_rows.push_back(vqnomos_row);

    if ((i + 1) % 250 == 0 || i + 1 == spec.upd_w2_values.size()) {
      std::cout << "  Processed " << (i + 1) << "/" << spec.upd_w2_values.size()
                << " points" << std::endl;
    }
  }

  const std::vector<std::pair<std::string, std::string> > outputs = {
      std::make_pair(joinPath(output_dir_, "client_search_time_fixed_w1"),
                     "client_time_ms"),
      std::make_pair(joinPath(output_dir_, "server_search_time_fixed_w1"),
                     "server_time_ms"),
      std::make_pair(joinPath(output_dir_, "gatekeeper_search_time_fixed_w1"),
                     "gatekeeper_time_ms")};

  for (size_t d = 0; d < outputs.size(); ++d) {
    writeSchemeCsv(outputs[d].first, "Nomos", nomos_rows, outputs[d].second);
    writeSchemeCsv(outputs[d].first, "MC-ODXT", mcodxt_rows,
                   outputs[d].second);
    writeSchemeCsv(outputs[d].first, "VQNomos", vqnomos_rows,
                   outputs[d].second);
  }
}

void ClientSearchFixedW1Experiment::writeSchemeCsv(
    const std::string& output_dir, const std::string& scheme,
    const std::vector<ClientSearchRow>& rows,
    const std::string& time_column) const {
  if (rows.empty()) {
    return;
  }

  const std::string filename =
      joinPath(output_dir, scheme + "_" + rows.front().dataset + ".csv");
  std::ofstream file(filename.c_str());
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open output file: " + filename);
  }

  file << "dataset,scheme,upd_w1,upd_w2," << time_column << ",repeat\n";
  for (size_t i = 0; i < rows.size(); ++i) {
    const ClientSearchRow& row = rows[i];
    double time_value = row.client_time_ms;
    if (time_column == "server_time_ms") {
      time_value = row.server_time_ms;
    } else if (time_column == "gatekeeper_time_ms") {
      time_value = row.gatekeeper_time_ms;
    }

    file << row.dataset << "," << row.scheme << "," << row.upd_w1 << ","
         << row.upd_w2 << "," << time_value << "," << row.repeat << "\n";
  }
}

ClientSearchFixedW1Experiment::SweepResult
ClientSearchFixedW1Experiment::runNomosSweep(const DatasetSpec& spec) const {
  SweepResult result;
  result.client_times.resize(spec.upd_w2_values.size(), 0.0);
  result.gatekeeper_times.resize(spec.upd_w2_values.size(), 0.0);
  result.server_times.resize(spec.upd_w2_values.size(), 0.0);

  const std::vector<std::string> w1_doc_ids = makeSharedDocIds(kFixedUpdW1);
  const std::vector<std::string> query = {spec.w1_keyword, spec.w2_keyword};

  for (size_t iteration = 0; iteration < repeat_count_; ++iteration) {
    Gatekeeper gatekeeper;
    Client client;
    Server server;
    size_t current_w2_count = 0;

    gatekeeper.setup(10);
    client.setup();
    server.setup(gatekeeper.getKm());

    for (size_t i = 0; i < kFixedUpdW1; ++i) {
      const UpdateMetadata meta =
          gatekeeper.update(OP_ADD, w1_doc_ids[i], spec.w1_keyword);
      server.update(meta);
    }

    for (size_t point = 0; point < spec.upd_w2_values.size(); ++point) {
      const size_t target_w2_count = spec.upd_w2_values[point];
      for (size_t next = current_w2_count; next < target_w2_count; ++next) {
        const std::string doc_id =
            (next < kFixedUpdW1) ? w1_doc_ids[next]
                                 : "w2_only_doc_" + std::to_string(next + 1);
        const UpdateMetadata meta =
            gatekeeper.update(OP_ADD, doc_id, spec.w2_keyword);
        server.update(meta);
      }
      current_w2_count = target_w2_count;

      const std::chrono::high_resolution_clock::time_point client_gen_start =
          std::chrono::high_resolution_clock::now();
      TokenRequest token_request =
          client.genToken(query, gatekeeper.getUpdateCounts());
      const std::chrono::high_resolution_clock::time_point client_gen_end =
          std::chrono::high_resolution_clock::now();

      const std::chrono::high_resolution_clock::time_point gatekeeper_start =
          std::chrono::high_resolution_clock::now();
      SearchToken search_token = gatekeeper.genToken(token_request);
      const std::chrono::high_resolution_clock::time_point gatekeeper_end =
          std::chrono::high_resolution_clock::now();

      const std::chrono::high_resolution_clock::time_point prepare_start =
          std::chrono::high_resolution_clock::now();
      Client::SearchRequest request =
          client.prepareSearch(search_token, token_request);
      const std::chrono::high_resolution_clock::time_point prepare_end =
          std::chrono::high_resolution_clock::now();

      const std::chrono::high_resolution_clock::time_point server_start =
          std::chrono::high_resolution_clock::now();
      const std::vector<SearchResultEntry> encrypted_results =
          server.search(request);
      const std::chrono::high_resolution_clock::time_point server_end =
          std::chrono::high_resolution_clock::now();

      const std::chrono::high_resolution_clock::time_point decrypt_start =
          std::chrono::high_resolution_clock::now();
      client.decryptResults(encrypted_results, search_token);
      const std::chrono::high_resolution_clock::time_point decrypt_end =
          std::chrono::high_resolution_clock::now();

      result.client_times[point] +=
          durationToMilliseconds(client_gen_end - client_gen_start) +
          durationToMilliseconds(prepare_end - prepare_start) +
          durationToMilliseconds(decrypt_end - decrypt_start);
      result.gatekeeper_times[point] +=
          durationToMilliseconds(gatekeeper_end - gatekeeper_start);
      result.server_times[point] +=
          durationToMilliseconds(server_end - server_start);
    }
  }

  for (size_t i = 0; i < result.client_times.size(); ++i) {
    result.client_times[i] /= static_cast<double>(repeat_count_);
    result.gatekeeper_times[i] /= static_cast<double>(repeat_count_);
    result.server_times[i] /= static_cast<double>(repeat_count_);
  }
  return result;
}

ClientSearchFixedW1Experiment::SweepResult
ClientSearchFixedW1Experiment::runMcOdxtSweep(const DatasetSpec& spec) const {
  SweepResult result;
  result.client_times.resize(spec.upd_w2_values.size(), 0.0);
  result.gatekeeper_times.resize(spec.upd_w2_values.size(), 0.0);
  result.server_times.resize(spec.upd_w2_values.size(), 0.0);

  const std::vector<std::string> w1_doc_ids = makeSharedDocIds(kFixedUpdW1);
  const std::vector<std::string> query = {spec.w1_keyword, spec.w2_keyword};

  for (size_t iteration = 0; iteration < repeat_count_; ++iteration) {
    mcodxt::McOdxtGatekeeper gatekeeper;
    mcodxt::McOdxtServer server;
    mcodxt::McOdxtClient client;
    size_t current_w2_count = 0;

    gatekeeper.setup(10);
    client.setup();
    server.setup(gatekeeper.getKm());

    for (size_t i = 0; i < kFixedUpdW1; ++i) {
      mcodxt::UpdateMetadata meta = gatekeeper.update(
          mcodxt::OpType::ADD, w1_doc_ids[i], spec.w1_keyword);
      server.update(meta);
    }

    for (size_t point = 0; point < spec.upd_w2_values.size(); ++point) {
      const size_t target_w2_count = spec.upd_w2_values[point];
      for (size_t next = current_w2_count; next < target_w2_count; ++next) {
        const std::string doc_id =
            (next < kFixedUpdW1) ? w1_doc_ids[next]
                                 : "w2_only_doc_" + std::to_string(next + 1);
        mcodxt::UpdateMetadata meta =
            gatekeeper.update(mcodxt::OpType::ADD, doc_id, spec.w2_keyword);
        server.update(meta);
      }
      current_w2_count = target_w2_count;

      const std::chrono::high_resolution_clock::time_point client_gen_start =
          std::chrono::high_resolution_clock::now();
      mcodxt::TokenRequest token_request =
          client.genToken(query, gatekeeper.getUpdateCounts());
      const std::chrono::high_resolution_clock::time_point client_gen_end =
          std::chrono::high_resolution_clock::now();

      const std::chrono::high_resolution_clock::time_point gatekeeper_start =
          std::chrono::high_resolution_clock::now();
      mcodxt::SearchToken token = gatekeeper.genToken(token_request);
      const std::chrono::high_resolution_clock::time_point gatekeeper_end =
          std::chrono::high_resolution_clock::now();

      const std::chrono::high_resolution_clock::time_point prepare_start =
          std::chrono::high_resolution_clock::now();
      mcodxt::McOdxtClient::SearchRequest request =
          client.prepareSearch(token, token_request);
      const std::chrono::high_resolution_clock::time_point prepare_end =
          std::chrono::high_resolution_clock::now();

      const std::chrono::high_resolution_clock::time_point server_start =
          std::chrono::high_resolution_clock::now();
      const std::vector<mcodxt::SearchResultEntry> encrypted_results =
          server.search(request);
      const std::chrono::high_resolution_clock::time_point server_end =
          std::chrono::high_resolution_clock::now();

      const std::chrono::high_resolution_clock::time_point decrypt_start =
          std::chrono::high_resolution_clock::now();
      client.decryptResults(encrypted_results, token);
      const std::chrono::high_resolution_clock::time_point decrypt_end =
          std::chrono::high_resolution_clock::now();

      result.client_times[point] +=
          durationToMilliseconds(client_gen_end - client_gen_start) +
          durationToMilliseconds(prepare_end - prepare_start) +
          durationToMilliseconds(decrypt_end - decrypt_start);
      result.gatekeeper_times[point] +=
          durationToMilliseconds(gatekeeper_end - gatekeeper_start);
      result.server_times[point] +=
          durationToMilliseconds(server_end - server_start);
    }
  }

  for (size_t i = 0; i < result.client_times.size(); ++i) {
    result.client_times[i] /= static_cast<double>(repeat_count_);
    result.gatekeeper_times[i] /= static_cast<double>(repeat_count_);
    result.server_times[i] /= static_cast<double>(repeat_count_);
  }
  return result;
}

ClientSearchFixedW1Experiment::SweepResult
ClientSearchFixedW1Experiment::runVQNomosSweep(const DatasetSpec& spec) const {
  SweepResult result;
  result.client_times.resize(spec.upd_w2_values.size(), 0.0);
  result.gatekeeper_times.resize(spec.upd_w2_values.size(), 0.0);
  result.server_times.resize(spec.upd_w2_values.size(), 0.0);

  const std::vector<std::string> w1_doc_ids = makeSharedDocIds(kFixedUpdW1);
  const std::vector<std::string> query = {spec.w1_keyword, spec.w2_keyword};

  for (size_t iteration = 0; iteration < repeat_count_; ++iteration) {
    vqnomos::Gatekeeper gatekeeper;
    vqnomos::Client client;
    vqnomos::Server server;
    size_t current_w2_count = 0;

    gatekeeper.setup(10, kQTreeCapacity);
    const vqnomos::Anchor initial_anchor = gatekeeper.getCurrentAnchor();
    client.setup(gatekeeper.getPublicKeyPem(), initial_anchor, kQTreeCapacity,
                 10);
    server.setup(gatekeeper.getKm(), initial_anchor, kQTreeCapacity);

    for (size_t i = 0; i < kFixedUpdW1; ++i) {
      const vqnomos::UpdateMetadata meta =
          gatekeeper.update(vqnomos::OP_ADD, w1_doc_ids[i], spec.w1_keyword);
      server.update(meta);
    }

    for (size_t point = 0; point < spec.upd_w2_values.size(); ++point) {
      const size_t target_w2_count = spec.upd_w2_values[point];
      for (size_t next = current_w2_count; next < target_w2_count; ++next) {
        const std::string doc_id =
            (next < kFixedUpdW1) ? w1_doc_ids[next]
                                 : "w2_only_doc_" + std::to_string(next + 1);
        const vqnomos::UpdateMetadata meta =
            gatekeeper.update(vqnomos::OP_ADD, doc_id, spec.w2_keyword);
        server.update(meta);
      }
      current_w2_count = target_w2_count;

      const std::chrono::high_resolution_clock::time_point client_gen_start =
          std::chrono::high_resolution_clock::now();
      vqnomos::TokenRequest token_request =
          client.genToken(query, gatekeeper.getUpdateCounts());
      const std::chrono::high_resolution_clock::time_point client_gen_end =
          std::chrono::high_resolution_clock::now();

      const std::chrono::high_resolution_clock::time_point gatekeeper_start =
          std::chrono::high_resolution_clock::now();
      vqnomos::SearchToken search_token = gatekeeper.genToken(token_request);
      const std::chrono::high_resolution_clock::time_point gatekeeper_end =
          std::chrono::high_resolution_clock::now();

      const std::chrono::high_resolution_clock::time_point prepare_start =
          std::chrono::high_resolution_clock::now();
      vqnomos::SearchRequest request =
          client.prepareSearch(search_token, token_request);
      const std::chrono::high_resolution_clock::time_point prepare_end =
          std::chrono::high_resolution_clock::now();

      const std::chrono::high_resolution_clock::time_point server_start =
          std::chrono::high_resolution_clock::now();
      const vqnomos::SearchResponse response =
          server.search(request, search_token);
      const std::chrono::high_resolution_clock::time_point server_end =
          std::chrono::high_resolution_clock::now();

      const std::chrono::high_resolution_clock::time_point verify_start =
          std::chrono::high_resolution_clock::now();
      client.decryptAndVerify(response, search_token, token_request);
      const std::chrono::high_resolution_clock::time_point verify_end =
          std::chrono::high_resolution_clock::now();

      result.client_times[point] +=
          durationToMilliseconds(client_gen_end - client_gen_start) +
          durationToMilliseconds(prepare_end - prepare_start) +
          durationToMilliseconds(verify_end - verify_start);
      result.gatekeeper_times[point] +=
          durationToMilliseconds(gatekeeper_end - gatekeeper_start);
      result.server_times[point] +=
          durationToMilliseconds(server_end - server_start);
    }
  }

  for (size_t i = 0; i < result.client_times.size(); ++i) {
    result.client_times[i] /= static_cast<double>(repeat_count_);
    result.gatekeeper_times[i] /= static_cast<double>(repeat_count_);
    result.server_times[i] /= static_cast<double>(repeat_count_);
  }
  return result;
}

}  // namespace benchmark
}  // namespace nomos
