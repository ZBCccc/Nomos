#include "verifiable/MerkleOpen.hpp"

#include <stdexcept>
#include <vector>

#include "vq-nomos/Common.hpp"

namespace verifiable {

MerkleOpenTree::MerkleOpenTree(const std::vector<std::string>& xtags)
    : m_leaf_count(static_cast<int>(xtags.size())), m_capacity(1) {
  if (xtags.empty()) {
    throw std::invalid_argument("MerkleOpenTree requires at least one leaf");
  }
  while (m_capacity < m_leaf_count) {
    m_capacity *= 2;
  }
  build(xtags);
}

MerkleOpenTree::~MerkleOpenTree() {}

std::string MerkleOpenTree::getRootHash() const {
  if (!m_root) {
    return "";
  }
  return m_root->hash;
}

std::vector<std::string> MerkleOpenTree::generateProof(int leaf_index) const {
  if (leaf_index < 1 || leaf_index > m_leaf_count || !m_root) {
    throw std::out_of_range("MerkleOpenTree leaf index out of range");
  }

  std::vector<std::string> proof;
  int zero_index = leaf_index - 1;
  const Node* current = m_root.get();
  int level_size = m_capacity;

  while (!current->is_leaf) {
    level_size /= 2;
    if (zero_index < level_size) {
      proof.push_back(current->right->hash);
      current = current->left.get();
    } else {
      proof.push_back(current->left->hash);
      current = current->right.get();
      zero_index -= level_size;
    }
  }

  return proof;
}

bool MerkleOpenTree::VerifyPath(const std::string& root_hash, int leaf_index,
                                const std::string& xtag,
                                const std::vector<std::string>& proof,
                                int leaf_count) {
  if (leaf_index < 1 || leaf_count <= 0 || leaf_index > leaf_count) {
    return false;
  }

  int capacity = 1;
  while (capacity < leaf_count) {
    capacity *= 2;
  }

  int zero_index = leaf_index - 1;
  std::vector<bool> went_right(proof.size(), false);
  int level_size = capacity;
  for (size_t i = 0; i < proof.size(); ++i) {
    level_size /= 2;
    if (zero_index >= level_size) {
      went_right[i] = true;
      zero_index -= level_size;
    }
  }

  std::string current_hash;
  current_hash.push_back(static_cast<char>(0));
  current_hash.append(vqnomos::EncodeUint32(static_cast<uint32_t>(leaf_index)));
  current_hash.append(xtag);
  current_hash = vqnomos::Sha256Binary(current_hash);

  for (int i = static_cast<int>(proof.size()) - 1; i >= 0; --i) {
    std::string input;
    input.push_back(static_cast<char>(1));
    if (went_right[static_cast<size_t>(i)]) {
      input.append(proof[static_cast<size_t>(i)]);
      input.append(current_hash);
    } else {
      input.append(current_hash);
      input.append(proof[static_cast<size_t>(i)]);
    }
    current_hash = vqnomos::Sha256Binary(input);
  }

  return current_hash == root_hash;
}

void MerkleOpenTree::build(const std::vector<std::string>& xtags) {
  std::vector<std::unique_ptr<Node>> current_level;
  current_level.reserve(static_cast<size_t>(m_capacity));

  for (int i = 1; i <= m_capacity; ++i) {
    std::unique_ptr<Node> leaf(new Node());
    leaf->is_leaf = true;
    if (i <= m_leaf_count) {
      leaf->hash = hashLeaf(i, xtags[static_cast<size_t>(i - 1)]);
    } else {
      leaf->hash = hashLeaf(i, std::string());
    }
    current_level.push_back(std::move(leaf));
  }

  while (current_level.size() > 1) {
    std::vector<std::unique_ptr<Node>> next_level;
    next_level.reserve(current_level.size() / 2);

    for (size_t i = 0; i < current_level.size(); i += 2) {
      std::unique_ptr<Node> node(new Node());
      node->is_leaf = false;
      node->left = std::move(current_level[i]);
      node->right = std::move(current_level[i + 1]);
      node->hash = hashInternal(node->left->hash, node->right->hash);
      next_level.push_back(std::move(node));
    }

    current_level = std::move(next_level);
  }

  m_root = std::move(current_level[0]);
}

std::string MerkleOpenTree::hashLeaf(int leaf_index,
                                     const std::string& xtag) const {
  std::string input;
  input.push_back(static_cast<char>(0));
  input.append(vqnomos::EncodeUint32(static_cast<uint32_t>(leaf_index)));
  input.append(xtag);
  return vqnomos::Sha256Binary(input);
}

std::string MerkleOpenTree::hashInternal(const std::string& left,
                                         const std::string& right) const {
  std::string input;
  input.push_back(static_cast<char>(1));
  input.append(left);
  input.append(right);
  return vqnomos::Sha256Binary(input);
}

}  // namespace verifiable
