#include "benchmark/BenchmarkUtils.hpp"

#include <sys/stat.h>
#include <sys/types.h>

#include <cctype>
#include <cerrno>
#include <chrono>
#include <stdexcept>

namespace nomos {
namespace benchmark {

std::string joinPath(const std::string& base, const std::string& child) {
  if (base.empty() || base == ".") {
    return child;
  }
  if (child.empty()) {
    return base;
  }
  if (base[base.size() - 1] == '/') {
    return base + child;
  }
  return base + "/" + child;
}

void ensureDirectory(const std::string& path) {
  if (path.empty()) {
    return;
  }

  if (path == "/") {
    return;
  }

  struct stat st;
  if (stat(path.c_str(), &st) == 0) {
    if (!S_ISDIR(st.st_mode)) {
      throw std::runtime_error("Path exists but is not a directory: " + path);
    }
    return;
  }

  std::string parent = path;
  std::string::size_type slash = parent.find_last_of('/');
  if (slash != std::string::npos && slash > 0) {
    parent = parent.substr(0, slash);
    ensureDirectory(parent);
  }

  if (mkdir(path.c_str(), 0755) != 0 && errno != EEXIST) {
    throw std::runtime_error("Failed to create directory: " + path);
  }
}

template <typename ClockDuration>
double durationToMilliseconds(const ClockDuration& duration) {
  return std::chrono::duration<double, std::milli>(duration).count();
}

// Explicit instantiations for common clock durations
template double durationToMilliseconds(const std::chrono::nanoseconds&);
template double durationToMilliseconds(const std::chrono::microseconds&);
template double durationToMilliseconds(const std::chrono::milliseconds&);
template double durationToMilliseconds(const std::chrono::seconds&);

std::vector<std::string> makeSharedDocIds(size_t count) {
  std::vector<std::string> docs;
  docs.reserve(count);
  for (size_t i = 0; i < count; ++i) {
    docs.push_back("shared_doc_" + std::to_string(i + 1));
  }
  return docs;
}

std::string normalizeSchemeFilter(const std::string& scheme_filter) {
  std::string normalized;
  normalized.reserve(scheme_filter.size());
  for (size_t i = 0; i < scheme_filter.size(); ++i) {
    normalized.push_back(static_cast<char>(
        std::tolower(static_cast<unsigned char>(scheme_filter[i]))));
  }

  if (normalized.empty() || normalized == "all") {
    return "all";
  }
  if (normalized == "nomos") {
    return "nomos";
  }
  if (normalized == "mc-odxt" || normalized == "mcodxt") {
    return "mc-odxt";
  }
  if (normalized == "vqnomos" || normalized == "vq-nomos" ||
      normalized == "verifiable") {
    return "vqnomos";
  }

  throw std::invalid_argument("Unsupported scheme filter: " + scheme_filter);
}

}  // namespace benchmark
}  // namespace nomos
