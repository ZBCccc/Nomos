#pragma once

#include <cassert>
#include <iostream>
#include <vector>

#include "core/Experiment.hpp"
#include "vq-nomos/Client.hpp"
#include "vq-nomos/Gatekeeper.hpp"
#include "vq-nomos/Server.hpp"

namespace vqnomos {

class VQNomosExperiment : public core::Experiment {
 public:
  VQNomosExperiment() {}
  ~VQNomosExperiment() override = default;

  int setup() override {
    std::cout << "[VQNomos] Setting up..." << std::endl;

    if (core_init() != RLC_OK) {
      std::cerr << "Failed to initialize RELIC" << std::endl;
      return -1;
    }
    if (pc_param_set_any() != RLC_OK) {
      std::cerr << "Failed to set RELIC parameters" << std::endl;
      return -1;
    }

    if (m_gatekeeper.setup(10, 1024) != 0) {
      return -1;
    }

    const Anchor initial_anchor = m_gatekeeper.getCurrentAnchor();
    if (m_client.setup(m_gatekeeper.getPublicKeyPem(), initial_anchor, 1024,
                       10) != 0) {
      return -1;
    }
    m_server.setup(m_gatekeeper.getKm(), initial_anchor, 1024);
    return 0;
  }

  void run() override {
    std::cout << "[VQNomos] Running experiment..." << std::endl;

    m_server.update(m_gatekeeper.update(OP_ADD, "doc1", "crypto"));
    m_server.update(m_gatekeeper.update(OP_ADD, "doc1", "security"));
    m_server.update(m_gatekeeper.update(OP_ADD, "doc2", "security"));
    m_server.update(m_gatekeeper.update(OP_ADD, "doc2", "privacy"));
    m_server.update(m_gatekeeper.update(OP_ADD, "doc3", "crypto"));
    m_server.update(m_gatekeeper.update(OP_ADD, "doc3", "blockchain"));

    const std::vector<std::string> query = {"crypto", "security"};
    const TokenRequest token_request =
        m_client.genToken(query, m_gatekeeper.getUpdateCounts());
    const SearchToken token = m_gatekeeper.genToken(token_request);
    const SearchRequest search_request =
        m_client.prepareSearch(token, token_request);
    const SearchResponse response = m_server.search(search_request, token);
    const VerificationResult verification =
        m_client.decryptAndVerify(response, token, token_request);

    assert(verification.accepted);
    assert(verification.ids.size() == 1);
    assert(verification.ids[0] == "doc1");

    std::cout << "[VQNomos] Verified result count: " << verification.ids.size()
              << std::endl;
    for (size_t i = 0; i < verification.ids.size(); ++i) {
      std::cout << "  - " << verification.ids[i] << std::endl;
    }
  }

  void teardown() override {
    std::cout << "[VQNomos] Teardown complete" << std::endl;
    core_clean();
  }

  std::string getName() const override { return "VQNomos"; }

 private:
  Gatekeeper m_gatekeeper;
  Client m_client;
  Server m_server;
};

}  // namespace vqnomos
