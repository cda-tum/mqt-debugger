#include "backend/dd/DDSimDebug.hpp"
#include "backend/debug.h"
#include "backend/diagnostics.h"
#include "utils_test.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>
#include <map>
#include <set>
#include <string>
#include <vector>

class DiagnosticsTest : public testing::Test {
  void SetUp() override {
    createDDSimulationState(&ddState);
    state = &ddState.interface;
    diagnostics = state->getDiagnostics(state);
  }

protected:
  DDSimulationState ddState;
  SimulationState* state = nullptr;
  Diagnostics* diagnostics = nullptr;

  void loadFromFile(const std::string& testName) {
    const auto code = readFromCircuitsPath(testName);
    state->loadCode(state, code.c_str());
  }
};

TEST_F(DiagnosticsTest, DataDependencies) {
  loadFromFile("failing-assertions");
  const std::map<size_t, std::set<size_t>> expected = {
      {1, {1}},
      {2, {1, 2}},
      {3, {1, 2, 3}},
      {4, {1, 2, 4}},
      {5, {1, 2, 3, 4, 5}},
      {6, {1, 2, 3, 4, 6}},
      {7, {1, 2, 4, 7}},
      {8, {1, 2, 4, 7, 8}},
      {9, {1, 2, 4, 9}},
      {10, {1, 2, 3, 4, 7, 8, 10}}};

  for (const auto& [instruction, expectedDependencies] : expected) {
    std::vector<uint8_t> dependencies(state->getInstructionCount(state), 0);
    // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
    diagnostics->getDataDependencies(
        diagnostics, instruction, reinterpret_cast<bool*>(dependencies.data()));
    // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
    std::set<size_t> dependenciesSet;
    for (size_t i = 0; i < dependencies.size(); ++i) {
      if (dependencies[i] != 0) {
        dependenciesSet.insert(i);
      }
    }
    ASSERT_EQ(dependenciesSet, expectedDependencies)
        << "Failed for instruction " << instruction;
  }
}

TEST_F(DiagnosticsTest, ControlAlwaysZeroTest) {
  loadFromFile("failing-assertions");
  state->runSimulation(state);
  std::array<ErrorCause, 10> problems{};
  const size_t count =
      diagnostics->potentialErrorCauses(diagnostics, problems.data(), 10);
  ASSERT_EQ(count, 1);
  ASSERT_EQ(problems[0].type, ErrorCauseType::ControlAlwaysZero);
  ASSERT_EQ(problems[0].instruction, 4);
}

TEST_F(DiagnosticsTest, MaximumControlAlwaysZeroTest) {
  loadFromFile("failing-assertions-multiple-zero-controls");
  state->runSimulation(state);
  std::array<ErrorCause, 10> problems{};
  ASSERT_EQ(diagnostics->potentialErrorCauses(diagnostics, problems.data(), 10),
            3);
  ASSERT_EQ(diagnostics->potentialErrorCauses(diagnostics, problems.data(), 3),
            3);
  ASSERT_EQ(diagnostics->potentialErrorCauses(diagnostics, problems.data(), 2),
            2);
}

TEST_F(DiagnosticsTest, MissingInteraction) {
  loadFromFile("failing-assertions-missing-interaction");
  state->runSimulation(state);
  ASSERT_EQ(state->getCurrentInstruction(state), 7);
  std::array<ErrorCause, 10> problems{};
  const size_t count =
      diagnostics->potentialErrorCauses(diagnostics, problems.data(), 10);
  ASSERT_EQ(count, 1);
  ASSERT_EQ(problems[0].type, ErrorCauseType::MissingInteraction);
  ASSERT_EQ(problems[0].instruction, 7);
}

TEST_F(DiagnosticsTest, MaximumMissingInteraction) {
  loadFromFile("failing-assertions-multiple-missing-interaction");
  state->runSimulation(state);
  ASSERT_EQ(state->getCurrentInstruction(state), 2);
  std::array<ErrorCause, 20> problems{};
  ASSERT_EQ(diagnostics->potentialErrorCauses(diagnostics, problems.data(), 20),
            10);
  ASSERT_EQ(diagnostics->potentialErrorCauses(diagnostics, problems.data(), 10),
            10);
  ASSERT_EQ(diagnostics->potentialErrorCauses(diagnostics, problems.data(), 3),
            3);
}

TEST_F(DiagnosticsTest, MaximumMultipleCauses) {
  loadFromFile("failing-assertions-multiple-causes");
  state->runSimulation(state);
  ASSERT_EQ(state->getCurrentInstruction(state), 3);
  std::array<ErrorCause, 20> problems{};
  const std::vector<size_t> maxErrors = {20, 8, 7, 4};
  const std::vector<std::pair<size_t, std::vector<ErrorCauseType>>>
      expectedTypes = {
          {9,
           {MissingInteraction, MissingInteraction, MissingInteraction,
            MissingInteraction, MissingInteraction, MissingInteraction,
            MissingInteraction, ControlAlwaysZero, ControlAlwaysZero}},
          {8,
           {MissingInteraction, MissingInteraction, MissingInteraction,
            MissingInteraction, MissingInteraction, MissingInteraction,
            MissingInteraction, ControlAlwaysZero}},
          {7,
           {MissingInteraction, MissingInteraction, MissingInteraction,
            MissingInteraction, MissingInteraction, MissingInteraction,
            MissingInteraction}},
          {4,
           {MissingInteraction, MissingInteraction, MissingInteraction,
            MissingInteraction}},
      };
  for (size_t i = 0; i < maxErrors.size(); i++) {
    const auto [expectedCount, types] = expectedTypes[i];
    ASSERT_EQ(diagnostics->potentialErrorCauses(diagnostics, problems.data(),
                                                maxErrors[i]),
              expectedCount);
    for (size_t j = 0; j < expectedCount; j++) {
      ASSERT_EQ(problems.at(j).type, types[j]);
    }
  }
}

TEST_F(DiagnosticsTest, NoFailedAssertions) {
  loadFromFile("complex-jumps");
  state->runSimulation(state);
  std::array<ErrorCause, 5> problems{};
  ASSERT_EQ(diagnostics->potentialErrorCauses(diagnostics, problems.data(), 5),
            0);
}
