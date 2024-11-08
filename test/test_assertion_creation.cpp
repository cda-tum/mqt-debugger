/**
 * @file test_assertion_creation.cpp
 * @brief Tests the correctness of the assertion creation diagnosis methods.
 */

#include "backend/debug.h"
#include "backend/diagnostics.h"
#include "common.h"
#include "common_fixtures.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <gtest/gtest.h>
#include <set>
#include <string>
#include <utility>
#include <vector>

/**
 * @brief Fixture for testing the correctness of assertion creation on custom
 * code.
 *
 * This fixture sets up a DDSimulationState and provides the method
 * `loadCode` to load custom code into the state.
 */
class AssertionCreationTest : public CustomCodeFixture {
public:
  void
  checkNewAssertions(const std::set<std::pair<size_t, std::string>>& expected,
                     size_t expectedErrors) {
    size_t errors = 0;
    ASSERT_EQ(state->runAll(state, &errors), OK);
    ASSERT_EQ(errors, expectedErrors);
    std::vector<size_t> newPositions(expected.size() + 1);
    std::vector<std::array<char, 256>> newAssertions(expected.size() + 1);
    std::vector<char*> newAssertionsPointers(expected.size() + 1);
    std::transform(newAssertions.begin(), newAssertions.end(),
                   newAssertionsPointers.begin(),
                   [](std::array<char, 256>& arr) { return arr.data(); });
    ASSERT_EQ(diagnostics->suggestNewAssertions(
                  diagnostics, newPositions.data(),
                  newAssertionsPointers.data(), newPositions.size()),
              expected.size());

    for (auto [pos, string] : expected) {
      pos += 2;
      bool found = false;
      for (size_t i = 0; i < expected.size(); i++) {
        if (newPositions[i] == pos && newAssertions[i].data() == string) {
          found = true;
          break;
        }
      }
      ASSERT_TRUE(found) << "No new assertion found for " << (pos - 2);
    }
  }
};

TEST_F(AssertionCreationTest, CreateEntanglementAssertionFromBigAssertion) {
  loadCode(3, 3, R"(
  h q[0];
  cx q[0], q[1];
  cx q[2], q[1];
  assert-ent q[0], q[1], q[2];
  )");

  const std::set<std::pair<size_t, std::string>> expected = {
      {3, "assert-ent q[1], q[2];\n"},
      {3, "assert-ent q[0], q[2];\n"},
      {3, "assert-ent q[0], q[1];\n"}};
  checkNewAssertions(expected, 1);
}

TEST_F(AssertionCreationTest, CreateEntanglementAssertionFromTreeSimple) {
  loadCode(3, 3, R"(
  h q[0];
  cx q[0], q[1];
  cx q[2], q[1];
  assert-ent q[0], q[2];
  )");

  const std::set<std::pair<size_t, std::string>> expected = {
      {3, "assert-ent q[1], q[2];\n"}};
  checkNewAssertions(expected, 1);
}

TEST_F(AssertionCreationTest, SplitEqualityAssertion) {
  loadCode(3, 1, R"(
  assert-eq q[0], q[1] { 1, 0, 0, 0 }
  )");

  const std::set<std::pair<size_t, std::string>> expected = {
      {0, "assert-eq q[0] { 1, 0 }\n"},
      {0, "assert-eq q[1] { 1, 0 }\n"}
  };
  checkNewAssertions(expected, 1);
}

TEST_F(AssertionCreationTest, SplitEqualityAssertionMultipleAmplitudes) {
  loadCode(3, 1, R"(
  assert-eq q[0], q[1] { 0.5, 0.5, 0.5, 0.5 }
  )");

  const std::set<std::pair<size_t, std::string>> expected = {
      {0, "assert-eq 0.99999, q[0] { 0.70711, 0.70711 }\n"},
      {0, "assert-eq 0.99999, q[1] { 0.70711, 0.70711 }\n"}
  };
  checkNewAssertions(expected, 1);
}

TEST_F(AssertionCreationTest, DontSplitEntangledEqualityAssertion) {
  loadCode(3, 1, R"(
  assert-eq 0.9, q[0], q[1] { 0.707, 0, 0, 0.707 }
  )");

  const std::set<std::pair<size_t, std::string>> expected = {};
  checkNewAssertions(expected, 1);
}