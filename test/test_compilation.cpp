/**
 * @file test_compilation.cpp
 * @brief Tests the correctness of the assertion compilation process for
 * assertion programs.
 */

#include "common.h"
#include "common_fixtures.hpp"

#include <cstddef>
#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <vector>

/**
 * @brief Represents a preamble entry in the compiled code.
 */
struct PreambleEntry {
  /**
   * @brief The name of the variable the preamble entry is for.
   */
  std::string name;
  /**
   * @brief The expected ratio of |1> results for the variable's measurement.
   */
  double oneRate;
  /**
   * @brief The required fidelity for the variable's measurement outcomes.
   */
  double fidelity;
};

/**
 * @brief Fixture for testing the correctness of the compilation process.
 *
 * This fixture sets up a DDSimulationState and provides the method
 * `loadCode` to load custom code into the state. The code is then
 * compiled and the resulting program is checked for correctness.
 */
class CompilationTest : public CustomCodeFixture {

protected:
  std::string addBoilerplate(size_t /*numQubits*/, size_t /*numClassics*/,
                             const char* code,
                             const char* /*preamble*/) override {
    return code;
  }

  static std::string addPreamble(const std::string& code,
                                 const std::vector<PreambleEntry>& preamble) {
    std::stringstream ss;
    for (const auto& entry : preamble) {
      ss << "// ASSERT: " << entry.name << "," << entry.oneRate << ","
         << entry.fidelity << "\n";
    }
    ss << code;
    return ss.str();
  }

public:
  /**
   * @brief Check the compilation of the loaded code with the given settings.
   * @param settings The settings to use for the compilation.
   * @param expected The expected compiled code.
   */
  void checkCompilation(const CompilationSettings& settings,
                        const std::string& expected,
                        const std::vector<PreambleEntry>& expectedPreamble) {
    // Compile the code
    const size_t size = state->compile(state, nullptr, settings);
    ASSERT_NE(size, 0) << "Compilation failed";
    std::vector<char> buffer(size);
    const size_t newSize = state->compile(state, buffer.data(), settings);
    ASSERT_EQ(size, newSize) << "Compilation resulted in unexpected size";
    const auto expectedCode = addPreamble(expected, expectedPreamble);
    ASSERT_EQ(std::string(buffer.data()), expectedCode)
        << "Compilation resulted in unexpected code";
  }
};

TEST_F(CompilationTest, StatisticalSingleEqualityCertainNoOpt) {
  loadCode(3, 0,
           "qreg q[1];"
           "x q[0];"
           "assert-eq q[0] { 0, 1 }");

  const CompilationSettings settings = {
      /*mode=*/CompilationMode::STATISTICAL_SLICES,
      /*opt=*/0,
      /*sliceIndex=*/0,
  };

  const std::vector<PreambleEntry> preamble = {
      {"test_q0", 0.0, 1.0},
  };

  checkCompilation(settings,
                   "qreg q[1];"
                   "h q[0];"
                   "assert-eq q[0] { 0, 1 }",
                   preamble);
}
