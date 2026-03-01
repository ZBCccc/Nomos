#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

extern "C" {
#include <relic/relic.h>
}

namespace verifiable {

/**
 * @brief Merkle Hash Tree for XSet bit array verification
 *
 * Implements the QTree structure from Chapter 2 (bf.tex)
 * Provides bit value authenticity for qualification verification
 */
class QTree {
public:
    QTree(size_t capacity);
    ~QTree();

    /**
     * @brief Initialize the tree with given bit array
     * @param bit_array The XSet bit array B^(t)
     * @param size Size of the bit array
     */
    void initialize(const std::vector<bool>& bit_array);

    /**
     * @brief Update a single bit in the tree
     * @param address The address to update
     * @param value The new bit value
     */
    void updateBit(const std::string& address, bool value);

    /**
     * @brief Generate authentication path for a single address
     * @param address The address to prove
     * @return Authentication path (sibling hashes from leaf to root)
     */
    std::vector<std::string> generateProof(const std::string& address);

    /**
     * @brief Generate Positive proof (k authentication paths)
     * @param addresses List of k addresses that should all be 1
     * @return Proof structure containing k paths
     */
    std::string generatePositiveProof(const std::vector<std::string>& addresses);

    /**
     * @brief Generate Negative proof (1 authentication path for a 0 bit)
     * @param address The address with bit value 0
     * @return Proof structure containing 1 path
     */
    std::string generateNegativeProof(const std::string& address);

    /**
     * @brief Get current root hash (commitment)
     * @return Root hash R_X^(t)
     */
    std::string getRootHash() const;

    /**
     * @brief Get current version number
     */
    uint64_t getVersion() const { return m_version; }

    /**
     * @brief Verify an authentication path
     * @param address The address being verified
     * @param bit_value The claimed bit value
     * @param proof The authentication path
     * @param root_hash The expected root hash
     * @return true if verification passes
     */
    bool verifyPath(const std::string& address, bool bit_value,
                    const std::vector<std::string>& proof,
                    const std::string& root_hash);

    // Get tree capacity
    size_t getCapacity() const { return m_capacity; }

private:
    struct Node {
        std::string hash;
        std::unique_ptr<Node> left;
        std::unique_ptr<Node> right;
        bool is_leaf;
        bool bit_value;  // Only for leaf nodes
    };

    void buildTree(const std::vector<bool>& bit_array);
    void computeHash(Node* node, size_t index);
    std::string hashLeaf(size_t address, bool bit_value);
    std::string hashInternal(const std::string& left_hash, const std::string& right_hash);
    void updatePath(size_t leaf_index);

    std::unique_ptr<Node> m_root;
    size_t m_capacity;
    uint64_t m_version;
    std::vector<bool> m_bit_array;
    std::unordered_map<std::string, size_t> m_address_to_index;
};

}  // namespace verifiable
