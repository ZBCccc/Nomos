#include "benchmark/ClientSearchFixedW2Experiment.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include "benchmark/BenchmarkUtils.hpp"
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

}  // namespace

ClientSearchFixedW2Experiment::ClientSearchFixedW2Experiment()
    : dataset_(DatasetLoader::Dataset::None),
      run_all_datasets_(true),
      max_points_(0),
      output_dir_("results/ch4/"),
      scheme_filter_("all") {}

int ClientSearchFixedW2Experiment::setup() {
  std::cout << "[ClientSearchFixedW2] Setting up..." << std::endl;
  ensureDirectory(joinPath(output_dir_, "client_search_time_fixed_w2"));
  ensureDirectory(joinPath(output_dir_, "server_search_time_fixed_w2"));
  ensureDirectory(joinPath(output_dir_, "gatekeeper_search_time_fixed_w2"));
  return 0;
}

void ClientSearchFixedW2Experiment::run() {
  std::cout << "[ClientSearchFixedW2] Running Chapter 4 client search benchmark"
            << std::endl;
  std::cout << "Output directory: " << output_dir_ << std::endl;

  const std::vector<DatasetLoader::Dataset> datasets = getDatasetsToRun();
  std::vector<DatasetSpec> specs;
  specs.reserve(datasets.size());

  for (size_t i = 0; i < datasets.size(); ++i) {
    const DatasetSpec spec = buildDatasetSpec(datasets[i]);
    specs.push_back(spec);
  }

  for (size_t i = 0; i < specs.size(); ++i) {
    const DatasetSpec& spec = specs[i];
    const std::string dataset_name =
        (spec.dataset == DatasetLoader::Dataset::None)
            ? "Random"
            : datasetToString(spec.dataset);
    std::cout << "\n[ClientSearchFixedW2] Dataset " << (i + 1) << "/"
              << specs.size() << ": " << dataset_name
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
}

void ClientSearchFixedW2Experiment::setRunAllDatasets(bool value) {
  run_all_datasets_ = value;
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

void ClientSearchFixedW2Experiment::setSchemeFilter(
    const std::string& scheme_filter) {
  scheme_filter_ = normalizeSchemeFilter(scheme_filter);
}

std::vector<DatasetLoader::Dataset>
ClientSearchFixedW2Experiment::getDatasetsToRun() const {
  if (!run_all_datasets_) {
    return std::vector<DatasetLoader::Dataset>(1, dataset_);
  }

  return std::vector<DatasetLoader::Dataset>{DatasetLoader::Dataset::Crime,
                                             DatasetLoader::Dataset::Enron,
                                             DatasetLoader::Dataset::Wiki};
}

bool ClientSearchFixedW2Experiment::shouldRunScheme(
    const std::string& scheme_name) const {
  return scheme_filter_ == "all" ||
         normalizeSchemeFilter(scheme_name) == scheme_filter_;
}

size_t ClientSearchFixedW2Experiment::getEnabledSchemeCount() const {
  return (scheme_filter_ == "all") ? static_cast<size_t>(3)
                                   : static_cast<size_t>(1);
}

ClientSearchFixedW2Experiment::DatasetSpec
ClientSearchFixedW2Experiment::buildDatasetSpec(
    DatasetLoader::Dataset dataset) const {
  if (dataset == DatasetLoader::Dataset::None) {
    DatasetSpec spec;
    spec.dataset = dataset;
    spec.w2_keyword = "random_w2_max";
    spec.upd_w2_fixed = 1000;

    const size_t point_count = (max_points_ > 0) ? max_points_ : 200;
    std::mt19937 rng(42);
    std::uniform_int_distribution<size_t> freq_dist(1, 1200);

    for (size_t i = 0; i < point_count; ++i) {
      spec.upd_w1_values.push_back(
          std::make_pair(freq_dist(rng), "random_w1_" + std::to_string(i + 1)));
    }
    return spec;
  }

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

  const std::vector<std::string>& all_keywords = loader.getAllKeywords();
  const std::vector<size_t>& all_frequencies =
      loader.getAllKeywordFrequencies();
  if (all_keywords.empty() || all_frequencies.empty() ||
      all_keywords.size() != all_frequencies.size()) {
    throw std::runtime_error("Dataset raw keyword/frequency mismatch: " +
                             datasetToString(dataset));
  }

  DatasetSpec spec;
  spec.dataset = dataset;

  // Find w2 as the keyword with maximum update count in the full dataset.
  auto max_it =
      std::max_element(all_frequencies.begin(), all_frequencies.end());
  size_t max_index = std::distance(all_frequencies.begin(), max_it);
  spec.upd_w2_fixed = all_frequencies[max_index];
  spec.w2_keyword = all_keywords[max_index];

  // Requirement: W1 sweeps all keywords in the dataset.
  for (size_t i = 0; i < all_keywords.size(); ++i) {
    spec.upd_w1_values.push_back(
        std::make_pair(all_frequencies[i], all_keywords[i]));
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
  SweepResult nomos_times;
  SweepResult mcodxt_times;
  SweepResult vqnomos_times;

  if (shouldRunScheme("Nomos")) {
    nomos_times = runNomosSweep(spec);
  }
  if (shouldRunScheme("MC-ODXT")) {
    mcodxt_times = runMcOdxtSweep(spec);
  }
  if (shouldRunScheme("VQNomos")) {
    vqnomos_times = runVQNomosSweep(spec);
  }

  nomos_rows.reserve(spec.upd_w1_values.size());
  mcodxt_rows.reserve(spec.upd_w1_values.size());
  vqnomos_rows.reserve(spec.upd_w1_values.size());

  for (size_t i = 0; i < spec.upd_w1_values.size(); ++i) {
    const size_t upd_w1 = spec.upd_w1_values[i].first;
    const std::string dataset_name =
        (spec.dataset == DatasetLoader::Dataset::None)
            ? "Random"
            : datasetToString(spec.dataset);

    if (!nomos_times.client_times.empty()) {
      ClientSearchRow nomos_row;
      nomos_row.dataset = dataset_name;
      nomos_row.scheme = "Nomos";
      nomos_row.upd_w1 = upd_w1;
      nomos_row.upd_w2 = spec.upd_w2_fixed;
      nomos_row.client_time_ms = nomos_times.client_times[i];
      nomos_row.server_time_ms = nomos_times.server_times[i];
      nomos_row.gatekeeper_time_ms = nomos_times.gatekeeper_times[i];
      nomos_rows.push_back(nomos_row);
    }

    if (!mcodxt_times.client_times.empty()) {
      ClientSearchRow mcodxt_row;
      mcodxt_row.dataset = dataset_name;
      mcodxt_row.scheme = "MC-ODXT";
      mcodxt_row.upd_w1 = upd_w1;
      mcodxt_row.upd_w2 = spec.upd_w2_fixed;
      mcodxt_row.client_time_ms = mcodxt_times.client_times[i];
      mcodxt_row.server_time_ms = mcodxt_times.server_times[i];
      mcodxt_row.gatekeeper_time_ms = mcodxt_times.gatekeeper_times[i];
      mcodxt_rows.push_back(mcodxt_row);
    }

    if (!vqnomos_times.client_times.empty()) {
      ClientSearchRow vqnomos_row;
      vqnomos_row.dataset = dataset_name;
      vqnomos_row.scheme = "VQNomos";
      vqnomos_row.upd_w1 = upd_w1;
      vqnomos_row.upd_w2 = spec.upd_w2_fixed;
      vqnomos_row.client_time_ms = vqnomos_times.client_times[i];
      vqnomos_row.server_time_ms = vqnomos_times.server_times[i];
      vqnomos_row.gatekeeper_time_ms = vqnomos_times.gatekeeper_times[i];
      vqnomos_rows.push_back(vqnomos_row);
    }
  }

  const std::vector<std::pair<std::string, std::string>> outputs = {
      std::make_pair(joinPath(output_dir_, "client_search_time_fixed_w2"),
                     "client_time_ms"),
      std::make_pair(joinPath(output_dir_, "server_search_time_fixed_w2"),
                     "server_time_ms"),
      std::make_pair(joinPath(output_dir_, "gatekeeper_search_time_fixed_w2"),
                     "gatekeeper_time_ms")};

  for (size_t d = 0; d < outputs.size(); ++d) {
    writeSchemeCsv(outputs[d].first, "Nomos", nomos_rows, outputs[d].second);
    writeSchemeCsv(outputs[d].first, "MC-ODXT", mcodxt_rows, outputs[d].second);
    writeSchemeCsv(outputs[d].first, "VQNomos", vqnomos_rows,
                   outputs[d].second);
  }
}

void ClientSearchFixedW2Experiment::writeSchemeCsv(
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

  file << "dataset,scheme,upd_w1,upd_w2," << time_column << "\n";
  for (size_t i = 0; i < rows.size(); ++i) {
    const ClientSearchRow& row = rows[i];
    double time_value = row.client_time_ms;
    if (time_column == "server_time_ms") {
      time_value = row.server_time_ms;
    } else if (time_column == "gatekeeper_time_ms") {
      time_value = row.gatekeeper_time_ms;
    }

    file << row.dataset << "," << row.scheme << "," << row.upd_w1 << ","
         << row.upd_w2 << "," << time_value << "\n";
  }
}

ClientSearchFixedW2Experiment::SweepResult
ClientSearchFixedW2Experiment::runNomosSweep(const DatasetSpec& spec) const {
  SweepResult result;
  result.client_times.resize(spec.upd_w1_values.size(), 0.0);
  result.gatekeeper_times.resize(spec.upd_w1_values.size(), 0.0);
  result.server_times.resize(spec.upd_w1_values.size(), 0.0);

  Gatekeeper gatekeeper;
  Client client;
  Server server;

  gatekeeper.setup(10);
  client.setup();
  server.setup(gatekeeper.getKm());

  // Insert all w1 documents
  for (size_t point = 0; point < spec.upd_w1_values.size(); ++point) {
    const size_t target_w1_count = spec.upd_w1_values[point].first;
    const std::string w1_keyword = spec.upd_w1_values[point].second;

    for (size_t next = 0; next < target_w1_count; ++next) {
      const std::string doc_id = "doc_" + std::to_string(next + 1);
      const UpdateMetadata meta = gatekeeper.update(OP_ADD, doc_id, w1_keyword);
      server.update(meta);
    }
  }

  // 执行search操作
  for (size_t point = 0; point < spec.upd_w1_values.size(); ++point) {
    const std::string w1_keyword = spec.upd_w1_values[point].second;

    const std::chrono::high_resolution_clock::time_point client_gen_start =
        std::chrono::high_resolution_clock::now();
    TokenRequest token_request = client.genToken({w1_keyword, spec.w2_keyword},
                                                 gatekeeper.getUpdateCounts());
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

    result.client_times[point] =
        durationToMilliseconds(client_gen_end - client_gen_start) +
        durationToMilliseconds(prepare_end - prepare_start) +
        durationToMilliseconds(decrypt_end - decrypt_start);
    result.gatekeeper_times[point] =
        durationToMilliseconds(gatekeeper_end - gatekeeper_start);
    result.server_times[point] =
        durationToMilliseconds(server_end - server_start);
  }

  return result;
}

ClientSearchFixedW2Experiment::SweepResult
ClientSearchFixedW2Experiment::runMcOdxtSweep(const DatasetSpec& spec) const {
  SweepResult result;
  result.client_times.resize(spec.upd_w1_values.size(), 0.0);
  result.gatekeeper_times.resize(spec.upd_w1_values.size(), 0.0);
  result.server_times.resize(spec.upd_w1_values.size(), 0.0);

  mcodxt::McOdxtGatekeeper gatekeeper;
  mcodxt::McOdxtServer server;
  mcodxt::McOdxtClient client;

  gatekeeper.setup(10);
  client.setup();
  server.setup(gatekeeper.getKm());

  // Insert all w1 documents
  for (size_t point = 0; point < spec.upd_w1_values.size(); ++point) {
    const size_t target_w1_count = spec.upd_w1_values[point].first;
    const std::string w1_keyword = spec.upd_w1_values[point].second;

    for (size_t next = 0; next < target_w1_count; ++next) {
      const std::string doc_id = "doc_" + std::to_string(next + 1);
      const mcodxt::UpdateMetadata meta =
          gatekeeper.update(mcodxt::OpType::ADD, doc_id, w1_keyword);
      server.update(meta);
    }
  }

  // 执行search操作
  for (size_t point = 0; point < spec.upd_w1_values.size(); ++point) {
    const std::string w1_keyword = spec.upd_w1_values[point].second;

    const std::chrono::high_resolution_clock::time_point client_gen_start =
        std::chrono::high_resolution_clock::now();
    mcodxt::TokenRequest token_request = client.genToken(
        {w1_keyword, spec.w2_keyword}, gatekeeper.getUpdateCounts());
    const std::chrono::high_resolution_clock::time_point client_gen_end =
        std::chrono::high_resolution_clock::now();

    const std::chrono::high_resolution_clock::time_point gatekeeper_start =
        std::chrono::high_resolution_clock::now();
    mcodxt::SearchToken search_token = gatekeeper.genToken(token_request);
    const std::chrono::high_resolution_clock::time_point gatekeeper_end =
        std::chrono::high_resolution_clock::now();

    const std::chrono::high_resolution_clock::time_point prepare_start =
        std::chrono::high_resolution_clock::now();
    mcodxt::McOdxtClient::SearchRequest request =
        client.prepareSearch(search_token, token_request);
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
    client.decryptResults(encrypted_results, search_token);
    const std::chrono::high_resolution_clock::time_point decrypt_end =
        std::chrono::high_resolution_clock::now();

    result.client_times[point] =
        durationToMilliseconds(client_gen_end - client_gen_start) +
        durationToMilliseconds(prepare_end - prepare_start) +
        durationToMilliseconds(decrypt_end - decrypt_start);
    result.gatekeeper_times[point] =
        durationToMilliseconds(gatekeeper_end - gatekeeper_start);
    result.server_times[point] =
        durationToMilliseconds(server_end - server_start);
  }

  return result;
}

ClientSearchFixedW2Experiment::SweepResult
ClientSearchFixedW2Experiment::runVQNomosSweep(const DatasetSpec& spec) const {
  SweepResult result;
  result.client_times.resize(spec.upd_w1_values.size(), 0.0);
  result.gatekeeper_times.resize(spec.upd_w1_values.size(), 0.0);
  result.server_times.resize(spec.upd_w1_values.size(), 0.0);

  vqnomos::Gatekeeper gatekeeper;
  vqnomos::Client client;
  vqnomos::Server server;

  gatekeeper.setup(10, kQTreeCapacity);
  const vqnomos::Anchor initial_anchor = gatekeeper.getCurrentAnchor();
  client.setup(gatekeeper.getPublicKeyPem(), initial_anchor, kQTreeCapacity,
               10);
  server.setup(gatekeeper.getKm(), initial_anchor, kQTreeCapacity);

  // Insert all w1 documents
  for (size_t point = 0; point < spec.upd_w1_values.size(); ++point) {
    const size_t target_w1_count = spec.upd_w1_values[point].first;
    const std::string w1_keyword = spec.upd_w1_values[point].second;

    for (size_t next = 0; next < target_w1_count; ++next) {
      const std::string doc_id = "doc_" + std::to_string(next + 1);
      const vqnomos::UpdateMetadata meta =
          gatekeeper.update(vqnomos::OP_ADD, doc_id, w1_keyword);
      server.update(meta);
    }
  }

  // 执行search操作
  for (size_t point = 0; point < spec.upd_w1_values.size(); ++point) {
    const std::string w1_keyword = spec.upd_w1_values[point].second;

    const std::chrono::high_resolution_clock::time_point client_gen_start =
        std::chrono::high_resolution_clock::now();
    vqnomos::TokenRequest token_request = client.genToken(
        {w1_keyword, spec.w2_keyword}, gatekeeper.getUpdateCounts());
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

    result.client_times[point] =
        durationToMilliseconds(client_gen_end - client_gen_start) +
        durationToMilliseconds(prepare_end - prepare_start) +
        durationToMilliseconds(verify_end - verify_start);
    result.gatekeeper_times[point] =
        durationToMilliseconds(gatekeeper_end - gatekeeper_start);
    result.server_times[point] =
        durationToMilliseconds(server_end - server_start);
  }

  return result;
}

}  // namespace benchmark
}  // namespace nomos
