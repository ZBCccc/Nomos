#ifndef NOMOS_DATASET_LOADER_HPP
#define NOMOS_DATASET_LOADER_HPP

#include <map>
#include <random>
#include <string>
#include <vector>

namespace nomos {
namespace benchmark {

/**
 * @brief Dataset loader for experimental evaluation
 *
 * Loads keyword frequency distributions from real datasets
 * and generates keywords according to Zipfian distribution.
 *
 * Supported datasets:
 * - Crime: 63,660 keywords, max freq 16,644 (highly skewed)
 * - Enron: 16,243 keywords, max freq 26,946 (highly skewed)
 * - Wiki: 10,001 keywords, max freq 9,738 (more uniform)
 */
class DatasetLoader {
 public:
  /**
   * @brief Supported datasets
   */
  enum class Dataset {
    None,   // Uniform random keywords (default)
    Crime,  // Crime dataset
    Enron,  // Enron email dataset
    Wiki    // Wikipedia dataset
  };

  /**
   * @brief Construct a DatasetLoader
   * @param dataset Dataset to load
   */
  explicit DatasetLoader(Dataset dataset = Dataset::None);

  /**
   * @brief Load dataset from JSON file
   * @param base_path Base path for dataset files
   * @return true if loaded successfully
   */
  bool load(const std::string& base_path = "");

  /**
   * @brief Generate keywords according to the dataset distribution
   * @param count Number of keywords to generate
   * @param seed Random seed for reproducibility
   * @return Vector of keyword strings
   */
  std::vector<std::string> generateKeywords(size_t count,
                                            unsigned int seed = 42);

  /**
   * @brief Generate file IDs for updates
   * @param count Number of file IDs to generate
   * @param seed Random seed for reproducibility
   * @return Vector of file ID strings
   */
  std::vector<std::string> generateFileIds(size_t count,
                                           unsigned int seed = 42);

  /**
   * @brief Get dataset name as string
   * @return Dataset name
   */
  std::string getDatasetName() const;

  /**
   * @brief Get number of unique keywords in dataset
   * @return Keyword count
   */
  size_t getKeywordCount() const { return keywords_.size(); }

  /**
   * @brief Get total document count (sum of all frequencies)
   * @return Total count
   */
  size_t getTotalDocuments() const { return total_docs_; }

  /**
   * @brief Get one representative keyword for each unique frequency
   * @return Frequency -> keyword map ordered by ascending frequency
   */
  std::map<size_t, std::string> getRepresentativeKeywordsByFrequency() const;

  /**
   * @brief Get raw keyword frequencies from dataset in file order
   * @return All keyword frequencies (can contain duplicates)
   */
  const std::vector<size_t>& getAllKeywordFrequencies() const {
    return frequencies_;
  }

  /**
   * @brief Get raw keywords from dataset in file order
   * @return All keywords
   */
  const std::vector<std::string>& getAllKeywords() const { return keywords_; }

 private:
  Dataset dataset_;
  std::vector<std::string> keywords_;
  std::vector<size_t> frequencies_;
  size_t total_docs_;
  bool loaded_;

  /**
   * @brief Parse JSON file and extract keyword frequencies
   */
  bool parseJsonFile(const std::string& filepath);

  /**
   * @brief Get keyword by Zipfian distribution
   */
  size_t sampleZipfian(std::mt19937& rng);
};

/**
 * @brief Convert string to Dataset enum
 */
DatasetLoader::Dataset stringToDataset(const std::string& name);

/**
 * @brief Convert Dataset enum to string
 */
std::string datasetToString(DatasetLoader::Dataset dataset);

}  // namespace benchmark
}  // namespace nomos

#endif  // NOMOS_DATASET_LOADER_HPP
