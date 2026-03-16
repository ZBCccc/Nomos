#include <gtest/gtest.h>

#include <chrono>
#include <sstream>
#include <string>

#include "benchmark/ExperimentProgress.hpp"

namespace {

size_t countNonEmptyLines(const std::string& text) {
  std::istringstream input(text);
  std::string line;
  size_t count = 0;
  while (std::getline(input, line)) {
    if (!line.empty()) {
      ++count;
    }
  }
  return count;
}

}  // namespace

TEST(ExperimentProgressTest, FormatLineIncludesOverallProgressAndEta) {
  const std::string line = nomos::benchmark::ExperimentProgress::formatLine(
      "ClientSearchFixedW1", "Crime", "Nomos", 1, 2, 25, 100, 150, 600, 12.5);

  EXPECT_NE(line.find("[ClientSearchFixedW1]"), std::string::npos);
  EXPECT_NE(line.find("dataset=Crime"), std::string::npos);
  EXPECT_NE(line.find("scheme=Nomos"), std::string::npos);
  EXPECT_NE(line.find("repeat 1/2"), std::string::npos);
  EXPECT_NE(line.find("point 25/100"), std::string::npos);
  EXPECT_NE(line.find("overall 150/600 (25.0%)"), std::string::npos);
  EXPECT_NE(line.find("elapsed 12.5s"), std::string::npos);
  EXPECT_NE(line.find("eta 37.5s"), std::string::npos);
}

TEST(ExperimentProgressTest, AdvanceEmitsFirstAndFinalLine) {
  std::ostringstream output;
  nomos::benchmark::ExperimentProgress progress("ClientSearchFixedW2", 3,
                                                &output);
  progress.setUpdateInterval(std::chrono::hours(1));

  progress.advance("Enron", "Nomos", 1, 1, 1, 3);
  progress.advance("Enron", "Nomos", 1, 1, 2, 3);
  progress.advance("Enron", "Nomos", 1, 1, 3, 3);

  const std::string text = output.str();
  EXPECT_EQ(countNonEmptyLines(text), static_cast<size_t>(2));
  EXPECT_NE(text.find("overall 1/3"), std::string::npos);
  EXPECT_NE(text.find("overall 3/3 (100.0%)"), std::string::npos);
}

TEST(ExperimentProgressTest, AdvanceEmitsAtSweepBoundaryWhenThrottled) {
  std::ostringstream output;
  nomos::benchmark::ExperimentProgress progress("ClientSearchFixedW1", 4,
                                                &output);
  progress.setUpdateInterval(std::chrono::hours(1));

  progress.advance("Crime", "Nomos", 1, 1, 1, 2);
  progress.advance("Crime", "Nomos", 1, 1, 2, 2);
  progress.advance("Crime", "MC-ODXT", 1, 1, 1, 2);

  const std::string text = output.str();
  EXPECT_EQ(countNonEmptyLines(text), static_cast<size_t>(2));
  EXPECT_NE(text.find("overall 2/4 (50.0%)"), std::string::npos);
}
