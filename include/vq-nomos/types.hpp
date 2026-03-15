#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

extern "C" {
#include <relic/relic.h>
}

namespace vqnomos {

enum OP { OP_ADD = 0, OP_DEL = 1 };

struct Anchor {
  uint64_t version;
  std::string root_hash;
  std::string signature;

  Anchor() : version(0) {}
};

struct MerkleAuth {
  std::string root_hash;
  std::string signature;
};

struct MerkleOpening {
  int beta_index;
  std::string xtag;
  std::vector<std::string> path;

  MerkleOpening() : beta_index(0) {}
};

struct QTreeWitness {
  std::string address;
  bool bit_value;
  std::vector<std::string> path;

  QTreeWitness() : bit_value(false) {}
};

struct QualificationProof {
  bool verdict;
  std::vector<QTreeWitness> witnesses;

  QualificationProof() : verdict(false) {}
};

struct RelationProof {
  int keyword_offset;
  int candidate_slot;
  QualificationProof qualification;
  bool has_auth;
  MerkleAuth auth;
  std::vector<MerkleOpening> openings;

  RelationProof() : keyword_offset(0), candidate_slot(0), has_auth(false) {}
};

struct CandidateEntry {
  int candidate_slot;
  std::vector<uint8_t> sval;

  CandidateEntry() : candidate_slot(0) {}
};

struct SearchResponse {
  std::vector<CandidateEntry> entries;
  std::vector<int> result_slots;
  Anchor anchor;
  std::vector<RelationProof> relation_proofs;
};

struct UpdateMetadata {
  ep_t addr;
  std::vector<uint8_t> val;
  bn_t alpha;
  std::vector<std::string> xtags;
  std::string keyword;
  std::string merkle_root;
  std::string merkle_signature;
  Anchor anchor;

  UpdateMetadata() {
    ep_null(addr);
    bn_null(alpha);
  }

  UpdateMetadata(UpdateMetadata&& other) noexcept
      : val(std::move(other.val)),
        xtags(std::move(other.xtags)),
        keyword(std::move(other.keyword)),
        merkle_root(std::move(other.merkle_root)),
        merkle_signature(std::move(other.merkle_signature)),
        anchor(std::move(other.anchor)) {
    std::memcpy(addr, other.addr, sizeof(ep_t));
    std::memcpy(alpha, other.alpha, sizeof(bn_t));
    ep_null(other.addr);
    bn_null(other.alpha);
  }

  UpdateMetadata& operator=(UpdateMetadata&& other) noexcept {
    if (this != &other) {
      ep_free(addr);
      bn_free(alpha);
      val = std::move(other.val);
      xtags = std::move(other.xtags);
      keyword = std::move(other.keyword);
      merkle_root = std::move(other.merkle_root);
      merkle_signature = std::move(other.merkle_signature);
      anchor = std::move(other.anchor);
      std::memcpy(addr, other.addr, sizeof(ep_t));
      std::memcpy(alpha, other.alpha, sizeof(bn_t));
      ep_null(other.addr);
      bn_null(other.alpha);
    }
    return *this;
  }

  ~UpdateMetadata() {
    ep_free(addr);
    bn_free(alpha);
  }

  UpdateMetadata(const UpdateMetadata&) = delete;
  UpdateMetadata& operator=(const UpdateMetadata&) = delete;
};

struct SearchToken {
  ep_t strap;
  std::vector<std::string> bstag;
  std::vector<std::string> delta;
  std::vector<std::vector<std::string>> bxtrap;
  std::vector<int> beta_indices;
  Anchor anchor;

  SearchToken() { ep_null(strap); }

  SearchToken(SearchToken&& other) noexcept
      : bstag(std::move(other.bstag)),
        delta(std::move(other.delta)),
        bxtrap(std::move(other.bxtrap)),
        beta_indices(std::move(other.beta_indices)),
        anchor(std::move(other.anchor)) {
    std::memcpy(strap, other.strap, sizeof(ep_t));
    ep_null(other.strap);
  }

  SearchToken& operator=(SearchToken&& other) noexcept {
    if (this != &other) {
      ep_free(strap);
      bstag = std::move(other.bstag);
      delta = std::move(other.delta);
      bxtrap = std::move(other.bxtrap);
      beta_indices = std::move(other.beta_indices);
      anchor = std::move(other.anchor);
      std::memcpy(strap, other.strap, sizeof(ep_t));
      ep_null(other.strap);
    }
    return *this;
  }

  ~SearchToken() { ep_free(strap); }

  SearchToken(const SearchToken&) = delete;
  SearchToken& operator=(const SearchToken&) = delete;
};

struct TokenRequest {
  std::vector<std::string> query_keywords;
  std::vector<std::string> hashed_keywords;
  std::vector<std::string> hw1_j_0;
  std::vector<std::string> hw1_j_1;
};

struct SearchRequest {
  int num_keywords;
  std::vector<std::string> stokenList;
  std::vector<std::vector<std::vector<std::string>>> xtokenList;

  SearchRequest() : num_keywords(0) {}
};

struct TSetEntry {
  std::vector<uint8_t> val;
  bn_t alpha;

  TSetEntry() { bn_null(alpha); }

  TSetEntry(TSetEntry&& other) noexcept : val(std::move(other.val)) {
    std::memcpy(alpha, other.alpha, sizeof(bn_t));
    bn_null(other.alpha);
  }

  TSetEntry& operator=(TSetEntry&& other) noexcept {
    if (this != &other) {
      bn_free(alpha);
      val = std::move(other.val);
      std::memcpy(alpha, other.alpha, sizeof(bn_t));
      bn_null(other.alpha);
    }
    return *this;
  }

  ~TSetEntry() { bn_free(alpha); }

  TSetEntry(const TSetEntry&) = delete;
  TSetEntry& operator=(const TSetEntry&) = delete;
};

struct VerificationResult {
  bool accepted;
  std::vector<std::string> ids;

  VerificationResult() : accepted(false) {}
};

}  // namespace vqnomos
