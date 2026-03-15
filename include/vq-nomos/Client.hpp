#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "vq-nomos/types.hpp"

namespace vqnomos {

class Client {
 public:
  Client();
  ~Client();

  int setup(const std::string& public_key_pem, const Anchor& initial_anchor,
            size_t qtree_capacity = 1024, int bucket_count = 10);

  TokenRequest genToken(
      const std::vector<std::string>& query_keywords,
      const std::unordered_map<std::string, int>& update_count);

  SearchRequest prepareSearch(const SearchToken& token,
                              const TokenRequest& token_request);

  VerificationResult decryptAndVerify(const SearchResponse& response,
                                      const SearchToken& token,
                                      const TokenRequest& token_request);

 private:
  struct DecryptedEntry {
    std::string id;
    int op;
  };

  bool decryptEntry(DecryptedEntry* output, const CandidateEntry& entry,
                    const SearchToken& token) const;

  std::string m_public_key_pem;
  Anchor m_local_anchor;
  size_t m_qtree_capacity;
  int m_bucket_count;
};

}  // namespace vqnomos
