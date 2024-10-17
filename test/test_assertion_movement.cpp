/**
 * @file test_assertion_movement.cpp
 * @brief Tests the correctness of the assertion movement diagnosis methods.
 */

#include "backend/dd/DDSimDebug.hpp"
#include "backend/debug.h"
#include "backend/diagnostics.h"
#include "common.h"
#include "common_fixtures.cpp"
#include "utils_test.hpp"

#include <array>
#include <cstddef>
#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

/**
 * @brief Fixture for testing the correctness of assertion movement on custom
 * code.
 *
 * This fixture sets up a DDSimulationState and provides the method
 * `loadCode` to load custom code into the state.
 */
class AssertionMovementTest : public CustomCodeFixture {
public:
  void checkMovements(const std::set<std::pair<size_t, size_t>>& expected) {
    std::vector<size_t> oldPositions(expected.size() + 1);
    std::vector<size_t> newPositions(expected.size() + 1);
    ASSERT_EQ(diagnostics->suggestAssertionMovements(
                  diagnostics, oldPositions.data(), newPositions.data(),
                  oldPositions.size()),
              expected.size());

    for (auto [oldPos, newPos] : expected) {
      oldPos += 2;
      newPos += 2;
      bool found = false;
      for (size_t i = 0; i < expected.size(); i++) {
        if (oldPositions[i] == oldPos) {
          ASSERT_EQ(newPositions[i], newPos)
              << "Expected " << (newPos - 2) << " but got "
              << (newPositions[i] - 2) << " for assertion at " << (oldPos - 2);
          found = true;
          break;
        }
      }
      ASSERT_TRUE(found) << "No suggestion found for assertion at "
                         << (oldPos - 2);
    }
  }
};

TEST_F(AssertionMovementTest, MoveOverIndependentInstructions) {
  loadCode(3, 3, R"(
  h q[0];
  cx q[0], q[1];
  cx q[0], q[2];
  x q[2];
  assert-eq 0.9, q[0], q[1] { 1, 0, 0, 0 }
  )");

  const std::set<std::pair<size_t, size_t>> expected = {{4, 3}};
  checkMovements(expected);
}

TEST_F(AssertionMovementTest, MoveEntOverSingleQubit) {
  loadCode(3, 3, R"(
  h q[0];
  cx q[0], q[1];
  h q[0];
  x q[0];
  h q[1];
  x q[1];
  assert-ent q[0], q[1];
  )");

  const std::set<std::pair<size_t, size_t>> expected = {{6, 2}};
  checkMovements(expected);
}

TEST_F(AssertionMovementTest, MoveSupOverSpecificGates) {
  loadCode(3, 3, R"(
  h q[0];
  cx q[0], q[1];
  h q[0];
  y q[0];
  s q[0];
  h q[1];
  x q[1];
  assert-sup q[0];
  )");

  const std::set<std::pair<size_t, size_t>> expected = {{7, 3}};
  checkMovements(expected);
}

TEST_F(AssertionMovementTest, MoveOverBarrier) {
  loadCode(3, 3, R"(
  h q[0];
  barrier q[0];
  assert-sup q[0];
  )");

  const std::set<std::pair<size_t, size_t>> expected = {{2, 1}};
  checkMovements(expected);
}

TEST_F(AssertionMovementTest, DontMoveMultipleAssertions) {
  loadCode(3, 3, R"(
  cx q[0], q[1];
  assert-sup q[0];
  assert-sup q[1];
  )");

  const std::set<std::pair<size_t, size_t>> expected;
  checkMovements(expected);
}

TEST_F(AssertionMovementTest, MoveThroughFunctionDefinition) {
  loadCode(3, 3, R"(
  h q[0];
  gate test q { h q; }
  assert-sup q[0];
  )");

  const std::set<std::pair<size_t, size_t>> expected({{4, 1}});
  checkMovements(expected);
}

TEST_F(AssertionMovementTest, MoveThroughOtherVariableDeclarations) {
  loadCode(3, 3, R"(
  h q[0];
  qreg x[2];
  creg y[2];
  assert-sup q[0];
  )");

  const std::set<std::pair<size_t, size_t>> expected({{3, 1}});
  checkMovements(expected);
}

TEST_F(AssertionMovementTest, DontMoveThroughOwnVariableDeclarations) {
  loadCode(3, 3, R"(
  h q[0];
  qreg x[2];
  x x[0];
  assert-sup x[0];
  )");

  const std::set<std::pair<size_t, size_t>> expected({{3, 2}});
  checkMovements(expected);
}

TEST_F(AssertionMovementTest, DontMoveThroughMeasurements) {
  loadCode(3, 3, R"(
  h q[0];
  measure q[1] -> c[1];
  assert-ent q[0];
  )");

  const std::set<std::pair<size_t, size_t>> expected;
  checkMovements(expected);
}

TEST_F(AssertionMovementTest, DontMoveThroughResets) {
  loadCode(3, 3, R"(
  h q[0];
  reset q[1];
  assert-ent q[0];
  )");

  const std::set<std::pair<size_t, size_t>> expected;
  checkMovements(expected);
}

TEST_F(AssertionMovementTest, DontMoveThroughFunctionCalls) {
  loadCode(3, 3, R"(
  h q[0];
  gate test q { h q; }
  test q;
  assert-sup q[0];
  )");

  const std::set<std::pair<size_t, size_t>> expected;
  checkMovements(expected);
}

TEST_F(AssertionMovementTest, RelatedClassicControlledGate) {
  loadCode(3, 3, R"(
  x q[0];
  measure q[0] -> c[0];
  if(c == 0) cx q[1], q[2];
  x q[1];
  assert-ent q[1], q[2];
  )");

  const std::set<std::pair<size_t, size_t>> expected({{4, 3}});
  checkMovements(expected);
}

TEST_F(AssertionMovementTest, UnrelatedClassicControlledGate) {
  loadCode(3, 3, R"(
  x q[0];
  measure q[0] -> c[0];
  if(c == 0) x q[2];
  x q[1];
  assert-ent q[1], q[2];
  )");

  const std::set<std::pair<size_t, size_t>> expected({{4, 2}});
  checkMovements(expected);
}
