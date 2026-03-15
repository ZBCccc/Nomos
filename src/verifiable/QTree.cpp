#include "verifiable/QTree.hpp"

#include <sstream>
#include <stdexcept>
#include <vector>

extern "C" {
#include <openssl/sha.h>
}

namespace verifiable {

namespace {

void appendUint64(std::string* out, uint64_t value) {
  for (int shift = 56; shift >= 0; shift -= 8) {
    out->push_back(static_cast<char>((value >> shift) & 0xff));
  }
}

std::string sha256(const std::string& input) {
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256(reinterpret_cast<const unsigned char*>(input.data()), input.size(),
         hash);
  return std::string(reinterpret_cast<const char*>(hash), SHA256_DIGEST_LENGTH);
}

}  // namespace

QTree::QTree(size_t capacity) : m_capacity(capacity), m_version(0) {
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
    throw std::runtime_error("Bit array exceeds QTree capacity");
  }

  m_bit_array = bit_array;
  m_bit_array.resize(m_capacity, false);
  buildTree(m_bit_array);
  m_version = 1;
}

void QTree::updateBit(const std::string& address, bool value) {
  updateBits(std::vector<std::string>(1, address), value);
}

void QTree::updateBits(const std::vector<std::string>& addresses, bool value) {
  if (!m_root) {
    initialize(std::vector<bool>(m_capacity, false));
  }

  for (size_t i = 0; i < addresses.size(); ++i) {
    const size_t index = getLeafIndex(addresses[i]);
    m_bit_array[index] = value;
    updatePath(index);
  }
  m_version++;
}

bool QTree::getBit(const std::string& address) const {
  if (m_bit_array.empty()) {
    return false;
  }
  return m_bit_array[getLeafIndex(address)];
}

std::vector<std::string> QTree::generateProof(
    const std::string& address) const {
  std::vector<std::string> proof;
  if (!m_root) {
    return proof;
  }

  size_t index = getLeafIndex(address);
  const Node* current = m_root.get();
  size_t level_size = m_capacity;

  while (!current->is_leaf) {
    level_size /= 2;
    if (index < level_size) {
      proof.push_back(current->right->hash);
      current = current->left.get();
    } else {
      proof.push_back(current->left->hash);
      current = current->right.get();
      index -= level_size;
    }
  }

  return proof;
}

std::string QTree::generatePositiveProof(
    const std::vector<std::string>& addresses) const {
  std::stringstream ss;
  ss << "POSITIVE|" << addresses.size() << "|";

  for (size_t i = 0; i < addresses.size(); ++i) {
    const std::vector<std::string> path = generateProof(addresses[i]);
    ss << addresses[i] << "|" << path.size() << "|";
    for (size_t j = 0; j < path.size(); ++j) {
      ss << path[j] << "|";
    }
  }

  return ss.str();
}

std::string QTree::generateNegativeProof(const std::string& address) const {
  std::stringstream ss;
  ss << "NEGATIVE|";

  const std::vector<std::string> path = generateProof(address);
  ss << address << "|" << path.size() << "|";
  for (size_t i = 0; i < path.size(); ++i) {
    ss << path[i] << "|";
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
                       const std::string& root_hash) const {
  const size_t index = getLeafIndex(address);

  std::vector<bool> went_right(proof.size(), false);
  size_t tmp_index = index;
  size_t level_size = m_capacity;
  for (size_t i = 0; i < proof.size(); ++i) {
    level_size /= 2;
    if (tmp_index >= level_size) {
      went_right[i] = true;
      tmp_index -= level_size;
    }
  }

  std::string current_hash = hashLeaf(index, bit_value);
  for (int i = static_cast<int>(proof.size()) - 1; i >= 0; --i) {
    if (went_right[static_cast<size_t>(i)]) {
      current_hash = hashInternal(proof[static_cast<size_t>(i)], current_hash);
    } else {
      current_hash = hashInternal(current_hash, proof[static_cast<size_t>(i)]);
    }
  }

  return current_hash == root_hash;
}

size_t QTree::getLeafIndex(const std::string& address) const {
  const std::string digest = sha256(address);
  uint64_t value = 0;
  for (int i = 0; i < 8; ++i) {
    value = (value << 8) |
            static_cast<unsigned char>(digest[static_cast<size_t>(i)]);
  }
  return static_cast<size_t>(value % m_capacity);
}

void QTree::buildTree(const std::vector<bool>& bit_array) {
  std::vector<std::unique_ptr<Node>> current_level;
  current_level.reserve(m_capacity);

  for (size_t i = 0; i < m_capacity; ++i) {
    std::unique_ptr<Node> leaf(new Node());
    leaf->is_leaf = true;
    leaf->bit_value = bit_array[i];
    leaf->hash = hashLeaf(i, bit_array[i]);
    current_level.push_back(std::move(leaf));
  }

  while (current_level.size() > 1) {
    std::vector<std::unique_ptr<Node>> next_level;
    next_level.reserve(current_level.size() / 2);

    for (size_t i = 0; i < current_level.size(); i += 2) {
      std::unique_ptr<Node> parent(new Node());
      parent->is_leaf = false;
      parent->left = std::move(current_level[i]);
      parent->right = std::move(current_level[i + 1]);
      parent->hash = hashInternal(parent->left->hash, parent->right->hash);
      next_level.push_back(std::move(parent));
    }

    current_level = std::move(next_level);
  }

  m_root = std::move(current_level[0]);
}

std::string QTree::hashLeaf(size_t address, bool bit_value) const {
  std::string input;
  input.push_back(static_cast<char>(0));
  appendUint64(&input, static_cast<uint64_t>(address));
  input.push_back(static_cast<char>(bit_value ? 1 : 0));
  return sha256(input);
}

std::string QTree::hashInternal(const std::string& left_hash,
                                const std::string& right_hash) const {
  std::string input;
  input.push_back(static_cast<char>(1));
  input.append(left_hash);
  input.append(right_hash);
  return sha256(input);
}

void QTree::updatePath(size_t leaf_index) {
  std::vector<Node*> path;
  Node* current = m_root.get();
  size_t index = leaf_index;
  size_t level_size = m_capacity;

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

  current->bit_value = m_bit_array[leaf_index];
  current->hash = hashLeaf(leaf_index, current->bit_value);

  for (std::vector<Node*>::reverse_iterator it = path.rbegin();
       it != path.rend(); ++it) {
    Node* node = *it;
    node->hash = hashInternal(node->left->hash, node->right->hash);
  }
}

}  // namespace verifiable
