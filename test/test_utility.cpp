#include "backend/dd/DDSimDebug.hpp"
#include "backend/debug.h"
#include "common.h"
#include "utils_test.hpp"

#include <cstddef>
#include <gtest/gtest.h>
#include <map>
#include <string>
#include <utility>

class UtilityTest : public testing::Test {
  void SetUp() override {
    createDDSimulationState(&ddState);
    state = &ddState.interface;
  }

protected:
  DDSimulationState ddState;
  SimulationState* state = nullptr;

  void loadFromFile(const std::string& testName) {
    const auto code = readFromCircuitsPath(testName);
    state->loadCode(state, code.c_str());
  }
};

TEST_F(UtilityTest, GetInstructionCount) {
  loadFromFile("complex-jumps");
  ASSERT_EQ(state->getInstructionCount(state), 15);
}

TEST_F(UtilityTest, GetInstructionPosition) {
  loadFromFile("complex-jumps");

  const std::map<size_t, std::pair<size_t, size_t>> expected = {
      {0, {0, 9}},     {1, {38, 112}},  {2, {79, 88}},
      {3, {112, 112}}, {4, {150, 298}}, {12, {452, 477}}};
  for (const auto& [instruction, expectedPosition] : expected) {
    size_t start = 0;
    size_t end = 0;
    state->getInstructionPosition(state, instruction, &start, &end);
    ASSERT_EQ(start, expectedPosition.first)
        << "Failed for instruction " << instruction;
    ASSERT_EQ(end, expectedPosition.second)
        << "Failed for instruction " << instruction;
  }
}

TEST_F(UtilityTest, BadInstructionPosition) {
  loadFromFile("complex-jumps");

  size_t start = 0;
  size_t end = 0;
  ASSERT_EQ(state->getInstructionPosition(state, 100, &start, &end), ERROR);
}
