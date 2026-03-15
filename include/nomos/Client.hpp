#pragma once

#include <string>
#include <unordered_map>
#include <vector>

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
   * @brief Generate the client-side Nomos GenToken request for experiments
   *
   * Reorders the query by update count and computes the hash points that the
   * gatekeeper will transform with the master keys.
   *
   * @param query_keywords Query keywords [w1, ..., wn]
   * @param updateCnt Gatekeeper update counters
   * @return TokenRequest to send to Gatekeeper::genToken
   */
  TokenRequest genToken(const std::vector<std::string>& query_keywords,
                        const std::unordered_map<std::string, int>& updateCnt);

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

  SearchRequest prepareSearch(const SearchToken& token,
                              const TokenRequest& token_request);

  /**
   * @brief Decrypt search results
   */
  std::vector<std::string> decryptResults(
      const std::vector<SearchResultEntry>& results, const SearchToken& token);
};

}  // namespace nomos
