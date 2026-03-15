#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "mc-odxt/McOdxtTypes.hpp"

namespace mcodxt {

class McOdxtClient {
 public:
  McOdxtClient();
  ~McOdxtClient();

  int setup();

  TokenRequest genToken(const std::vector<std::string>& query_keywords,
                        const std::unordered_map<std::string, int>& updateCnt);

  struct SearchRequest {
    int num_keywords;
    std::vector<std::string> stokenList;
    std::vector<std::vector<std::vector<std::string>>> xtokenList;

    SearchRequest() : num_keywords(0) {}
  };

  SearchRequest prepareSearch(const SearchToken& token,
                              const TokenRequest& token_request);

  std::vector<std::string> decryptResults(
      const std::vector<SearchResultEntry>& results, const SearchToken& token);
};

}  // namespace mcodxt
