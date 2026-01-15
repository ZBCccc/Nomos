#pragma once

#include <string>

namespace core {

/**
 * @brief Abstract base class for all experiments.
 */
class Experiment {
public:
    virtual ~Experiment() = default;

    /**
     * @brief Setup the experiment environment.
     * @return 0 on success, non-zero on failure.
     */
    virtual int setup() = 0;

    /**
     * @brief Run the experiment.
     */
    virtual void run() = 0;

    /**
     * @brief Teardown the experiment and clean up resources.
     */
    virtual void teardown() = 0;

    /**
     * @brief Get the name of the experiment.
     */
    virtual std::string getName() const = 0;
};

}  // namespace core
