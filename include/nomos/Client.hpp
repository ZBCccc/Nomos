#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Gatekeeper.hpp"
#include "types.hpp"

extern "C" {
#include <relic/relic.h>
}

namespace nomos {

/**
 * @brief Nomos Client implementation for the simplified experimental path
 */
class Client {
 public:
  Client();
  ~Client();

  int setup();

  /**
   * @brief Generate search token for experiments
   *
   * Gatekeeper directly computes all tokens without OPRF blinding.
   *
   * @param query_keywords Query keywords [w1, ..., wn]
   * @param gatekeeper Reference to gatekeeper
   * @return SearchToken with pre-computed tokens
   */
  SearchToken genTokenSimplified(const std::vector<std::string>& query_keywords,
                                 Gatekeeper& gatekeeper);

  /**
   * @brief Search - Algorithm 4 (Client side)
   * Derive search request values from the token.
   */
  struct SearchRequest {
    int num_keywords;
    std::vector<std::string> stokenList;
    std::vector<std::vector<std::vector<std::string>>> xtokenList;

    SearchRequest() : num_keywords(0) {}
    ~SearchRequest() {}
  };

  SearchRequest prepareSearch(
      const SearchToken& token, const std::vector<std::string>& query_keywords,
      const std::unordered_map<std::string, int>& updateCnt);

  /**
   * @brief Decrypt search results
   */
  std::vector<std::string> decryptResults(
      const std::vector<SearchResultEntry>& results, const SearchToken& token);
};

}  // namespace nomos
