#pragma once

#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>

#include "Experiment.hpp"

namespace core {

class ExperimentFactory {
public:
    using CreatorFunc = std::function<std::unique_ptr<Experiment>()>;

    static ExperimentFactory& instance() {
        static ExperimentFactory instance;
        return instance;
    }

    void registerExperiment(const std::string& name, CreatorFunc creator) {
        m_registry.emplace(name, std::move(creator));
    }

    std::unique_ptr<Experiment> createExperiment(const std::string& name) {
        auto it = m_registry.find(name);
        if (it != m_registry.end()) {
            return it->second();
        }
        throw std::runtime_error("Unknown experiment: " + name);
    }

    const std::map<std::string, CreatorFunc>& getRegistry() const {
        return m_registry;
    }

private:
    ExperimentFactory() = default;
    std::map<std::string, CreatorFunc> m_registry;
};

}  // namespace core