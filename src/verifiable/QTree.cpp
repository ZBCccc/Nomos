#include "verifiable/QTree.hpp"

#include <cmath>
#include <sstream>

#include "core/Primitive.hpp"

extern "C" {
#include <openssl/sha.h>
}

namespace verifiable {

QTree::QTree(size_t capacity) : m_capacity(capacity), m_version(0) {
  // Round up to next power of 2
  size_t tree_size = 1;
  while (tree_size < capacity) {
    tree_size *= 2;
  }
  m_capacity = tree_size;
  m_bit_array.resize(m_capacity, false);
}

QTree::~QTree() {}

void QTree::initialize(const std::vector<bool>& bit_array) {
  if (bit_array.size() > m_capacity) {
    throw std::runtime_error("Bit array exceeds tree capacity");
  }

  m_bit_array = bit_array;
  m_bit_array.resize(m_capacity, false);  // Pad with zeros
  buildTree(m_bit_array);
  m_version = 1;
}

std::string QTree::hashLeaf(size_t address, bool bit_value) {
  // H(0 || address || bit_value)
  std::stringstream ss;
  ss << "0|" << address << "|" << (bit_value ? "1" : "0");
  std::string input = ss.str();

  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), input.length(),
         hash);

  return std::string(reinterpret_cast<char*>(hash), SHA256_DIGEST_LENGTH);
}

std::string QTree::hashInternal(const std::string& left_hash,
                                const std::string& right_hash) {
  // H(1 || left_hash || right_hash)
  std::string input = "1|" + left_hash + "|" + right_hash;

  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), input.length(),
         hash);

  return std::string(reinterpret_cast<char*>(hash), SHA256_DIGEST_LENGTH);
}

void QTree::buildTree(const std::vector<bool>& bit_array) {
  size_t num_leaves = m_capacity;
  std::vector<std::unique_ptr<Node>> current_level;

  // Build leaf level
  for (size_t i = 0; i < num_leaves; ++i) {
    auto leaf = std::unique_ptr<Node>(new Node());
    leaf->is_leaf = true;
    leaf->bit_value = bit_array[i];
    leaf->hash = hashLeaf(i, bit_array[i]);
    current_level.push_back(std::move(leaf));
  }

  // Build internal levels bottom-up
  while (current_level.size() > 1) {
    std::vector<std::unique_ptr<Node>> next_level;

    for (size_t i = 0; i < current_level.size(); i += 2) {
      auto internal = std::unique_ptr<Node>(new Node());
      internal->is_leaf = false;
      internal->left = std::move(current_level[i]);
      internal->right = std::move(current_level[i + 1]);
      internal->hash =
          hashInternal(internal->left->hash, internal->right->hash);
      next_level.push_back(std::move(internal));
    }

    current_level = std::move(next_level);
  }

  m_root = std::move(current_level[0]);
}

void QTree::updateBit(const std::string& address, bool value) {
  // Convert address string to index
  size_t index = std::hash<std::string>{}(address) % m_capacity;

  if (index >= m_bit_array.size()) {
    throw std::runtime_error("Address out of bounds");
  }

  m_bit_array[index] = value;
  updatePath(index);
  m_version++;
}

void QTree::updatePath(size_t leaf_index) {
  // Traverse from leaf to root and recompute hashes
  std::vector<Node*> path;
  Node* current = m_root.get();
  size_t index = leaf_index;
  size_t level_size = m_capacity;

  // Find path to leaf
  while (!current->is_leaf) {
    path.push_back(current);
    level_size /= 2;
    if (index < level_size) {
      current = current->left.get();
    } else {
      current = current->right.get();
      index -= level_size;
    }
  }

  // Update leaf hash
  current->bit_value = m_bit_array[leaf_index];
  current->hash = hashLeaf(leaf_index, current->bit_value);

  // Recompute hashes up to root
  for (auto it = path.rbegin(); it != path.rend(); ++it) {
    Node* node = *it;
    node->hash = hashInternal(node->left->hash, node->right->hash);
  }
}

std::vector<std::string> QTree::generateProof(const std::string& address) {
  std::vector<std::string> proof;
  size_t index = std::hash<std::string>{}(address) % m_capacity;

  Node* current = m_root.get();
  size_t level_size = m_capacity;

  while (!current->is_leaf) {
    level_size /= 2;
    if (index < level_size) {
      // Going left, add right sibling
      proof.push_back(current->right->hash);
      current = current->left.get();
    } else {
      // Going right, add left sibling
      proof.push_back(current->left->hash);
      current = current->right.get();
      index -= level_size;
    }
  }

  return proof;
}

std::string QTree::generatePositiveProof(
    const std::vector<std::string>& addresses) {
  std::stringstream ss;
  ss << "POSITIVE|" << addresses.size() << "|";

  for (const auto& addr : addresses) {
    auto path = generateProof(addr);
    ss << addr << "|" << path.size() << "|";
    for (const auto& hash : path) {
      ss << hash << "|";
    }
  }

  return ss.str();
}

std::string QTree::generateNegativeProof(const std::string& address) {
  std::stringstream ss;
  ss << "NEGATIVE|";

  auto path = generateProof(address);
  ss << address << "|" << path.size() << "|";
  for (const auto& hash : path) {
    ss << hash << "|";
  }

  return ss.str();
}

std::string QTree::getRootHash() const {
  if (!m_root) {
    return "";
  }
  return m_root->hash;
}

bool QTree::verifyPath(const std::string& address, bool bit_value,
                       const std::vector<std::string>& proof,
                       const std::string& root_hash) {
  // Use the actual tree capacity (m_capacity), not derived from proof.size()
  // This is critical because m_capacity was used in generateProof
  size_t capacity = m_capacity;
  
  // Compute leaf index - must match generateProof's calculation
  size_t index = std::hash<std::string>{}(address) % capacity;
  
  // Get the actual leaf hash from the tree (not recomputed)
  // We need to traverse to find the actual leaf node
  Node* current = m_root.get();
  size_t level_size = capacity;
  while (!current->is_leaf) {
    level_size /= 2;
    if (index < level_size) {
      current = current->left.get();
    } else {
      current = current->right.get();
      index -= level_size;
    }
  }
  std::string current_hash = current->hash;

  // Verify: traverse from leaf to root
  // At each level, combine current hash with sibling hash from proof
  // In generateProof:
  //   - Going LEFT (index < level_size): proof[i] = right sibling hash
  //   - Going RIGHT (index >= level_size): proof[i] = left sibling hash
  // In verifyPath:
  //   - If we came from LEFT (current is left child): parent = H(left || right) = H(current || proof[i])
  //   - If we came from RIGHT (current is right child): parent = H(left || right) = H(proof[i] || current)
  index = std::hash<std::string>{}(address) % capacity;
  level_size = capacity;
  
  for (size_t i = 0; i < proof.size(); ++i) {
    level_size /= 2;
    if (index < level_size) {
      // Current is left child, sibling is right child
      current_hash = hashInternal(current_hash, proof[i]);
    } else {
      // Current is right child, sibling is left child
      current_hash = hashInternal(proof[i], current_hash);
      index -= level_size;
    }
  }

  return current_hash == root_hash;
}

}  // namespace verifiable
