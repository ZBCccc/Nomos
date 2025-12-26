#pragma once

#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

#include "Experiment.hpp"

namespace core {

class ExperimentFactory {
public:
    using CreatorFunc = std::function<std::unique_ptr<Experiment>()>;

    static ExperimentFactory& instance() {
        static ExperimentFactory instance;
        return instance;
    }

    void registerExperiment(std::string_view name, CreatorFunc creator) {
        m_registry.emplace(name, std::move(creator));
    }

    [[nodiscard]] std::unique_ptr<Experiment> createExperiment(std::string_view name) {
        auto it = m_registry.find(std::string(name));
        if (it != m_registry.end()) {
            return it->second();
        }
        throw std::runtime_error("Unknown experiment: " + std::string(name));
    }

    [[nodiscard]] const std::map<std::string, CreatorFunc>& getRegistry() const {
        return m_registry;
    }

private:
    ExperimentFactory() = default;
    std::map<std::string, CreatorFunc> m_registry;
};

}  // namespace core
