#include "benchmark/DatasetLoader.hpp"

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace nomos {
namespace benchmark {

DatasetLoader::DatasetLoader(Dataset dataset)
    : dataset_(dataset), total_docs_(0), loaded_(false) {}

bool DatasetLoader::load(const std::string& base_path) {
  if (dataset_ == Dataset::None) {
    std::cout
        << "[DatasetLoader] No dataset selected (using uniform distribution)"
        << std::endl;
    return true;
  }

  const std::string dataset_name = getDatasetName();
  const std::string filename = dataset_name + "_filecnt_sorted.json";

  // If caller gives base_path, use it directly (supports relative and
  // absolute).
  if (!base_path.empty()) {
    const std::string filepath = base_path + "/" + filename;
    std::cout << "[DatasetLoader] Loading dataset: " << dataset_name
              << std::endl;
    std::cout << "[DatasetLoader] File: " << filepath << std::endl;
    return parseJsonFile(filepath);
  }

  std::cout << "[DatasetLoader] Loading dataset: " << dataset_name << std::endl;

  // Default resolution order (prefer repository-relative paths):
  // 1) NOMOS_DATA_DIR env override
  // 2) pic/raw_data (run from repo root)
  // 3) ../pic/raw_data (run from build/)
  // 4) ../../pic/raw_data (run from nested build dirs)
  std::vector<std::string> candidate_dirs;
  const char* env_dir = std::getenv("NOMOS_DATA_DIR");
  if (env_dir != nullptr && env_dir[0] != '\0') {
    candidate_dirs.push_back(std::string(env_dir));
  }
  candidate_dirs.push_back("pic/raw_data");
  candidate_dirs.push_back("../pic/raw_data");
  candidate_dirs.push_back("../../pic/raw_data");

  for (size_t i = 0; i < candidate_dirs.size(); ++i) {
    const std::string filepath = candidate_dirs[i] + "/" + filename;
    std::cout << "[DatasetLoader] Trying: " << filepath << std::endl;
    if (parseJsonFile(filepath)) {
      return true;
    }
  }

  std::cerr << "[DatasetLoader] Could not locate dataset file for "
            << dataset_name
            << ". You can set NOMOS_DATA_DIR or pass an explicit base path."
            << std::endl;
  return false;
}

bool DatasetLoader::parseJsonFile(const std::string& filepath) {
  std::ifstream file(filepath);
  if (!file.is_open()) {
    std::cerr << "[DatasetLoader] Failed to open file: " << filepath
              << std::endl;
    return false;
  }

  try {
    nlohmann::json j;
    file >> j;

    for (auto it = j.begin(); it != j.end(); ++it) {
      std::string keyword = it.key();
      size_t freq = it.value().get<size_t>();

      keywords_.push_back(keyword);
      frequencies_.push_back(freq);
      total_docs_ += freq;
    }
  } catch (const std::exception& e) {
    std::cerr << "[DatasetLoader] JSON parsing error: " << e.what()
              << std::endl;
    return false;
  }

  loaded_ = true;
  std::cout << "[DatasetLoader] Loaded " << keywords_.size() << " keywords, "
            << total_docs_ << " total documents" << std::endl;

  return true;
}

std::vector<std::string> DatasetLoader::generateKeywords(size_t count,
                                                         unsigned int seed) {
  std::vector<std::string> result;
  result.reserve(count);

  if (!loaded_ || dataset_ == Dataset::None) {
    // Uniform random keywords
    std::mt19937 rng(seed);
    std::uniform_int_distribution<size_t> dist(0, 999);
    for (size_t i = 0; i < count; ++i) {
      result.push_back("keyword_" + std::to_string(dist(rng)));
    }
    return result;
  }

  // Zipfian distribution
  std::mt19937 rng(seed);
  for (size_t i = 0; i < count; ++i) {
    size_t idx = sampleZipfian(rng);
    result.push_back(keywords_[idx]);
  }

  return result;
}

std::vector<std::string> DatasetLoader::generateFileIds(size_t count,
                                                        unsigned int seed) {
  std::vector<std::string> result;
  result.reserve(count);

  std::mt19937 rng(seed);
  std::uniform_int_distribution<size_t> dist(0, 999999);

  for (size_t i = 0; i < count; ++i) {
    result.push_back("file_" + std::to_string(dist(rng)));
  }

  return result;
}

size_t DatasetLoader::sampleZipfian(std::mt19937& rng) {
  // Zipfian distribution: P(i) ~ 1/(i+1)^s where s is the skew parameter
  // Using s = 1.2 for realistic keyword distribution
  const double s = 1.2;

  std::uniform_real_distribution<double> dist(0.0, 1.0);
  double r = dist(rng);

  // Inverse transform sampling
  // CDF: F(i) = sum_{j=0}^{i} 1/(j+1)^s / H_n
  // where H_n is the nth harmonic number

  size_t n = keywords_.size();
  double harmonic = 0.0;
  for (size_t i = 1; i <= n; ++i) {
    harmonic += 1.0 / std::pow(i, s);
  }

  // Find index
  double cumulative = 0.0;
  for (size_t i = 0; i < n; ++i) {
    cumulative += 1.0 / std::pow(i + 1, s) / harmonic;
    if (r <= cumulative) {
      return i;
    }
  }

  return n - 1;  // Fallback to last element
}

std::string DatasetLoader::getDatasetName() const {
  return datasetToString(dataset_);
}

std::map<size_t, std::string>
DatasetLoader::getRepresentativeKeywordsByFrequency() const {
  std::map<size_t, std::string> representatives;

  for (size_t i = 0; i < keywords_.size() && i < frequencies_.size(); ++i) {
    if (representatives.find(frequencies_[i]) == representatives.end()) {
      representatives[frequencies_[i]] = keywords_[i];
    }
  }

  return representatives;
}

DatasetLoader::Dataset stringToDataset(const std::string& name) {
  if (name == "Crime" || name == "crime") return DatasetLoader::Dataset::Crime;
  if (name == "Enron" || name == "enron") return DatasetLoader::Dataset::Enron;
  if (name == "Wiki" || name == "wiki") return DatasetLoader::Dataset::Wiki;
  return DatasetLoader::Dataset::None;
}

std::string datasetToString(DatasetLoader::Dataset dataset) {
  switch (dataset) {
    case DatasetLoader::Dataset::Crime:
      return "Crime";
    case DatasetLoader::Dataset::Enron:
      return "Enron";
    case DatasetLoader::Dataset::Wiki:
      return "Wiki";
    default:
      return "None";
  }
}

}  // namespace benchmark
}  // namespace nomos
