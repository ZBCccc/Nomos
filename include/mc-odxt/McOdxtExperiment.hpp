#pragma once

#include <string>

#include "core/Experiment.hpp"
#include "mc-odxt/McOdxtClient.hpp"
#include "mc-odxt/McOdxtGatekeeper.hpp"
#include "mc-odxt/McOdxtServer.hpp"
#include "mc-odxt/McOdxtTypes.hpp"

namespace mcodxt {

class McOdxtExperiment : public core::Experiment {
 public:
  McOdxtExperiment();
  ~McOdxtExperiment() override;

  int setup() override;
  void run() override;
  void teardown() override;
  std::string getName() const override;

 private:
  McOdxtGatekeeper m_gatekeeper;
  McOdxtClient m_client;
  McOdxtServer m_server;
};

}  // namespace mcodxt
