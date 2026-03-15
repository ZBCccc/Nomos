#include "mc-odxt/McOdxtServer.hpp"

#include <algorithm>

#include "core/Primitive.hpp"

namespace mcodxt {

McOdxtServer::McOdxtServer() {}

McOdxtServer::~McOdxtServer() {
  m_TSet.clear();
  m_XSet.clear();
}

void McOdxtServer::setup(const std::vector<uint8_t>& /*Km*/) {}

void McOdxtServer::update(const UpdateMetadata& meta) {
  const std::string addr_key = SerializePoint(meta.addr);

  TSetEntry entry;
  entry.val = meta.val;
  bn_new(entry.alpha);
  bn_copy(entry.alpha, meta.alpha);
  m_TSet[addr_key] = std::move(entry);

  if (!meta.xtag.empty()) {
    m_XSet[meta.xtag] = true;
  }
}

std::vector<SearchResultEntry> McOdxtServer::search(
    const McOdxtClient::SearchRequest& req) {
  std::vector<SearchResultEntry> results;

  const int m = static_cast<int>(req.stokenList.size());
  const int n = req.num_keywords;

  for (int j = 0; j < m; ++j) {
    const std::string& stag_key = req.stokenList[j];
    std::map<std::string, TSetEntry>::const_iterator it = m_TSet.find(stag_key);
    if (it == m_TSet.end()) {
      continue;
    }

    const TSetEntry& entry = it->second;
    bool all_match = true;
    int match_count = 0;

    if (n > 1) {
      for (int i = 0; i < n - 1; ++i) {
        if (j >= static_cast<int>(req.xtokenList.size()) ||
            i >= static_cast<int>(req.xtokenList[j].size())) {
          all_match = false;
          break;
        }

        const std::vector<std::string>& xtokens = req.xtokenList[j][i];
        bool keyword_match = false;

        for (size_t t = 0; t < xtokens.size(); ++t) {
          ep_t xtoken;
          ep_new(xtoken);
          DeserializePoint(xtoken, xtokens[t]);

          ep_t xtag;
          ep_new(xtag);
          ep_mul(xtag, xtoken, entry.alpha);

          const std::string xtag_key = SerializePoint(xtag);
          if (m_XSet.find(xtag_key) != m_XSet.end()) {
            keyword_match = true;
            match_count++;
          }

          ep_free(xtag);
          ep_free(xtoken);

          if (keyword_match) {
            break;
          }
        }

        if (!keyword_match) {
          all_match = false;
          break;
        }
      }
    }

    if (all_match) {
      SearchResultEntry result;
      result.j = j + 1;
      result.sval = entry.val;
      result.cnt = match_count;
      results.push_back(result);
    }
  }

  return results;
}

}  // namespace mcodxt
