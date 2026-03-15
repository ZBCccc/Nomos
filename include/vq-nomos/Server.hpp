#pragma once

#include <map>
#include <memory>
#include <string>

#include "vq-nomos/MerkleOpen.hpp"
#include "vq-nomos/QTree.hpp"
#include "vq-nomos/types.hpp"

namespace vqnomos {

class Server {
 public:
  Server();
  ~Server();

  void setup(const std::vector<uint8_t>& Km, const Anchor& initial_anchor,
             size_t qtree_capacity = 1024);

  void update(const UpdateMetadata& metadata);

  SearchResponse search(const SearchRequest& request, const SearchToken& token);

  size_t getTSetSize() const { return m_TSet.size(); }
  size_t getXSetSize() const { return m_XSet.size(); }

 private:
  struct MerklePosition {
    int leaf_index;
    std::string root_hash;
    std::string signature;

    MerklePosition() : leaf_index(0) {}
  };

  std::string serializePoint(const ep_t point) const;

  std::map<std::string, TSetEntry> m_TSet;
  std::map<std::string, bool> m_XSet;
  std::map<std::string, MerklePosition> m_MPos;
  std::map<std::string, std::shared_ptr<MerkleOpenTree>> m_MTree;
  std::unique_ptr<QTree> m_qtree;
  Anchor m_current_anchor;
};

}  // namespace vqnomos
