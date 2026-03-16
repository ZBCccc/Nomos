#pragma once

#include <chrono>
#include <cstddef>
#include <iosfwd>
#include <string>

namespace nomos {
namespace benchmark {

/**
 * @brief Throttled progress logger for long-running benchmark experiments.
 */
class ExperimentProgress {
 public:
  typedef std::chrono::steady_clock Clock;
  typedef Clock::time_point TimePoint;

  /**
   * @brief Construct a progress logger with a fixed total amount of work.
   * @param experiment_name Human-readable experiment name shown in logs.
   * @param total_work_units Total point count across datasets, schemes, and
   * repeats.
   * @param output Stream used for progress output.
   */
  ExperimentProgress(const std::string& experiment_name,
                     size_t total_work_units, std::ostream* output);

  /**
   * @brief Report one completed work unit.
   * @param dataset Dataset currently being processed.
   * @param scheme Scheme currently being benchmarked.
   * @param current_repeat 1-based repeat index for the active sweep.
   * @param total_repeats Total repeat count configured for the sweep.
   * @param current_point 1-based point index within the active sweep.
   * @param total_points Total number of points in the active sweep.
   */
  void advance(const std::string& dataset, const std::string& scheme,
               size_t current_repeat, size_t total_repeats,
               size_t current_point, size_t total_points);

  /**
   * @brief Override the minimum interval between emitted progress lines.
   * Primarily intended for tests.
   */
  void setUpdateInterval(std::chrono::milliseconds interval);

  /**
   * @brief Format a progress line for logging or tests.
   */
  static std::string formatLine(
      const std::string& experiment_name, const std::string& dataset,
      const std::string& scheme, size_t current_repeat, size_t total_repeats,
      size_t current_point, size_t total_points, size_t completed_work_units,
      size_t total_work_units, double elapsed_seconds);

 private:
  bool shouldEmit(const TimePoint& now_time, bool is_final) const;

  std::string experiment_name_;
  size_t total_work_units_;
  size_t completed_work_units_;
  std::ostream* output_;
  std::chrono::milliseconds update_interval_;
  TimePoint start_time_;
  TimePoint last_emit_time_;
};

}  // namespace benchmark
}  // namespace nomos
