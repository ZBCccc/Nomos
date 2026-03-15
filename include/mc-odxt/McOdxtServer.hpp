#pragma once

#include <map>
#include <string>
#include <vector>

#include "mc-odxt/McOdxtClient.hpp"
#include "mc-odxt/McOdxtTypes.hpp"

namespace mcodxt {

class McOdxtServer {
 public:
  McOdxtServer();
  ~McOdxtServer();

  void setup(const std::vector<uint8_t>& Km);
  void update(const UpdateMetadata& meta);
  std::vector<SearchResultEntry> search(const McOdxtClient::SearchRequest& req);

  size_t getTSetSize() const { return m_TSet.size(); }
  size_t getXSetSize() const { return m_XSet.size(); }

 private:
  std::map<std::string, TSetEntry> m_TSet;
  std::map<std::string, bool> m_XSet;

  std::string serializePoint(const ep_t point) const;
};

}  // namespace mcodxt
