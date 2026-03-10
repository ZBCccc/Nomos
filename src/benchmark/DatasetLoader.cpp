#include "benchmark/DatasetLoader.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace nomos {
namespace benchmark {

DatasetLoader::DatasetLoader(Dataset dataset)
    : dataset_(dataset), total_docs_(0), loaded_(false) {}

bool DatasetLoader::load(const std::string& base_path) {
    if (dataset_ == Dataset::None) {
        std::cout << "[DatasetLoader] No dataset selected (using uniform distribution)"
                  << std::endl;
        return true;
    }

    std::string dataset_name = getDatasetName();
    std::string filepath = base_path + "/" + dataset_name + "_filecnt_sorted.json";

    // Try default path relative to paper directory
    if (base_path.empty()) {
        filepath = "/Users/cyan/code/paper/pic/raw_data/" + dataset_name + "_filecnt_sorted.json";
    }

    std::cout << "[DatasetLoader] Loading dataset: " << dataset_name << std::endl;
    std::cout << "[DatasetLoader] File: " << filepath << std::endl;

    return parseJsonFile(filepath);
}

bool DatasetLoader::parseJsonFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "[DatasetLoader] Failed to open file: " << filepath << std::endl;
        return false;
    }

    // Read entire file content
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();

    // Simple JSON parsing (keyword: count pairs)
    // Format: { "F123": 100, "F456": 200, ... }
    size_t pos = 0;
    size_t brace_count = 0;

    // Skip opening brace
    while (pos < content.size() && content[pos] != '{') pos++;
    if (pos >= content.size()) return false;
    pos++;  // skip '{'

    while (pos < content.size()) {
        // Skip whitespace
        while (pos < content.size() && (content[pos] == ' ' || content[pos] == '\n' ||
                                        content[pos] == '\r' || content[pos] == '\t')) pos++;

        if (pos >= content.size()) break;

        // Check for closing brace
        if (content[pos] == '}') break;

        // Parse keyword (string)
        if (content[pos] != '"') {
            pos++;
            continue;
        }
        pos++;  // skip opening quote

        // Extract keyword
        size_t end_pos = content.find('"', pos);
        if (end_pos == std::string::npos) break;
        std::string keyword = content.substr(pos, end_pos - pos);
        pos = end_pos + 1;

        // Skip whitespace
        while (pos < content.size() && (content[pos] == ' ' || content[pos] == '\n' ||
                                        content[pos] == '\r' || content[pos] == '\t')) pos++;

        // Skip colon
        if (pos < content.size() && content[pos] == ':') pos++;

        // Skip whitespace
        while (pos < content.size() && (content[pos] == ' ' || content[pos] == '\n' ||
                                        content[pos] == '\r' || content[pos] == '\t')) pos++;

        // Parse frequency (number)
        size_t freq_start = pos;
        while (pos < content.size() && (isdigit(content[pos]) || content[pos] == '-')) pos++;
        std::string freq_str = content.substr(freq_start, pos - freq_start);

        try {
            size_t freq = std::stoul(freq_str);
            keywords_.push_back(keyword);
            frequencies_.push_back(freq);
            total_docs_ += freq;
        } catch (...) {
            // Skip invalid entries
        }

        // Skip comma and whitespace
        while (pos < content.size() && (content[pos] == ',' || content[pos] == ' ' ||
                                        content[pos] == '\n' || content[pos] == '\r')) pos++;
    }

    loaded_ = true;
    std::cout << "[DatasetLoader] Loaded " << keywords_.size() << " keywords, "
              << total_docs_ << " total documents" << std::endl;

    return true;
}

std::vector<std::string> DatasetLoader::generateKeywords(size_t count, unsigned int seed) {
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

std::vector<std::string> DatasetLoader::generateFileIds(size_t count, unsigned int seed) {
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

DatasetLoader::Dataset stringToDataset(const std::string& name) {
    if (name == "Crime" || name == "crime") return DatasetLoader::Dataset::Crime;
    if (name == "Enron" || name == "enron") return DatasetLoader::Dataset::Enron;
    if (name == "Wiki" || name == "wiki") return DatasetLoader::Dataset::Wiki;
    return DatasetLoader::Dataset::None;
}

std::string datasetToString(DatasetLoader::Dataset dataset) {
    switch (dataset) {
        case DatasetLoader::Dataset::Crime: return "Crime";
        case DatasetLoader::Dataset::Enron: return "Enron";
        case DatasetLoader::Dataset::Wiki: return "Wiki";
        default: return "None";
    }
}

}  // namespace benchmark
}  // namespace nomos
