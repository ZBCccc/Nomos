#pragma once

#include <cstring>
#include <string>
#include <vector>

extern "C" {
#include <relic/relic.h>
}

namespace mcodxt {

enum class OpType : uint8_t { ADD = 0, DEL = 1 };

struct UpdateMetadata {
  ep_t addr;
  std::vector<uint8_t> val;
  bn_t alpha;
  std::string xtag;

  UpdateMetadata() {
    ep_null(addr);
    bn_null(alpha);
  }

  UpdateMetadata(UpdateMetadata&& other) noexcept
      : val(std::move(other.val)), xtag(std::move(other.xtag)) {
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
      xtag = std::move(other.xtag);
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

  SearchToken() { ep_null(strap); }

  SearchToken(SearchToken&& other) noexcept
      : bstag(std::move(other.bstag)),
        delta(std::move(other.delta)),
        bxtrap(std::move(other.bxtrap)) {
    std::memcpy(strap, other.strap, sizeof(ep_t));
    ep_null(other.strap);
  }

  SearchToken& operator=(SearchToken&& other) noexcept {
    if (this != &other) {
      ep_free(strap);
      bstag = std::move(other.bstag);
      delta = std::move(other.delta);
      bxtrap = std::move(other.bxtrap);
      std::memcpy(strap, other.strap, sizeof(ep_t));
      ep_null(other.strap);
    }
    return *this;
  }

  ~SearchToken() { ep_free(strap); }

  SearchToken(const SearchToken&) = delete;
  SearchToken& operator=(const SearchToken&) = delete;
};

// Client-side GenToken output: reordered query plus pre-hashed group elements
// that the gatekeeper transforms with Ks, Kt and Kx.
struct TokenRequest {
  std::vector<std::string> query_keywords;
  std::vector<std::string> hashed_keywords;
  std::vector<std::string> hw1_j_0;
  std::vector<std::string> hw1_j_1;
};

struct SearchResultEntry {
  int j;
  std::vector<uint8_t> sval;
  int cnt;
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

}  // namespace mcodxt
