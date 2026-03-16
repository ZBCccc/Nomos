#include "benchmark/ExperimentProgress.hpp"

#include <algorithm>
#include <iomanip>
#include <ostream>
#include <sstream>

namespace nomos {
namespace benchmark {

namespace {

double computeProgressPercent(size_t completed_work_units,
                              size_t total_work_units) {
  if (total_work_units == 0) {
    return 100.0;
  }
  return 100.0 * static_cast<double>(completed_work_units) /
         static_cast<double>(total_work_units);
}

double computeEtaSeconds(size_t completed_work_units, size_t total_work_units,
                         double elapsed_seconds) {
  if (completed_work_units == 0 || completed_work_units >= total_work_units) {
    return 0.0;
  }

  return elapsed_seconds *
         (static_cast<double>(total_work_units - completed_work_units) /
          static_cast<double>(completed_work_units));
}

}  // namespace

ExperimentProgress::ExperimentProgress(const std::string& experiment_name,
                                       size_t total_work_units,
                                       std::ostream* output)
    : experiment_name_(experiment_name),
      total_work_units_(total_work_units),
      completed_work_units_(0),
      output_(output),
      update_interval_(std::chrono::seconds(1)),
      start_time_(Clock::now()),
      last_emit_time_(start_time_) {}

void ExperimentProgress::advance(const std::string& dataset,
                                 const std::string& scheme,
                                 size_t current_repeat, size_t total_repeats,
                                 size_t current_point, size_t total_points) {
  completed_work_units_ = std::min(
      total_work_units_, completed_work_units_ + static_cast<size_t>(1));

  const TimePoint now_time = Clock::now();
  const bool is_final_update =
      (completed_work_units_ >= total_work_units_) ||
      (total_points > 0 && current_point >= total_points);
  if (!shouldEmit(now_time, is_final_update)) {
    return;
  }

  const double elapsed_seconds =
      std::chrono::duration<double>(now_time - start_time_).count();
  if (output_ != NULL) {
    (*output_) << formatLine(experiment_name_, dataset, scheme, current_repeat,
                             total_repeats, current_point, total_points,
                             completed_work_units_, total_work_units_,
                             elapsed_seconds)
               << std::endl;
  }
  last_emit_time_ = now_time;
}

void ExperimentProgress::setUpdateInterval(std::chrono::milliseconds interval) {
  update_interval_ = interval;
}

std::string ExperimentProgress::formatLine(
    const std::string& experiment_name, const std::string& dataset,
    const std::string& scheme, size_t current_repeat, size_t total_repeats,
    size_t current_point, size_t total_points, size_t completed_work_units,
    size_t total_work_units, double elapsed_seconds) {
  const double progress_percent =
      computeProgressPercent(completed_work_units, total_work_units);
  const double eta_seconds = computeEtaSeconds(
      completed_work_units, total_work_units, elapsed_seconds);

  std::ostringstream oss;
  oss << "[" << experiment_name << "]"
      << " dataset=" << dataset << " scheme=" << scheme << " repeat "
      << current_repeat << "/" << total_repeats << " point " << current_point
      << "/" << total_points << " overall " << completed_work_units << "/"
      << total_work_units << " (" << std::fixed << std::setprecision(1)
      << progress_percent << "%)"
      << " elapsed " << elapsed_seconds << "s"
      << " eta " << eta_seconds << "s";
  return oss.str();
}

bool ExperimentProgress::shouldEmit(const TimePoint& now_time,
                                    bool is_final) const {
  return completed_work_units_ == 1 || is_final ||
         (now_time - last_emit_time_) >= update_interval_;
}

}  // namespace benchmark
}  // namespace nomos
