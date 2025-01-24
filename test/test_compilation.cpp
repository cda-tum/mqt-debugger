/**
 * @file test_compilation.cpp
 * @brief Tests the correctness of the assertion compilation process for
 * assertion programs.
 */

#include "common.h"
#include "common_fixtures.hpp"

#include <cstddef>
#include <gtest/gtest.h>
#include <iostream>
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
   * @brief The expected ratio of |1> results for the variable's measurement or
   * another variable it is related to.
   */
  std::string oneRate;
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

  /**
   * @brief Add the testing preamble to the given code.
   * @param code The code to add the preamble to.
   * @param preamble The preamble to add to the code.
   * @return The code with the preamble added.
   */
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

  void loadCode(const char* code) {
    CustomCodeFixture::loadCode(0, 0, code, false, "");
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
    std::cout << expectedCode;
    ASSERT_EQ(std::string(buffer.data()), expectedCode)
        << "Compilation resulted in unexpected code";
  }

  /**
   * @brief Check whether the compilation of the loaded code fails with the
   * given settings.
   * @param settings The settings to use for the compilation.
   */
  void checkNoCompilation(const CompilationSettings& settings) {
    ASSERT_EQ(state->compile(state, nullptr, settings), 0)
        << "Compilation should have failed";
  }
};

/**
 * @brief Tests the compilation of a simple equality assertion using
 * statistical slices, no optimization, with a certain outcome.
 */
TEST_F(CompilationTest, StatisticalSingleEqualityCertainNoOpt) {
  loadCode("qreg q[1];\n"
           "x q[0];\n"
           "assert-eq q[0] { 0, 1 }\n");

  const CompilationSettings settings = {
      /*mode=*/CompilationMode::STATISTICAL_SLICES,
      /*opt=*/0,
      /*sliceIndex=*/0,
  };

  const std::vector<PreambleEntry> preamble = {
      {"test_q0", "1.0", 1.0},
  };

  checkCompilation(settings,
                   "creg test_q0[1];\n"
                   "qreg q[1];\n"
                   "h q[0];\n"
                   "measure q[0] -> test_q0[0];\n",
                   preamble);

  const CompilationSettings settings2 = {
      /*mode=*/CompilationMode::STATISTICAL_SLICES,
      /*opt=*/0,
      /*sliceIndex=*/1,
  };
  checkNoCompilation(settings2);
}

/**
 * @brief Tests the compilation of a simple equality assertion using
 * statistical slices, no optimization, with an uncertain outcome.
 */
TEST_F(CompilationTest, StatisticalSingleEqualityUncertainNoOpt) {
  loadCode("qreg q[1];\n"
           "h q[0];\n"
           "assert-eq 0.9, q[0] { 0.707, 0.707 }\n");

  const CompilationSettings settings = {
      /*mode=*/CompilationMode::STATISTICAL_SLICES,
      /*opt=*/0,
      /*sliceIndex=*/0,
  };

  const std::vector<PreambleEntry> preamble = {
      {"test_q0", "0.499849", 0.9},
  };

  checkCompilation(settings,
                   "creg test_q0[1];\n"
                   "qreg q[1];\n"
                   "h q[0];\n"
                   "measure q[0] -> test_q0[0];\n",
                   preamble);
}

/**
 * @brief Tests the compilation of an equality assertion on an entangled
 * bell pair using statistical slices, no optimization, with an uncertain
 * outcome.
 */
TEST_F(CompilationTest, StatisticalTwoQubitEqualityNoOpt) {
  loadCode("qreg q[2];\n"
           "h q[0];\n"
           "cx q[0], q[1];\n"
           "assert-eq 0.9, q[0], q[1] { 0.707, 0, 0, 0.707 }\n");

  const CompilationSettings settings = {
      /*mode=*/CompilationMode::STATISTICAL_SLICES,
      /*opt=*/0,
      /*sliceIndex=*/0,
  };

  const std::vector<PreambleEntry> preamble = {
      {"test_q0", "0.499849", 0.9},
      {"test_q1", "test_q0", 0.9},
  };

  checkCompilation(settings,
                   "creg test_q0[1];\n"
                   "creg test_q1[1];\n"
                   "qreg q[2];\n"
                   "h q[0];\n"
                   "cx q[0], q[1];\n"
                   "measure q[0] -> test_q0[0];\n"
                   "measure q[1] -> test_q1[0];\n",
                   preamble);
}

/**
 * @brief Tests the compilation of a two directly consecutive equality
 * assertions using statistical slices, no optimization, with an uncertain
 * outcome.
 */
TEST_F(CompilationTest, StatisticalConsecutiveMultiSliceEqualityNoOpt) {
  loadCode("qreg q[1];\n"
           "h q[0];\n"
           "assert-eq 0.9, q[0] { 0.707, 0.707 }\n"
           "assert-eq 0.9, q[0] { 0.707, 0.707 }\n");

  const CompilationSettings settings1 = {
      /*mode=*/CompilationMode::STATISTICAL_SLICES,
      /*opt=*/0,
      /*sliceIndex=*/0,
  };
  const std::vector<PreambleEntry> preamble1 = {
      {"test_q0", "0.499849", 0.9},
  };
  checkCompilation(settings1,
                   "creg test_q0[1];\n"
                   "qreg q[1];\n"
                   "h q[0];\n"
                   "measure q[0] -> test_q0[0];\n",
                   preamble1);

  const CompilationSettings settings2 = {
      /*mode=*/CompilationMode::STATISTICAL_SLICES,
      /*opt=*/0,
      /*sliceIndex=*/1,
  };
  const std::vector<PreambleEntry> preamble2 = {
      {"test_q0", "0.499849", 0.9},
  };
  checkCompilation(settings2,
                   "creg test_q0[1];\n"
                   "qreg q[1];\n"
                   "h q[0];\n"
                   "measure q[0] -> test_q0[0];\n",
                   preamble2);

  const CompilationSettings settings3 = {
      /*mode=*/CompilationMode::STATISTICAL_SLICES,
      /*opt=*/0,
      /*sliceIndex=*/2,
  };
  checkNoCompilation(settings3);
}

/**
 * @brief Tests the compilation of a two non-consecutive equality assertions
 * using statistical slices, no optimization, with an uncertain outcome.
 */
TEST_F(CompilationTest, StatisticalNonConsecutiveMultiSliceEqualityNoOpt) {
  loadCode("qreg q[1];\n"
           "x q[0];\n"
           "assert-eq q[0] { 0, 1 }\n"
           "x q[0];\n"
           "assert-eq q[0] { 1, 0 }\n");

  const CompilationSettings settings1 = {
      /*mode=*/CompilationMode::STATISTICAL_SLICES,
      /*opt=*/0,
      /*sliceIndex=*/0,
  };
  const std::vector<PreambleEntry> preamble1 = {
      {"test_q0", "1.0", 1.0},
  };
  checkCompilation(settings1,
                   "creg test_q0[1];\n"
                   "qreg q[1];\n"
                   "x q[0];\n"
                   "measure q[0] -> test_q0[0];\n",
                   preamble1);

  const CompilationSettings settings2 = {
      /*mode=*/CompilationMode::STATISTICAL_SLICES,
      /*opt=*/0,
      /*sliceIndex=*/1,
  };
  const std::vector<PreambleEntry> preamble2 = {
      {"test_q0", "0.0", 1.0},
  };
  checkCompilation(settings2,
                   "creg test_q0[1];\n"
                   "qreg q[1];\n"
                   "x q[0];\n"
                   "x q[0];\n"
                   "measure q[0] -> test_q0[0];\n",
                   preamble2);

  const CompilationSettings settings3 = {
      /*mode=*/CompilationMode::STATISTICAL_SLICES,
      /*opt=*/0,
      /*sliceIndex=*/2,
  };
  checkNoCompilation(settings3);
}

/**
 * @brief Tests the compilation of a two directly consecutive equality
 * assertions using statistical slices, with optimization, with an uncertain
 * outcome.
 *
 * The second assertion should not be included in the compiled code as it is
 * removed.
 */
TEST_F(CompilationTest, StatisticalConsecutiveMultiSliceEqualityWithOpt) {
  loadCode("qreg q[1];\n"
           "x q[0];\n"
           "assert-eq q[0] { 0, 1 }\n"
           "assert-eq q[0] { 0, 1 }\n");

  const CompilationSettings settings1 = {
      /*mode=*/CompilationMode::STATISTICAL_SLICES,
      /*opt=*/1,
      /*sliceIndex=*/0,
  };
  const std::vector<PreambleEntry> preamble1 = {
      {"test_q0", "1.0", 1.0},
  };
  checkCompilation(settings1,
                   "creg test_q0[1];\n"
                   "qreg q[1];\n"
                   "x q[0];\n"
                   "measure q[0] -> test_q0[0];\n",
                   preamble1);

  const CompilationSettings settings2 = {
      /*mode=*/CompilationMode::STATISTICAL_SLICES,
      /*opt=*/1,
      /*sliceIndex=*/1,
  };
  checkNoCompilation(settings2);
}
