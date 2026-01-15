#pragma once

#include <iostream>

#include "core/Experiment.hpp"

namespace mcodxt {

class McOdxtExperiment : public core::Experiment {
public:
    McOdxtExperiment();
    ~McOdxtExperiment() override;

    int setup() override;
    void run() override;
    void teardown() override;
    std::string getName() const override;
};

}  // namespace mcodxt
