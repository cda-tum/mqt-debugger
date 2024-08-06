#include "backend/dd/DDSimDebug.hpp"
#include "backend/debug.h"
#include "utils_test.hpp"

#include <gtest/gtest.h>
#include <memory>

class SimulationTest : public testing::TestWithParam<std::string> {
  void SetUp() override {
    createDDSimulationState(&ddState);
    state = &ddState.interface;
    loadFromFile(GetParam());
  }

protected:
  DDSimulationState ddState;
  SimulationState* state = nullptr;

  void loadFromFile(const std::string& testName) {
    const auto code = readFromCircuitsPath(testName);
    state->loadCode(state, code.c_str());
  }

  void moveAndCheck(
      const std::vector<std::pair<const std::string, size_t>>& movements) {
    size_t step = 0;
    for (const auto& movement : movements) {
      Result (*movementFunction)(SimulationState*) = nullptr;
      if (movement.first == "sf") {
        movementFunction = state->stepForward;
      } else if (movement.first == "of") {
        movementFunction = state->stepOverForward;
      } else if (movement.first == "uf") {
        movementFunction = state->stepOutForward;
      } else if (movement.first == "rf") {
        movementFunction = state->runSimulation;
      } else if (movement.first == "sb") {
        movementFunction = state->stepBackward;
      } else if (movement.first == "ob") {
        movementFunction = state->stepOverBackward;
      } else if (movement.first == "ub") {
        movementFunction = state->stepOutBackward;
      } else if (movement.first == "rb") {
        movementFunction = state->runSimulationBackward;
      } else if (movement.first == "assertion") {
        ASSERT_TRUE(state->didAssertionFail(state))
            << "Expected assertion to fail at step " << step << " in "
            << GetParam() << "\n";
        step++;
        continue;
      } else {
        FAIL() << "Unknown movement type " << movement.first << "\n";
      }
      ASSERT_EQ(movementFunction(state), Result::OK)
          << "Movement " << movement.first << " failed at step " << step
          << " in " << GetParam() << "\n";
      ASSERT_EQ(state->getCurrentInstruction(state), movement.second)
          << "Movement " << movement.first
          << " did not reach expected instruction " << movement.second
          << " at step " << step << " in " << GetParam() << "\n";
      step++;
    }
  }
};

TEST_P(SimulationTest, StepThroughCode) {
  const std::map<const std::string, std::vector<size_t>> expected = {
      {"complex-jumps", {0, 1,  4, 9,  12, 5, 6, 10, 2,  3,  11, 7, 10, 2,
                         3, 11, 8, 13, 10, 2, 3, 11, 14, 10, 2,  3, 11}},
      {"failing-assertions", {0, 1, 2, 3, 4, 5, 6, 6, 7, 8, 9, 9, 10}}};
  for (const auto exp : expected.at(GetParam())) {
    ASSERT_EQ(state->getCurrentInstruction(state), exp);
    state->stepForward(state);
  }
}

TEST_P(SimulationTest, StackTraceRetrieval) {
  const std::map<const std::string, std::vector<size_t>> expectedDepths = {
      {"complex-jumps", {1, 1, 1, 1, 1, 2, 2, 3, 4, 4, 3, 2, 3, 4,
                         4, 3, 2, 1, 2, 3, 3, 2, 1, 2, 3, 3, 2}},
      {"failing-assertions", {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}}};
  const std::map<const std::string, std::vector<std::vector<size_t>>>
      expectedStacks = {
          {"complex-jumps",
           {{0},
            {1},
            {4},
            {9},
            {12},
            {5, 12},
            {6, 12},
            {10, 6, 12},
            {2, 10, 6, 12},
            {3, 10, 6, 12},
            {11, 6, 12},
            {7, 12},
            {10, 7, 12},
            {2, 10, 7, 12},
            {3, 10, 7, 12},
            {11, 7, 12},
            {8, 12},
            {13},
            {10, 13},
            {2, 10, 13},
            {3, 10, 13},
            {11, 13},
            {14},
            {10, 14},
            {2, 10, 14},
            {3, 10, 14},
            {11, 14}}},
          {"failing-assertions",
           {{0}, {1}, {2}, {3}, {4}, {5}, {6}, {6}, {7}, {8}, {9}, {9}, {10}}}};

  for (size_t index = 0; index < expectedDepths.at(GetParam()).size();
       index++) {
    const auto exp = expectedDepths.at(GetParam())[index];
    size_t depth = 0;
    ASSERT_EQ(state->getStackDepth(state, &depth), Result::OK);
    ASSERT_EQ(depth, exp) << "Depth computation failed for instruction "
                          << state->getCurrentInstruction(state) << " at index "
                          << index << " in " << GetParam() << "\n";
    for (size_t depthToTest = 1; depthToTest <= depth; depthToTest++) {
      std::vector<size_t> stack(depthToTest);
      ASSERT_EQ(state->getStackTrace(state, depthToTest, stack.data()),
                Result::OK)
          << "Failed to get stack trace for depth " << depthToTest
          << " for instruction " << state->getCurrentInstruction(state)
          << " in " << GetParam() << "\n";
      const std::vector<size_t>& expectedStack =
          expectedStacks.at(GetParam()).at(index);
      for (size_t i = 0; i < depthToTest; i++) {
        ASSERT_EQ(stack[i], expectedStack[i])
            << "Failed for index " << i << " at depth " << depthToTest
            << " for instruction " << state->getCurrentInstruction(state)
            << " in " << GetParam() << "\n";
      }
    }

    state->stepForward(state);
  }
}

TEST_P(SimulationTest, TopLevelBreakpoints) {
  const std::map<const std::string, std::vector<size_t>> breakpointPositions = {
      {"complex-jumps", {174, 451, 488, 525}},
      {"failing-assertions", {58, 322, 374, 427, 487}}};
  const std::map<const std::string, std::vector<size_t>>
      expectedBreakpointPositions = {{"complex-jumps", {4, 12, 13, 14}},
                                     {"failing-assertions", {1, 5, 6, 7, 8}}};

  for (size_t index = 0; index < breakpointPositions.at(GetParam()).size();
       index++) {
    const auto breakpoint = breakpointPositions.at(GetParam())[index];
    size_t targetInstruction = 0;
    ASSERT_EQ(state->setBreakpoint(state, breakpoint, &targetInstruction),
              Result::OK)
        << "Failed to set breakpoint at instruction " << breakpoint << " in "
        << GetParam() << "\n";
    ASSERT_EQ(targetInstruction,
              expectedBreakpointPositions.at(GetParam())[index])
        << "Breakpoint set at wrong instruction for breakpoint " << breakpoint
        << " in " << GetParam() << "\n";
  }

  for (const auto instruction : expectedBreakpointPositions.at(GetParam())) {
    ASSERT_EQ(state->runSimulation(state), Result::OK)
        << "Failed to run simulation in " << GetParam() << "\n";
    while (state->didAssertionFail(state)) {
      ASSERT_EQ(state->runSimulation(state), Result::OK)
          << "Failed to run simulation in " << GetParam() << "\n";
    }
    ASSERT_EQ(state->getCurrentInstruction(state), instruction)
        << "Breakpoint not hit at expected instruction " << instruction
        << " in " << GetParam() << "\n";
    ASSERT_TRUE(state->wasBreakpointHit(state));
  }

  ASSERT_EQ(state->clearBreakpoints(state), Result::OK)
      << "Failed to clear breakpoints in " << GetParam() << "\n";
  ASSERT_EQ(state->runSimulationBackward(state), Result::OK);
  ASSERT_FALSE(state->wasBreakpointHit(state))
      << "Breakpoint hit after clearing in " << GetParam() << "\n";
  while (!state->isFinished(state)) {
    ASSERT_EQ(state->runSimulation(state), Result::OK);
    ASSERT_FALSE(state->wasBreakpointHit(state))
        << "Breakpoint hit after clearing in " << GetParam() << "\n";
  }
}

TEST_P(SimulationTest, PauseSimulation) {
  // The protocol allows us to be lenient with the 'pause' operation. We do not
  // need to pause immediately, instead we can pause at the next convenient
  // time. 'stepOver' and 'stepOut' methods therefore do not necessarily stop if
  // 'pause' was called before they started. `run` (in both directions), on the
  // other hand, will ALWAYS pause immediately, even if 'pause' was called
  // before they started.

  ASSERT_EQ(state->stepForward(state), Result::OK);

  size_t currentPosition = state->getCurrentInstruction(state);
  ASSERT_EQ(state->pauseSimulation(state), Result::OK);
  ASSERT_EQ(state->runSimulation(state), Result::OK);
  ASSERT_EQ(state->getCurrentInstruction(state), currentPosition)
      << "Simulation with 'run' continued after pause in " << GetParam()
      << "\n";
  ASSERT_EQ(state->stepOverForward(state), Result::OK);
  ASSERT_NE(state->getCurrentInstruction(state), currentPosition)
      << "Simulation still paused after second 'step over' in " << GetParam()
      << "\n";

  currentPosition = state->getCurrentInstruction(state);
  ASSERT_EQ(state->pauseSimulation(state), Result::OK);
  ASSERT_EQ(state->runSimulationBackward(state), Result::OK);
  ASSERT_EQ(state->getCurrentInstruction(state), currentPosition)
      << "Simulation with 'runBackward' continued after pause in " << GetParam()
      << "\n";
  ASSERT_EQ(state->stepOverForward(state), Result::OK);
  ASSERT_NE(state->getCurrentInstruction(state), currentPosition)
      << "Simulation still paused after second 'step over' in " << GetParam()
      << "\n";
}

TEST_P(SimulationTest, ResetSimulation) {
  for (size_t i = 0; i < 10; i++) {
    ASSERT_EQ(state->stepOverForward(state), Result::OK);
    ASSERT_EQ(state->stepOverForward(state), Result::OK);
    ASSERT_EQ(state->stepOverForward(state), Result::OK);
    ASSERT_EQ(state->resetSimulation(state), Result::OK);
    ASSERT_FALSE(state->canStepBackward(state));
    ASSERT_EQ(state->getCurrentInstruction(state), 0);
    ASSERT_EQ(state->runSimulation(state), Result::OK);
    while (state->didAssertionFail(state)) {
      ASSERT_EQ(state->runSimulation(state), Result::OK);
    }
    ASSERT_FALSE(state->canStepForward(state));
    ASSERT_EQ(state->resetSimulation(state), Result::OK);
    ASSERT_FALSE(state->canStepBackward(state));
    ASSERT_EQ(state->getCurrentInstruction(state), 0);
  }
}

TEST_P(SimulationTest, StepOver) {
  const std::map<const std::string,
                 std::vector<std::pair<const std::string, size_t>>>
      expected = {
          {"complex-jumps",
           {
               {"of", 1},  {"of", 4},  {"sf", 9},  {"sf", 12}, {"of", 13},
               {"ob", 12}, {"sf", 5},  {"of", 6},  {"of", 7},  {"ob", 6},
               {"sf", 10}, {"of", 11}, {"ob", 10}, {"sf", 2},  {"of", 3},
               {"of", 11}, {"of", 7},  {"sb", 11}, {"of", 7},  {"of", 8},
               {"of", 13}, {"sb", 8},  {"of", 13}, {"of", 14},
           }},
          {"failing-assertions",
           {{"of", 1},        {"of", 2},        {"of", 3},
            {"of", 4},        {"of", 5},        {"of", 6},
            {"of", 6},        {"assertion", 6}, {"of", 7},
            {"ob", 6},        {"assertion", 6}, {"of", 7},
            {"ob", 6},        {"assertion", 6}, {"ob", 5},
            {"of", 6},        {"of", 6},        {"assertion", 6},
            {"of", 7},        {"of", 8},        {"sf", 9},
            {"sb", 8},        {"sf", 9},        {"sf", 9},
            {"assertion", 9}, {"sb", 8},        {"sf", 9},
            {"sf", 9},        {"assertion", 9}, {"sf", 10},
            {"sb", 9},        {"assertion", 9}, {"sf", 10}}}};
  moveAndCheck(expected.at(GetParam()));
}

TEST_P(SimulationTest, StepOut) {
  const std::map<const std::string,
                 std::vector<std::pair<const std::string, size_t>>>
      expected = {{"complex-jumps",
                   {{"sf", 1},  {"sf", 4}, {"sf", 9},  {"sf", 12}, {"sf", 5},
                    {"ub", 12}, {"sf", 5}, {"sf", 6},  {"ub", 12}, {"sf", 5},
                    {"of", 6},  {"of", 7}, {"ub", 12}, {"sf", 5},  {"sf", 6},
                    {"sf", 10}, {"ub", 6}, {"sf", 10}, {"sf", 2},  {"sf", 3},
                    {"ub", 10}, {"uf", 7}, {"ob", 6},  {"sf", 10}, {"sf", 2},
                    {"uf", 11}, {"uf", 7}, {"uf", 13}}},
                  {"failing-assertions",
                   {{"sf", 1},
                    {"uf", 6},
                    {"uf", 9},
                    {"uf", 11},
                    {"ub", 0},
                    {"uf", 6},
                    {"ub", 0},
                    {"uf", 6},
                    {"uf", 9}}}};
  moveAndCheck(expected.at(GetParam()));
}

TEST_P(SimulationTest, RunSimulation) {
  const std::map<const std::string,
                 std::vector<std::pair<const std::string, size_t>>>
      expected = {{"complex-jumps",
                   {{"sf", 1},
                    {"rf", 15},
                    {"rb", 0},
                    {"sf", 1},
                    {"sf", 4},
                    {"sf", 9},
                    {"sf", 12},
                    {"sf", 5},
                    {"sf", 6},
                    {"rf", 15},
                    {"rb", 0},
                    {"rf", 15}}},
                  {"failing-assertions",
                   {{"sf", 1},
                    {"rf", 6},
                    {"rf", 9},
                    {"rf", 11},
                    {"rb", 0},
                    {"rf", 6},
                    {"rb", 0},
                    {"rf", 6},
                    {"rf", 9},
                    {"rf", 11}}}};
  moveAndCheck(expected.at(GetParam()));
  ASSERT_TRUE(state->isFinished(state));
}

INSTANTIATE_TEST_SUITE_P(StringParams, SimulationTest,
                         ::testing::Values("complex-jumps",
                                           "failing-assertions"));
