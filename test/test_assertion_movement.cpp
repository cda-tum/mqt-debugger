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
  void checkMovements(std::set<std::pair<size_t, size_t>> expected) {
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
  assert-ent q[0], q[1];
  )");

  checkMovements({{4, 3}});
}
