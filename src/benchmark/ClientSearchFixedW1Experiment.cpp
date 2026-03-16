#include "benchmark/ClientSearchFixedW1Experiment.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
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

const size_t kFixedUpdW1 = 10;
const size_t kQTreeCapacity = 1024;

}  // namespace

ClientSearchFixedW1Experiment::ClientSearchFixedW1Experiment()
    : dataset_(DatasetLoader::Dataset::None),
      run_all_datasets_(true),
      output_dir_("results/ch4/"),
      scheme_filter_("all") {}

int ClientSearchFixedW1Experiment::setup() {
  std::cout << "[ClientSearchFixedW1] Setting up..." << std::endl;
  ensureDirectory(joinPath(output_dir_, "client_search_time_fixed_w1"));
  ensureDirectory(joinPath(output_dir_, "server_search_time_fixed_w1"));
  ensureDirectory(joinPath(output_dir_, "gatekeeper_search_time_fixed_w1"));
  return 0;
}

void ClientSearchFixedW1Experiment::run() {
  std::cout << "[ClientSearchFixedW1] Running Chapter 4 search benchmark"
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
    std::cout << "\n[ClientSearchFixedW1] Dataset " << (i + 1) << "/"
              << specs.size() << ": " << datasetToString(spec.dataset) << " ("
              << spec.upd_w2_values.size() << " points)" << std::endl;
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

void ClientSearchFixedW1Experiment::setOutputDir(
    const std::string& output_dir) {
  if (!output_dir.empty()) {
    output_dir_ = output_dir;
  }
}

void ClientSearchFixedW1Experiment::setSchemeFilter(
    const std::string& scheme_filter) {
  scheme_filter_ = normalizeSchemeFilter(scheme_filter);
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

bool ClientSearchFixedW1Experiment::shouldRunScheme(
    const std::string& scheme_name) const {
  return scheme_filter_ == "all" ||
         normalizeSchemeFilter(scheme_name) == scheme_filter_;
}

size_t ClientSearchFixedW1Experiment::getEnabledSchemeCount() const {
  return (scheme_filter_ == "all") ? static_cast<size_t>(3)
                                   : static_cast<size_t>(1);
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
  spec.upd_w1_fixed = kFixedUpdW1;

  // Sweep all representative keywords as w2, each with its own keyword string.
  for (std::map<size_t, std::string>::const_iterator it =
           representatives.begin();
       it != representatives.end(); ++it) {
    spec.upd_w2_values.push_back(std::make_pair(it->first, it->second));
  }

  if (spec.upd_w2_values.empty()) {
    throw std::runtime_error("Dataset has no keyword frequencies: " +
                             datasetToString(dataset));
  }

  return spec;
}

void ClientSearchFixedW1Experiment::runDataset(const DatasetSpec& spec) const {
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

  nomos_rows.reserve(spec.upd_w2_values.size());
  mcodxt_rows.reserve(spec.upd_w2_values.size());
  vqnomos_rows.reserve(spec.upd_w2_values.size());

  for (size_t i = 0; i < spec.upd_w2_values.size(); ++i) {
    const size_t upd_w2 = spec.upd_w2_values[i].first;

    if (!nomos_times.client_times.empty()) {
      ClientSearchRow nomos_row;
      nomos_row.dataset = datasetToString(spec.dataset);
      nomos_row.scheme = "Nomos";
      nomos_row.upd_w1 = kFixedUpdW1;
      nomos_row.upd_w2 = upd_w2;
      nomos_row.client_time_ms = nomos_times.client_times[i];
      nomos_row.server_time_ms = nomos_times.server_times[i];
      nomos_row.gatekeeper_time_ms = nomos_times.gatekeeper_times[i];
      nomos_rows.push_back(nomos_row);
    }

    if (!mcodxt_times.client_times.empty()) {
      ClientSearchRow mcodxt_row;
      mcodxt_row.dataset = datasetToString(spec.dataset);
      mcodxt_row.scheme = "MC-ODXT";
      mcodxt_row.upd_w1 = kFixedUpdW1;
      mcodxt_row.upd_w2 = upd_w2;
      mcodxt_row.client_time_ms = mcodxt_times.client_times[i];
      mcodxt_row.server_time_ms = mcodxt_times.server_times[i];
      mcodxt_row.gatekeeper_time_ms = mcodxt_times.gatekeeper_times[i];
      mcodxt_rows.push_back(mcodxt_row);
    }

    if (!vqnomos_times.client_times.empty()) {
      ClientSearchRow vqnomos_row;
      vqnomos_row.dataset = datasetToString(spec.dataset);
      vqnomos_row.scheme = "VQNomos";
      vqnomos_row.upd_w1 = kFixedUpdW1;
      vqnomos_row.upd_w2 = upd_w2;
      vqnomos_row.client_time_ms = vqnomos_times.client_times[i];
      vqnomos_row.server_time_ms = vqnomos_times.server_times[i];
      vqnomos_row.gatekeeper_time_ms = vqnomos_times.gatekeeper_times[i];
      vqnomos_rows.push_back(vqnomos_row);
    }
  }

  const std::vector<std::pair<std::string, std::string>> outputs = {
      std::make_pair(joinPath(output_dir_, "client_search_time_fixed_w1"),
                     "client_time_ms"),
      std::make_pair(joinPath(output_dir_, "server_search_time_fixed_w1"),
                     "server_time_ms"),
      std::make_pair(joinPath(output_dir_, "gatekeeper_search_time_fixed_w1"),
                     "gatekeeper_time_ms")};

  for (size_t d = 0; d < outputs.size(); ++d) {
    writeSchemeCsv(outputs[d].first, "Nomos", nomos_rows, outputs[d].second);
    writeSchemeCsv(outputs[d].first, "MC-ODXT", mcodxt_rows, outputs[d].second);
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

ClientSearchFixedW1Experiment::SweepResult
ClientSearchFixedW1Experiment::runNomosSweep(const DatasetSpec& spec) const {
  SweepResult result;
  result.client_times.resize(spec.upd_w2_values.size(), 0.0);
  result.gatekeeper_times.resize(spec.upd_w2_values.size(), 0.0);
  result.server_times.resize(spec.upd_w2_values.size(), 0.0);

  const std::vector<std::string> w1_doc_ids = makeSharedDocIds(kFixedUpdW1);

  Gatekeeper gatekeeper;
  Client client;
  Server server;

  gatekeeper.setup(10);
  client.setup();
  server.setup(gatekeeper.getKm());

  for (size_t point = 0; point < spec.upd_w2_values.size(); ++point) {
    const size_t target_w2_count = spec.upd_w2_values[point].first;
    const std::string w2_keyword = spec.upd_w2_values[point].second;
    const std::vector<std::string> query = {spec.w1_keyword, w2_keyword};

    for (size_t next = 0; next < target_w2_count; ++next) {
      const std::string doc_id = "doc_" + std::to_string(next + 1);
      const UpdateMetadata meta = gatekeeper.update(OP_ADD, doc_id, w2_keyword);
      server.update(meta);
    }
  }

  // 执行search操作
  for (size_t point = 0; point < spec.upd_w2_values.size(); ++point) {
    const std::string w2_keyword = spec.upd_w2_values[point].second;
    const std::vector<std::string> query = {spec.w1_keyword, w2_keyword};

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

ClientSearchFixedW1Experiment::SweepResult
ClientSearchFixedW1Experiment::runMcOdxtSweep(const DatasetSpec& spec) const {
  SweepResult result;
  result.client_times.resize(spec.upd_w2_values.size(), 0.0);
  result.gatekeeper_times.resize(spec.upd_w2_values.size(), 0.0);
  result.server_times.resize(spec.upd_w2_values.size(), 0.0);

  const std::vector<std::string> w1_doc_ids = makeSharedDocIds(kFixedUpdW1);

  mcodxt::McOdxtGatekeeper gatekeeper;
  mcodxt::McOdxtServer server;
  mcodxt::McOdxtClient client;

  gatekeeper.setup(10);
  client.setup();
  server.setup(gatekeeper.getKm());

  for (size_t point = 0; point < spec.upd_w2_values.size(); ++point) {
    const size_t target_w2_count = spec.upd_w2_values[point].first;
    const std::string w2_keyword = spec.upd_w2_values[point].second;

    for (size_t next = 0; next < target_w2_count; ++next) {
      const std::string doc_id = "doc_" + std::to_string(next + 1);
      mcodxt::UpdateMetadata meta =
          gatekeeper.update(mcodxt::OpType::ADD, doc_id, w2_keyword);
      server.update(meta);
    }
  }

  // 执行search操作
  for (size_t point = 0; point < spec.upd_w2_values.size(); ++point) {
    const std::string w2_keyword = spec.upd_w2_values[point].second;
    const std::vector<std::string> query = {spec.w1_keyword, w2_keyword};

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

ClientSearchFixedW1Experiment::SweepResult
ClientSearchFixedW1Experiment::runVQNomosSweep(const DatasetSpec& spec) const {
  SweepResult result;
  result.client_times.resize(spec.upd_w2_values.size(), 0.0);
  result.gatekeeper_times.resize(spec.upd_w2_values.size(), 0.0);
  result.server_times.resize(spec.upd_w2_values.size(), 0.0);

  const std::vector<std::string> w1_doc_ids = makeSharedDocIds(kFixedUpdW1);

  vqnomos::Gatekeeper gatekeeper;
  vqnomos::Client client;
  vqnomos::Server server;

  gatekeeper.setup(10, kQTreeCapacity);
  const vqnomos::Anchor initial_anchor = gatekeeper.getCurrentAnchor();
  client.setup(gatekeeper.getPublicKeyPem(), initial_anchor, kQTreeCapacity,
               10);
  server.setup(gatekeeper.getKm(), initial_anchor, kQTreeCapacity);

  for (size_t point = 0; point < spec.upd_w2_values.size(); ++point) {
    const size_t target_w2_count = spec.upd_w2_values[point].first;
    const std::string w2_keyword = spec.upd_w2_values[point].second;

    for (size_t next = 0; next < target_w2_count; ++next) {
      const std::string doc_id = "doc_" + std::to_string(next + 1);
      const vqnomos::UpdateMetadata meta =
          gatekeeper.update(vqnomos::OP_ADD, doc_id, w2_keyword);
      server.update(meta);
    }
  }

  // 执行search操作
  for (size_t point = 0; point < spec.upd_w2_values.size(); ++point) {
    const std::string w2_keyword = spec.upd_w2_values[point].second;
    const std::vector<std::string> query = {spec.w1_keyword, w2_keyword};

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
