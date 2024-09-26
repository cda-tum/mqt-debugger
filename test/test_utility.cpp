/**
 * @file test_utility.cpp
 * @brief Test the functionality of utility functions provided by the debugger.
 */
#include "backend/dd/DDSimDebug.hpp"
#include "backend/debug.h"
#include "common.h"
#include "utils_test.hpp"

#include <cstddef>
#include <gtest/gtest.h>
#include <map>
#include <string>
#include <utility>

/**
 * @brief Fixture for testing the correctness of utility functions.
 *
 * This fixture creates a DDSimulationState and allows to load code from files
 * in the `circuits` directory.
 */
class UtilityTest : public testing::Test {
  void SetUp() override {
    createDDSimulationState(&ddState);
    state = &ddState.interface;
  }

protected:
  /**
   * @brief The DDSimulationState to use for testing.
   */
  DDSimulationState ddState;
  /**
   * @brief A reference to the SimulationState interface for easier access.
   */
  SimulationState* state = nullptr;

  /**
   * @brief Load the code from the file with the given name.
   *
   * The given file should be located in the `circuits` directory and use the
   * `.qasm` extension.
   * @param testName The name of the file to load (not including the `circuits`
   * directory path and the extension).
   */
  void loadFromFile(const std::string& testName) {
    const auto code = readFromCircuitsPath(testName);
    state->loadCode(state, code.c_str());
  }
};

/**
 * @test Test the retrieval of the number of instructions in the loaded code.
 */
TEST_F(UtilityTest, GetInstructionCount) {
  loadFromFile("complex-jumps");
  ASSERT_EQ(state->getInstructionCount(state), 15);
}

/**
 * @test Test the retrieval of the position of an instruction in the loaded
 * code.
 */
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

/**
 * @test Test that an error is returned when trying to access the position of an
 * instruction with index larger than the total number of instructions.
 */
TEST_F(UtilityTest, BadInstructionPosition) {
  loadFromFile("complex-jumps");

  size_t start = 0;
  size_t end = 0;
  ASSERT_EQ(state->getInstructionPosition(state, 100, &start, &end), ERROR);
}
