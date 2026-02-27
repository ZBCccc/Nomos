#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "AddressCommitment.hpp"
#include "QTree.hpp"
#include "nomos/Gatekeeper.hpp"
#include "nomos/Server.hpp"
#include "nomos/types.hpp"

namespace verifiable {

/**
 * @brief Extended metadata for verifiable scheme
 */
struct VerifiableMetadata {
    nomos::Metadata base_metadata;  // Original Nomos metadata
    std::string commitment;         // Address commitment Cm_{w,id}
    std::string ae_ciphertext;      // Authenticated encryption of (id||op||Cm)
};

/**
 * @brief Extended trapdoor for verifiable scheme
 */
struct VerifiableTrapdoor {
    nomos::TrapdoorMetadata base_trapdoor;
    std::vector<int> beta_indices;  // Sampled indices for each cross keyword
};

/**
 * @brief Verification proof structure
 */
struct VerificationProof {
    std::string qtree_proof;              // QTree authentication paths
    std::vector<std::string> opening;     // Address opening (ℓ xtags)
    std::string root_hash;                // QTree root R_X^(t)
    uint64_t version;                     // Version number t
    std::string signature;                // Signature on (t, R_X^(t))
};

/**
 * @brief Verifiable Gatekeeper extending Nomos
 */
class VerifiableGatekeeper : public nomos::GateKeeper {
public:
    VerifiableGatekeeper();
    ~VerifiableGatekeeper();

    /**
     * @brief Setup with QTree initialization
     */
    int setupVerifiable(size_t xset_capacity);

    /**
     * @brief Update with commitment embedding
     */
    VerifiableMetadata updateVerifiable(nomos::OP op, const std::string& id,
                                       const std::string& keyword);

    /**
     * @brief Generate token with β indices
     */
    VerifiableTrapdoor genTokenVerifiable(const std::vector<std::string>& query_keywords);

    /**
     * @brief Get current QTree root for anchor generation
     */
    std::string getCurrentRoot() const;

    /**
     * @brief Get current version
     */
    uint64_t getCurrentVersion() const;

private:
    std::unique_ptr<QTree> m_qtree;
    std::unique_ptr<AddressCommitment> m_commitment;
    std::unordered_map<std::string, std::vector<std::string>> m_xtag_cache;  // Cache for (w,id) -> xtags
};

/**
 * @brief Verifiable Server extending Nomos
 */
class VerifiableServer : public nomos::Server {
public:
    VerifiableServer();
    ~VerifiableServer();

    /**
     * @brief Update with QTree maintenance
     */
    void updateVerifiable(const VerifiableMetadata& metadata);

    /**
     * @brief Search with proof generation
     */
    std::pair<std::vector<nomos::sEOp>, VerificationProof>
    searchVerifiable(const VerifiableTrapdoor& trapdoor);

    /**
     * @brief Set QTree instance (shared with gatekeeper for testing)
     */
    void setQTree(std::shared_ptr<QTree> qtree);

private:
    std::shared_ptr<QTree> m_qtree;
    std::unordered_map<std::string, std::vector<std::string>> m_xtag_storage;
};

}  // namespace verifiable
