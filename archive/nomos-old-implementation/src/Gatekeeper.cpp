#include "nomos/Gatekeeper.hpp"

#include "core/Primitive.hpp"
#include "relic/relic_ep.h"

extern "C" {
#include <openssl/rand.h>
#include <openssl/sha.h>
}

#include <sstream>

using namespace nomos;

GateKeeper::GateKeeper() {
  bn_new(Ks);
  bn_new(Ky);
  bn_new(Km);
  state.reset(new GatekeeperState());
}

GateKeeper::~GateKeeper() {
  bn_free(Ks);
  bn_free(Ky);
  bn_free(Km);
}

int GateKeeper::Setup() {
  bn_t ord;
  bn_new(ord);
  ep_curve_get_ord(ord);
  bn_rand_mod(Ks, ord);
  bn_rand_mod(Ky, ord);
  bn_rand_mod(Km, ord);

  this->state->Clear();
  UpdateCnt.clear();

  bn_free(ord);
  return 0;
}

// Helper: Compute PRF-like hash
static std::string computePRF(const std::string& key, const std::string& input) {
  std::string combined = key + "|" + input;
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256(reinterpret_cast<const unsigned char*>(combined.c_str()),
         combined.length(), hash);
  return std::string(reinterpret_cast<char*>(hash), SHA256_DIGEST_LENGTH);
}

// Helper: Serialize bn_t to string
static std::string bn_to_string(bn_t bn) {
  uint8_t bytes[256];
  int len = bn_size_bin(bn);
  bn_write_bin(bytes, len, bn);
  return std::string(reinterpret_cast<char*>(bytes), len);
}

Metadata GateKeeper::Update(OP op, const std::string& id,
                            const std::string& keyword) {
  Metadata meta;

  // Step 1: Get or initialize update counter
  if (UpdateCnt.find(keyword) == UpdateCnt.end()) {
    UpdateCnt[keyword] = "0";
  }
  std::string cnt = UpdateCnt[keyword];

  // Step 2: Compute Kz = PRF(Ks, keyword)
  std::string ks_str = bn_to_string(Ks);
  std::string kz = computePRF(ks_str, keyword);

  // Step 3: Compute TSet address = PRF(Kz, cnt)
  meta.addr = computePRF(kz, cnt);

  // Step 4: Compute encrypted value
  // val = (id || op) ⊕ PRF(Ky, keyword || cnt)
  std::string ky_str = bn_to_string(Ky);
  std::string mask = computePRF(ky_str, keyword + "|" + cnt);

  std::stringstream ss;
  ss << id << "|" << static_cast<int>(op);
  std::string plaintext = ss.str();

  // XOR encryption
  std::string encrypted;
  for (size_t i = 0; i < plaintext.length(); ++i) {
    encrypted += static_cast<char>(plaintext[i] ^ mask[i % mask.length()]);
  }
  meta.val = encrypted;

  // Step 5: Compute alpha = PRF(Kz, "alpha")
  meta.alpha = computePRF(kz, "alpha");

  // Step 6: Compute ℓ xtags
  const int ell = 3;
  std::string km_str = bn_to_string(Km);
  std::string id_op = id + "|" + std::to_string(static_cast<int>(op));

  for (int i = 1; i <= ell; ++i) {
    // xtag_i = PRF(Km, keyword || id || op || i)
    std::stringstream xtag_input;
    xtag_input << keyword << "|" << id_op << "|" << i;
    std::string xtag = computePRF(km_str, xtag_input.str());
    meta.xtag_list.push_back(xtag);
  }

  // Step 7: Increment counter
  int cnt_int = std::stoi(cnt);
  UpdateCnt[keyword] = std::to_string(cnt_int + 1);

  // For testing: store plaintext
  meta.id_plaintext = id;
  meta.op_plaintext = static_cast<int>(op);
  meta.keyword = keyword;

  return meta;
}

TrapdoorMetadata GateKeeper::GenToken(
    const std::vector<std::string>& query_keywords) {
  TrapdoorMetadata trapdoor;

  if (query_keywords.empty()) {
    return trapdoor;
  }

  // Select primary keyword (ws) - lowest update count
  std::string ws = query_keywords[0];
  int min_cnt = UpdateCnt.count(ws) ? std::stoi(UpdateCnt[ws]) : 0;

  for (const auto& kw : query_keywords) {
    int cnt = UpdateCnt.count(kw) ? std::stoi(UpdateCnt[kw]) : 0;
    if (cnt < min_cnt) {
      min_cnt = cnt;
      ws = kw;
    }
  }

  // Generate stokens for primary keyword
  std::string ks_str = bn_to_string(Ks);
  std::string kz = computePRF(ks_str, ws);

  int cnt = UpdateCnt.count(ws) ? std::stoi(UpdateCnt[ws]) : 0;
  for (int i = 0; i < cnt; ++i) {
    // stoken = (Kz, i) - server will compute addr = PRF(Kz, i)
    std::stringstream ss;
    ss << kz << "|" << i;
    trapdoor.stokenList.push_back(ss.str());
  }

  // Generate xtokens for cross keywords
  const int k = 2;  // Number of samples
  const int ell = 3;

  for (const auto& wj : query_keywords) {
    if (wj == ws) continue;

    std::vector<std::string> xtoken_list;
    std::string km_str = bn_to_string(Km);

    // Sample k indices from [1, ℓ]
    for (int b = 0; b < k; ++b) {
      int beta = (b % ell) + 1;

      // xtoken = (keyword, beta) - server will compute xtag
      std::stringstream ss;
      ss << wj << "|" << beta;
      xtoken_list.push_back(ss.str());
    }

    trapdoor.xtokenList.push_back(xtoken_list);
  }

  trapdoor.primary_keyword = ws;

  return trapdoor;
}
