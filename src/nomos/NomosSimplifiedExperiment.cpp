#include "nomos/NomosSimplifiedExperiment.hpp"

#include <cassert>
#include <iostream>

namespace nomos {

void NomosSimplifiedExperiment::run() {
  std::cout << "\n[Nomos-Simplified] Running experiment..." << std::endl;

  // Test 1: Add documents with keywords
  std::cout << "\n--- Test 1: Adding documents ---" << std::endl;

  // Add doc1 with keywords: crypto, security
  auto meta1 = m_gatekeeper.update(OP_ADD, "doc1", "crypto");
  m_server.update(meta1);
  std::cout << "Added: doc1 -> crypto" << std::endl;

  auto meta2 = m_gatekeeper.update(OP_ADD, "doc1", "security");
  m_server.update(meta2);
  std::cout << "Added: doc1 -> security" << std::endl;

  // Add doc2 with keywords: security, privacy
  auto meta3 = m_gatekeeper.update(OP_ADD, "doc2", "security");
  m_server.update(meta3);
  std::cout << "Added: doc2 -> security" << std::endl;

  auto meta4 = m_gatekeeper.update(OP_ADD, "doc2", "privacy");
  m_server.update(meta4);
  std::cout << "Added: doc2 -> privacy" << std::endl;

  // Add doc3 with keywords: crypto, blockchain
  auto meta5 = m_gatekeeper.update(OP_ADD, "doc3", "crypto");
  m_server.update(meta5);
  std::cout << "Added: doc3 -> crypto" << std::endl;

  auto meta6 = m_gatekeeper.update(OP_ADD, "doc3", "blockchain");
  m_server.update(meta6);
  std::cout << "Added: doc3 -> blockchain" << std::endl;

  std::cout << "TSet size: " << m_server.getTSetSize() << std::endl;
  std::cout << "XSet size: " << m_server.getXSetSize() << std::endl;

  // Test 2: Search for [crypto, security]
  std::cout << "\n--- Test 2: Search [crypto, security] ---" << std::endl;
  {
    std::vector<std::string> query = {"crypto", "security"};
    auto token = m_client.genTokenSimplified(query, m_gatekeeper);
    auto req =
        m_client.prepareSearch(token, query, m_gatekeeper.getUpdateCounts());
    auto results = m_server.search(req);
    auto ids = m_client.decryptResults(results, token);

    std::cout << "Query: [crypto, security]" << std::endl;
    std::cout << "Results: " << ids.size() << " documents" << std::endl;
    for (const auto& id : ids) {
      std::cout << "  - " << id << std::endl;
    }

    // Verify: should return doc1 only
    assert(ids.size() == 1);
    assert(ids[0] == "doc1");
    std::cout << "✓ Test passed: found doc1" << std::endl;
  }

  // Test 3: Search for [security, privacy]
  std::cout << "\n--- Test 3: Search [security, privacy] ---" << std::endl;
  {
    std::vector<std::string> query = {"security", "privacy"};
    auto token = m_client.genTokenSimplified(query, m_gatekeeper);
    auto req =
        m_client.prepareSearch(token, query, m_gatekeeper.getUpdateCounts());
    auto results = m_server.search(req);
    auto ids = m_client.decryptResults(results, token);

    std::cout << "Query: [security, privacy]" << std::endl;
    std::cout << "Results: " << ids.size() << " documents" << std::endl;
    for (const auto& id : ids) {
      std::cout << "  - " << id << std::endl;
    }

    // Verify: should return doc2 only
    assert(ids.size() == 1);
    assert(ids[0] == "doc2");
    std::cout << "✓ Test passed: found doc2" << std::endl;
  }

  // Test 4: Search for [crypto] (single keyword)
  std::cout << "\n--- Test 4: Search [crypto] ---" << std::endl;
  {
    std::vector<std::string> query = {"crypto"};
    auto token = m_client.genTokenSimplified(query, m_gatekeeper);
    auto req =
        m_client.prepareSearch(token, query, m_gatekeeper.getUpdateCounts());
    auto results = m_server.search(req);
    auto ids = m_client.decryptResults(results, token);

    std::cout << "Query: [crypto]" << std::endl;
    std::cout << "Results: " << ids.size() << " documents" << std::endl;
    for (const auto& id : ids) {
      std::cout << "  - " << id << std::endl;
    }

    // Verify: should return doc1 and doc3
    assert(ids.size() == 2);
    std::cout << "✓ Test passed: found 2 documents" << std::endl;
  }

  // Test 5: Search for non-existent keyword
  std::cout << "\n--- Test 5: Search [nonexistent] ---" << std::endl;
  {
    std::vector<std::string> query = {"nonexistent"};
    auto token = m_client.genTokenSimplified(query, m_gatekeeper);
    auto req =
        m_client.prepareSearch(token, query, m_gatekeeper.getUpdateCounts());
    auto results = m_server.search(req);
    auto ids = m_client.decryptResults(results, token);

    std::cout << "Query: [nonexistent]" << std::endl;
    std::cout << "Results: " << ids.size() << " documents" << std::endl;

    // Verify: should return nothing
    assert(ids.size() == 0);
    std::cout << "✓ Test passed: no results" << std::endl;
  }

  std::cout << "\n[Nomos-Simplified] All tests passed! ✓" << std::endl;
}

}  // namespace nomos
