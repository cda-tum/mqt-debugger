/**
 * @file test_compilation.cpp
 * @brief Tests the correctness of the assertion compilation process for
 * assertion programs.
 */

#include "common_fixtures.hpp"

/**
 * @brief Fixture for testing the correctness of the compilation process.
 *
 * This fixture sets up a DDSimulationState and provides the method
 * `loadCode` to load custom code into the state. The code is then
 * compiled and the resulting program is checked for correctness.
 */
class CompilationTest : public CustomCodeFixture {

public:
  void checkCompilation(std::string expected) {
    ASSERT_EQ(true, true); // TODO
  }
};
