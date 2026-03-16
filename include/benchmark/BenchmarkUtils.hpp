#pragma once

#include <string>
#include <vector>

namespace nomos {
namespace benchmark {

std::string joinPath(const std::string& base, const std::string& child);

void ensureDirectory(const std::string& path);

template <typename ClockDuration>
double durationToMilliseconds(const ClockDuration& duration);

std::vector<std::string> makeSharedDocIds(size_t count);

std::string normalizeSchemeFilter(const std::string& scheme_filter);

}  // namespace benchmark
}  // namespace nomos
