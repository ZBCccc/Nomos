#pragma once

#include <iostream>
#include <memory>

#include "core/Experiment.hpp"
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
    void runBenchmark();
    
    std::unique_ptr<McOdxtGatekeeper> m_gatekeeper;
    std::unique_ptr<McOdxtServer> m_server;
    std::unique_ptr<McOdxtClient> m_client;
    std::unique_ptr<McOdxtDataOwner> m_data_owner;
};

}  // namespace mcodxt
