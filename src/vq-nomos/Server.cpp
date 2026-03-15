#include "vq-nomos/Server.hpp"

#include <stdexcept>
#include <vector>

namespace vqnomos {

Server::Server() : m_qtree(new QTree(1024)) {}

Server::~Server() {
  m_TSet.clear();
  m_XSet.clear();
  m_MPos.clear();
  m_MTree.clear();
}

void Server::setup(const std::vector<uint8_t>& /*Km*/,
                   const Anchor& initial_anchor, size_t qtree_capacity) {
  m_qtree.reset(new QTree(qtree_capacity));
  m_qtree->initialize(std::vector<bool>(m_qtree->getCapacity(), false));
  m_current_anchor = initial_anchor;
}

void Server::update(const UpdateMetadata& metadata) {
  // Paper: Update' - store TSet/XSet plus Merkle-open auxiliary state.
  const std::string addr_key = serializePoint(metadata.addr);

  TSetEntry entry;
  entry.val = metadata.val;
  bn_new(entry.alpha);
  bn_copy(entry.alpha, metadata.alpha);
  m_TSet[addr_key] = std::move(entry);

  std::shared_ptr<MerkleOpenTree> merkle_tree(
      new MerkleOpenTree(metadata.xtags));
  m_MTree[metadata.merkle_root] = merkle_tree;

  for (size_t i = 0; i < metadata.xtags.size(); ++i) {
    const std::string& xtag = metadata.xtags[i];
    m_XSet[xtag] = true;

    MerklePosition position;
    position.leaf_index = static_cast<int>(i + 1);
    position.root_hash = metadata.merkle_root;
    position.signature = metadata.merkle_signature;
    m_MPos[xtag] = position;
  }

  m_qtree->updateBits(metadata.xtags, true);
  m_current_anchor = metadata.anchor;
}

SearchResponse Server::search(const SearchRequest& request,
                              const SearchToken& token) {
  // Paper: Search-Prove' - generate Merkle-open and QTree proofs.
  SearchResponse response;
  response.anchor = m_current_anchor;

  const int num_keywords = request.num_keywords;
  const int cross_keyword_count = num_keywords > 0 ? num_keywords - 1 : 0;

  for (int j = 0; j < static_cast<int>(request.stokenList.size()); ++j) {
    const std::string& stag_key = request.stokenList[static_cast<size_t>(j)];
    std::map<std::string, TSetEntry>::const_iterator entry_it =
        m_TSet.find(stag_key);
    if (entry_it == m_TSet.end()) {
      continue;
    }

    CandidateEntry candidate;
    candidate.candidate_slot = j + 1;
    candidate.sval = entry_it->second.val;
    response.entries.push_back(candidate);

    bool all_match = true;
    for (int keyword_offset = 0; keyword_offset < cross_keyword_count;
         ++keyword_offset) {
      if (j >= static_cast<int>(request.xtokenList.size()) ||
          keyword_offset >=
              static_cast<int>(
                  request.xtokenList[static_cast<size_t>(j)].size())) {
        all_match = false;
        break;
      }

      RelationProof relation_proof;
      relation_proof.keyword_offset = keyword_offset + 1;
      relation_proof.candidate_slot = j + 1;

      const std::vector<std::string>& xtokens =
          request.xtokenList[static_cast<size_t>(j)]
                            [static_cast<size_t>(keyword_offset)];
      std::vector<std::string> sampled_xtags;
      std::vector<bool> sampled_bits;
      std::vector<MerklePosition> sampled_positions;
      bool has_full_merkle_open = !xtokens.empty();

      for (size_t t = 0; t < xtokens.size(); ++t) {
        ep_t xtoken;
        ep_new(xtoken);
        ep_read_bin(xtoken, reinterpret_cast<const uint8_t*>(xtokens[t].data()),
                    static_cast<int>(xtokens[t].size()));

        ep_t xtag;
        ep_new(xtag);
        ep_mul(xtag, xtoken, entry_it->second.alpha);
        const std::string xtag_key = serializePoint(xtag);

        sampled_xtags.push_back(xtag_key);
        sampled_bits.push_back(m_XSet.find(xtag_key) != m_XSet.end());

        std::map<std::string, MerklePosition>::const_iterator mpos_it =
            m_MPos.find(xtag_key);
        if (mpos_it == m_MPos.end()) {
          has_full_merkle_open = false;
        } else {
          sampled_positions.push_back(mpos_it->second);
        }

        ep_free(xtag);
        ep_free(xtoken);
      }

      if (has_full_merkle_open &&
          sampled_positions.size() == sampled_xtags.size()) {
        const std::string root_hash = sampled_positions[0].root_hash;
        const std::string signature = sampled_positions[0].signature;
        for (size_t t = 1; t < sampled_positions.size(); ++t) {
          if (sampled_positions[t].root_hash != root_hash ||
              sampled_positions[t].signature != signature) {
            has_full_merkle_open = false;
            break;
          }
        }

        if (has_full_merkle_open) {
          relation_proof.has_auth = true;
          relation_proof.auth.root_hash = root_hash;
          relation_proof.auth.signature = signature;

          std::map<std::string, std::shared_ptr<MerkleOpenTree>>::const_iterator
              tree_it = m_MTree.find(root_hash);
          if (tree_it == m_MTree.end()) {
            throw std::runtime_error("Merkle tree state missing for root");
          }

          for (size_t t = 0; t < sampled_xtags.size(); ++t) {
            MerkleOpening opening;
            opening.beta_index = token.beta_indices[t];
            opening.xtag = sampled_xtags[t];
            opening.path =
                tree_it->second->generateProof(sampled_positions[t].leaf_index);
            relation_proof.openings.push_back(opening);
          }
        }
      }

      relation_proof.qualification.verdict = true;
      int first_zero_index = -1;
      for (size_t t = 0; t < sampled_bits.size(); ++t) {
        if (!sampled_bits[t]) {
          relation_proof.qualification.verdict = false;
          first_zero_index = static_cast<int>(t);
          break;
        }
      }

      if (relation_proof.qualification.verdict) {
        for (size_t t = 0; t < sampled_xtags.size(); ++t) {
          QTreeWitness witness;
          witness.address = sampled_xtags[t];
          witness.bit_value = true;
          witness.path = m_qtree->generateProof(sampled_xtags[t]);
          relation_proof.qualification.witnesses.push_back(witness);
        }
      } else if (first_zero_index >= 0) {
        QTreeWitness witness;
        witness.address = sampled_xtags[static_cast<size_t>(first_zero_index)];
        witness.bit_value = false;
        witness.path = m_qtree->generateProof(witness.address);
        relation_proof.qualification.witnesses.push_back(witness);
      }

      response.relation_proofs.push_back(relation_proof);
      if (!relation_proof.qualification.verdict) {
        all_match = false;
      }
    }

    if (all_match) {
      response.result_slots.push_back(j + 1);
    }
  }

  return response;
}

std::string Server::serializePoint(const ep_t point) const {
  const int len = ep_size_bin(point, 1);
  std::vector<uint8_t> bytes(static_cast<size_t>(len));
  ep_write_bin(bytes.data(), len, point, 1);
  return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

}  // namespace vqnomos
