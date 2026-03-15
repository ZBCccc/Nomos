#pragma once

#include <memory>
#include <string>
#include <vector>

namespace verifiable {

class MerkleOpenTree {
 public:
  explicit MerkleOpenTree(const std::vector<std::string>& xtags);
  ~MerkleOpenTree();

  std::string getRootHash() const;
  std::vector<std::string> generateProof(int leaf_index) const;

  static bool VerifyPath(const std::string& root_hash, int leaf_index,
                         const std::string& xtag,
                         const std::vector<std::string>& proof, int leaf_count);

 private:
  struct Node {
    std::string hash;
    std::unique_ptr<Node> left;
    std::unique_ptr<Node> right;
    bool is_leaf;
  };

  void build(const std::vector<std::string>& xtags);
  std::string hashLeaf(int leaf_index, const std::string& xtag) const;
  std::string hashInternal(const std::string& left,
                           const std::string& right) const;

  std::unique_ptr<Node> m_root;
  int m_leaf_count;
  int m_capacity;
};

}  // namespace verifiable
