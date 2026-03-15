#include "vq-nomos/Client.hpp"

#include <algorithm>
#include <cstdlib>
#include <map>
#include <set>
#include <sstream>
#include <unordered_map>
#include <utility>

#include "core/Primitive.hpp"
#include "vq-nomos/Common.hpp"
#include "vq-nomos/MerkleOpen.hpp"
#include "vq-nomos/QTree.hpp"

namespace vqnomos {

namespace {

int getUpdateCount(const std::unordered_map<std::string, int>& update_count,
                   const std::string& keyword) {
  std::unordered_map<std::string, int>::const_iterator it =
      update_count.find(keyword);
  if (it == update_count.end()) {
    return 0;
  }
  return it->second;
}

struct RelationKey {
  int keyword_offset;
  int candidate_slot;

  bool operator<(const RelationKey& other) const {
    if (candidate_slot != other.candidate_slot) {
      return candidate_slot < other.candidate_slot;
    }
    return keyword_offset < other.keyword_offset;
  }
};

}  // namespace

Client::Client() : m_qtree_capacity(1024), m_bucket_count(10) {}

Client::~Client() {}

int Client::setup(const std::string& public_key_pem,
                  const Anchor& initial_anchor, size_t qtree_capacity,
                  int bucket_count) {
  m_public_key_pem = public_key_pem;
  m_local_anchor = initial_anchor;
  m_qtree_capacity = qtree_capacity;
  m_bucket_count = bucket_count;
  return 0;
}

TokenRequest Client::genToken(
    const std::vector<std::string>& query_keywords,
    const std::unordered_map<std::string, int>& update_count) {
  // Paper: Algorithm 4 - Nomos GenToken (client side) without OPRF blinding.
  TokenRequest req;
  const int n = static_cast<int>(query_keywords.size());
  if (n == 0) {
    return req;
  }

  req.query_keywords = query_keywords;

  int min_update_count = getUpdateCount(update_count, req.query_keywords[0]);
  int min_index = 0;
  for (int i = 1; i < n; ++i) {
    const int current_count =
        getUpdateCount(update_count, req.query_keywords[i]);
    if (current_count < min_update_count) {
      min_update_count = current_count;
      min_index = i;
    }
  }

  if (min_index != 0) {
    std::swap(req.query_keywords[0], req.query_keywords[min_index]);
  }

  const std::string& w1 = req.query_keywords[0];
  const int m = getUpdateCount(update_count, w1);
  if (m == 0) {
    return req;
  }

  for (int i = 0; i < n; ++i) {
    ep_t hw;
    ep_new(hw);
    Hash_H1(hw, req.query_keywords[i]);
    req.hashed_keywords.push_back(SerializePoint(hw));
    ep_free(hw);
  }

  for (int j = 1; j <= m; ++j) {
    std::stringstream ss_b;
    std::stringstream ss_c;
    ss_b << w1 << "|" << j << "|0";
    ss_c << w1 << "|" << j << "|1";

    ep_t b;
    ep_t c;
    ep_new(b);
    ep_new(c);
    Hash_H1(b, ss_b.str());
    Hash_H1(c, ss_c.str());
    req.hw1_j_0.push_back(SerializePoint(b));
    req.hw1_j_1.push_back(SerializePoint(c));
    ep_free(b);
    ep_free(c);
  }

  return req;
}

SearchRequest Client::prepareSearch(const SearchToken& token,
                                    const TokenRequest& token_request) {
  // Paper: Algorithm 5 - Search (client side)
  SearchRequest req;
  const int n = static_cast<int>(token_request.query_keywords.size());
  if (n == 0) {
    return req;
  }

  const int m =
      static_cast<int>(std::min(token.bstag.size(), token.delta.size()));
  if (m == 0) {
    return req;
  }

  req.num_keywords = n;

  const std::string& w1 = token_request.query_keywords[0];
  const std::string kz = F(SerializePoint(token.strap), "1");

  bn_t* e = new bn_t[m];
  for (int j = 1; j <= m; ++j) {
    bn_new(e[j - 1]);
    std::stringstream ss;
    ss << w1 << "|" << j;
    F_p(e[j - 1], kz, ss.str());
  }

  for (int j = 0; j < m; ++j) {
    req.stokenList.push_back(token.bstag[static_cast<size_t>(j)]);
  }

  const int k = 2;
  const int cross_keyword_count =
      std::min(static_cast<int>(token.bxtrap.size()), n - 1);
  for (int j = 0; j < m; ++j) {
    std::vector<std::vector<std::string>> xtoken_list_j;
    for (int i = 0; i < cross_keyword_count; ++i) {
      std::vector<std::string> xtoken_list_ji;
      for (int t = 0; t < k; ++t) {
        ep_t bxtrap_it;
        ep_new(bxtrap_it);
        DeserializePoint(
            bxtrap_it,
            token.bxtrap[static_cast<size_t>(i)][static_cast<size_t>(t)]);

        ep_t xtoken;
        ep_new(xtoken);
        ep_mul(xtoken, bxtrap_it, e[j]);
        xtoken_list_ji.push_back(SerializePoint(xtoken));

        ep_free(xtoken);
        ep_free(bxtrap_it);
      }
      xtoken_list_j.push_back(xtoken_list_ji);
    }
    req.xtokenList.push_back(xtoken_list_j);
  }

  for (int j = 0; j < m; ++j) {
    bn_free(e[j]);
  }
  delete[] e;
  return req;
}

VerificationResult Client::decryptAndVerify(const SearchResponse& response,
                                            const SearchToken& token,
                                            const TokenRequest& token_request) {
  // Paper: Verify' - four-layer VQNomos verification.
  VerificationResult result;
  if (token_request.query_keywords.empty()) {
    result.accepted = response.entries.empty() && response.result_slots.empty();
    return result;
  }

  const std::string anchor_message =
      BuildAnchorMessage(response.anchor.version, response.anchor.root_hash);
  if (!VerifyMessage(m_public_key_pem, anchor_message,
                     response.anchor.signature)) {
    return result;
  }
  if (response.anchor.version < m_local_anchor.version ||
      response.anchor.version != token.anchor.version ||
      response.anchor.root_hash != token.anchor.root_hash ||
      response.anchor.signature != token.anchor.signature) {
    return result;
  }

  std::map<int, DecryptedEntry> decrypted_entries;
  for (size_t i = 0; i < response.entries.size(); ++i) {
    DecryptedEntry decrypted;
    if (!decryptEntry(&decrypted, response.entries[i], token)) {
      return result;
    }
    decrypted_entries[response.entries[i].candidate_slot] = decrypted;
  }

  const int cross_keyword_count =
      static_cast<int>(token_request.query_keywords.size()) - 1;
  std::set<RelationKey> expected_relations;
  for (std::map<int, DecryptedEntry>::const_iterator it =
           decrypted_entries.begin();
       it != decrypted_entries.end(); ++it) {
    for (int keyword_offset = 1; keyword_offset <= cross_keyword_count;
         ++keyword_offset) {
      RelationKey key;
      key.keyword_offset = keyword_offset;
      key.candidate_slot = it->first;
      expected_relations.insert(key);
    }
  }

  std::map<RelationKey, bool> relation_verdicts;
  QTree qtree_verifier(m_qtree_capacity);
  for (size_t i = 0; i < response.relation_proofs.size(); ++i) {
    const RelationProof& proof = response.relation_proofs[i];
    RelationKey key;
    key.keyword_offset = proof.keyword_offset;
    key.candidate_slot = proof.candidate_slot;

    if (expected_relations.find(key) == expected_relations.end() ||
        relation_verdicts.find(key) != relation_verdicts.end()) {
      return result;
    }

    std::set<std::string> opened_addresses;
    if (proof.has_auth) {
      if (proof.keyword_offset < 1 ||
          proof.keyword_offset >=
              static_cast<int>(token_request.query_keywords.size())) {
        return result;
      }

      const int bucket_index = ComputeKeywordBucketIndex(
          token_request
              .query_keywords[static_cast<size_t>(proof.keyword_offset)],
          m_bucket_count);
      const std::string auth_message =
          BuildMerkleAuthMessage(bucket_index, proof.auth.root_hash);
      if (!VerifyMessage(m_public_key_pem, auth_message,
                         proof.auth.signature)) {
        return result;
      }

      if (proof.openings.size() != token.beta_indices.size()) {
        return result;
      }

      for (size_t opening_index = 0; opening_index < proof.openings.size();
           ++opening_index) {
        const MerkleOpening& opening = proof.openings[opening_index];
        if (opening.beta_index != token.beta_indices[opening_index]) {
          return result;
        }
        if (!MerkleOpenTree::VerifyPath(proof.auth.root_hash,
                                        opening.beta_index, opening.xtag,
                                        opening.path, 3)) {
          return result;
        }
        opened_addresses.insert(opening.xtag);
      }
    }

    std::set<std::string> proof_addresses;
    bool recomputed_verdict = proof.qualification.verdict;
    if (proof.qualification.verdict) {
      if (proof.qualification.witnesses.size() != token.beta_indices.size()) {
        return result;
      }
      recomputed_verdict = true;
    } else {
      if (proof.qualification.witnesses.empty()) {
        return result;
      }
      recomputed_verdict = false;
    }

    for (size_t witness_index = 0;
         witness_index < proof.qualification.witnesses.size();
         ++witness_index) {
      const QTreeWitness& witness =
          proof.qualification.witnesses[witness_index];
      if (!qtree_verifier.verifyPath(witness.address, witness.bit_value,
                                     witness.path, response.anchor.root_hash)) {
        return result;
      }
      proof_addresses.insert(witness.address);
      if (proof.qualification.verdict && !witness.bit_value) {
        return result;
      }
      if (!proof.qualification.verdict && !witness.bit_value) {
        recomputed_verdict = false;
      }
    }

    if (proof.qualification.verdict) {
      if (proof_addresses != opened_addresses) {
        return result;
      }
    } else if (proof.has_auth) {
      for (std::set<std::string>::const_iterator it = proof_addresses.begin();
           it != proof_addresses.end(); ++it) {
        if (opened_addresses.find(*it) == opened_addresses.end()) {
          return result;
        }
      }
    }

    if (recomputed_verdict != proof.qualification.verdict) {
      return result;
    }
    relation_verdicts[key] = recomputed_verdict;
  }

  if (relation_verdicts.size() != expected_relations.size()) {
    return result;
  }

  std::set<int> recomputed_result_slots;
  for (std::map<int, DecryptedEntry>::const_iterator it =
           decrypted_entries.begin();
       it != decrypted_entries.end(); ++it) {
    bool keep = true;
    for (int keyword_offset = 1; keyword_offset <= cross_keyword_count;
         ++keyword_offset) {
      RelationKey key;
      key.keyword_offset = keyword_offset;
      key.candidate_slot = it->first;
      if (!relation_verdicts[key]) {
        keep = false;
        break;
      }
    }
    if (keep) {
      recomputed_result_slots.insert(it->first);
    }
  }

  std::set<int> server_result_slots(response.result_slots.begin(),
                                    response.result_slots.end());
  if (recomputed_result_slots != server_result_slots) {
    return result;
  }

  std::unordered_map<std::string, int> net_count;
  for (std::map<int, DecryptedEntry>::const_iterator it =
           decrypted_entries.begin();
       it != decrypted_entries.end(); ++it) {
    if (recomputed_result_slots.find(it->first) ==
        recomputed_result_slots.end()) {
      continue;
    }
    if (it->second.op == OP_ADD) {
      net_count[it->second.id]++;
    } else {
      net_count[it->second.id]--;
    }
  }

  for (std::unordered_map<std::string, int>::const_iterator it =
           net_count.begin();
       it != net_count.end(); ++it) {
    if (it->second > 0) {
      result.ids.push_back(it->first);
    }
  }
  std::sort(result.ids.begin(), result.ids.end());

  result.accepted = true;
  m_local_anchor = response.anchor;
  return result;
}

bool Client::decryptEntry(DecryptedEntry* output, const CandidateEntry& entry,
                          const SearchToken& token) const {
  if (entry.candidate_slot < 1 ||
      entry.candidate_slot > static_cast<int>(token.delta.size())) {
    return false;
  }

  ep_t delta_j;
  ep_new(delta_j);
  DeserializePoint(delta_j,
                   token.delta[static_cast<size_t>(entry.candidate_slot - 1)]);

  const int delta_len = ep_size_bin(delta_j, 1);
  if (delta_len <= 0) {
    ep_free(delta_j);
    return false;
  }

  std::vector<uint8_t> delta_bytes(static_cast<size_t>(delta_len));
  ep_write_bin(delta_bytes.data(), delta_len, delta_j, 1);
  ep_free(delta_j);

  const size_t dec_len =
      std::min(entry.sval.size(), static_cast<size_t>(delta_len));
  std::vector<uint8_t> plaintext(dec_len);
  for (size_t i = 0; i < dec_len; ++i) {
    plaintext[i] = entry.sval[i] ^ delta_bytes[i];
  }

  const std::string decrypted(plaintext.begin(), plaintext.end());
  const size_t separator = decrypted.find('|');
  if (separator == std::string::npos) {
    return false;
  }

  output->id = decrypted.substr(0, separator);
  output->op = std::atoi(decrypted.substr(separator + 1).c_str());
  return true;
}

}  // namespace vqnomos
