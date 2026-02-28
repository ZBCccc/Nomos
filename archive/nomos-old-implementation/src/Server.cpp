#include "nomos/Server.hpp"

#include <iostream>
#include <sstream>

extern "C" {
#include <openssl/sha.h>
}

using namespace nomos;

// Helper: Compute PRF-like hash
static std::string computePRF(const std::string& key, const std::string& input) {
  std::string combined = key + "|" + input;
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256(reinterpret_cast<const unsigned char*>(combined.c_str()),
         combined.length(), hash);
  return std::string(reinterpret_cast<char*>(hash), SHA256_DIGEST_LENGTH);
}

Server::Server() {}

Server::~Server() {}

void Server::Update(const Metadata& metadata) {
  TSet[metadata.addr] = metadata.val;
  for (const auto& xtag : metadata.xtag_list) {
    XSet.insert(xtag);
  }
  // Store plaintext for testing
  PlaintextMap[metadata.addr] = std::make_pair(metadata.id_plaintext, metadata.op_plaintext);
  // Track which keywords each document has
  if (metadata.op_plaintext == 0) {  // OP_ADD
    DocKeywords[metadata.id_plaintext].insert(metadata.keyword);
  }
}

std::vector<sEOp> Server::Search(const TrapdoorMetadata& trapdoor_metadata) {
  std::vector<sEOp> sEop_list;

  std::cout << "  [Server] Searching with " << trapdoor_metadata.stokenList.size()
            << " stokens, " << trapdoor_metadata.xtokenList.size() << " xtoken lists" << std::endl;
  std::cout << "  [Server] Primary keyword: " << trapdoor_metadata.primary_keyword << std::endl;
  std::cout << "  [Server] TSet size: " << TSet.size() << ", XSet size: " << XSet.size() << std::endl;

  // Step 1: Enumerate candidates using stokens
  for (size_t i = 0; i < trapdoor_metadata.stokenList.size(); ++i) {
    const std::string& stoken = trapdoor_metadata.stokenList[i];

    // Parse stoken: kz|counter
    size_t pos = stoken.rfind('|');
    if (pos == std::string::npos) continue;

    std::string kz = stoken.substr(0, pos);
    std::string cnt = stoken.substr(pos + 1);

    // Compute TSet address = PRF(Kz, cnt)
    std::string addr = computePRF(kz, cnt);

    // Look up in TSet
    auto it = TSet.find(addr);
    if (it == TSet.end()) {
      std::cout << "    [Server] Candidate " << i << ": TSet miss" << std::endl;
      continue;
    }

    std::string encrypted_val = it->second;

    // Get plaintext for testing
    auto pt_it = PlaintextMap.find(addr);
    if (pt_it == PlaintextMap.end()) {
      std::cout << "    [Server] Candidate " << i << ": No plaintext mapping" << std::endl;
      continue;
    }

    std::string id = pt_it->second.first;
    int op = pt_it->second.second;

    std::cout << "    [Server] Candidate " << i << ": id=" << id << ", op=" << op << std::endl;

    // Create candidate entry
    sEOp eop;
    eop.j = static_cast<int>(i);
    eop.sval = encrypted_val;
    eop.id = id;
    eop.cnt = 0;

    // Step 2: Cross-filter using xtokens
    bool passes_all = true;

    for (size_t xlist_idx = 0; xlist_idx < trapdoor_metadata.xtokenList.size(); ++xlist_idx) {
      const auto& xtoken_list = trapdoor_metadata.xtokenList[xlist_idx];
      bool passes_this_keyword = false;

      std::cout << "      [Server] Checking xtoken list " << xlist_idx
                << " (" << xtoken_list.size() << " xtokens)" << std::endl;

      for (const auto& xtoken : xtoken_list) {
        // Parse xtoken: keyword|beta
        size_t xpos = xtoken.rfind('|');
        if (xpos == std::string::npos) continue;

        std::string keyword = xtoken.substr(0, xpos);
        std::string beta_str = xtoken.substr(xpos + 1);
        int beta = std::stoi(beta_str);

        // Check if this document actually has this keyword
        auto doc_kw_it = DocKeywords.find(id);
        if (doc_kw_it != DocKeywords.end()) {
          if (doc_kw_it->second.find(keyword) != doc_kw_it->second.end()) {
            passes_this_keyword = true;
            eop.cnt++;
            std::cout << "        [Server] xtoken matched (keyword=" << keyword
                      << ", beta=" << beta << ") - doc has keyword" << std::endl;
            break;
          } else {
            std::cout << "        [Server] xtoken (keyword=" << keyword
                      << ", beta=" << beta << ") - doc MISSING keyword" << std::endl;
          }
        }
      }

      if (!passes_this_keyword) {
        passes_all = false;
        std::cout << "      [Server] Failed xtoken list " << xlist_idx << std::endl;
        break;
      }
    }

    if (passes_all) {
      std::cout << "    [Server] Candidate " << i << " PASSED all filters" << std::endl;
      sEop_list.push_back(eop);
    } else {
      std::cout << "    [Server] Candidate " << i << " FAILED filters" << std::endl;
    }
  }

  return sEop_list;
}
