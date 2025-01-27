/**
 * @file test_compilation.cpp
 * @brief Tests the correctness of the assertion compilation process for
 * assertion programs.
 */

#include "common.h"
#include "common/parsing/Utils.hpp"
#include "common_fixtures.hpp"
#include "utils_test.hpp"

#include <algorithm>
#include <cstddef>
#include <gtest/gtest.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

/**
 * @brief Represents a preamble entry in the compiled code.
 */
struct PreambleEntry {
  /**
   * @brief The name of the variable the preamble entry is for.
   */
  std::vector<std::string> names{};
  /**
   * @brief The expected ratio of |1> results for the variable's measurement or
   * another variable it is related to.
   */
  std::vector<Complex> distribution{};
  /**
   * @brief The required fidelity for the variable's measurement outcomes.
   */
  double fidelity{};
};

/**
 * @brief Create a preamble entry consisting of only real values.
 * @param names The name of the variable the preamble entry is for.
 * @param distribution
 * @param fidelity
 * @return
 */
PreambleEntry realPreamble(std::vector<std::string> names,
                           std::vector<double> distribution, double fidelity) {
  std::vector<Complex> complexDistribution(distribution.size());
  std::transform(distribution.begin(), distribution.end(),
                 complexDistribution.begin(),
                 [](double value) { return Complex{value, 0.0}; });
  return {std::move(names), complexDistribution, fidelity};
}

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
      ss << "// ASSERT: (";
      for (size_t i = 0; i < entry.names.size(); i++) {
        ss << entry.names[i];
        if (i < entry.names.size() - 1) {
          ss << ",";
        }
      }
      ss << ") " << entry.fidelity << " {";
      for (size_t i = 0; i < entry.distribution.size(); i++) {
        ss << complexToStringTest(entry.distribution[i]);
        if (i < entry.distribution.size() - 1) {
          ss << ",";
        }
      }
      ss << "}\n";
    }
    ss << code;
    return ss.str();
  }

  /**
   * @brief Load the given code into the state.
   *
   * Convenience overload for base method that automatically sets unneeded
   * parameters to 0.
   *
   * @param code The code to load.
   */
  void loadCode(const char* code) {
    CustomCodeFixture::loadCode(0, 0, code, false, "");
  }

  /**
   * @brief Display the expected and actual compiled codes side by side.
   * @param expected The expected compiled code.
   * @param actual The actual compiled code.
   */
  static void prettyPrintComparison(const std::string& expected,
                                    const std::string& actual) {
    auto expectedLines = splitString(expected, '\n');
    auto actualLines = splitString(actual, '\n');
    expectedLines.insert(expectedLines.begin(), "");
    actualLines.insert(actualLines.begin(), "");
    expectedLines.insert(expectedLines.begin(), "EXPECTED:");
    actualLines.insert(actualLines.begin(), "ACTUAL:");
    const auto maxLenExpected =
        std::max_element(expectedLines.begin(), expectedLines.end(),
                         [](const std::string& a, const std::string& b) {
                           return a.size() < b.size();
                         })
            ->size();
    const auto maxLenActual =
        std::max_element(actualLines.begin(), actualLines.end(),
                         [](const std::string& a, const std::string& b) {
                           return a.size() < b.size();
                         })
            ->size();
    const auto lineCount = std::max(expectedLines.size(), actualLines.size());
    std::cout << "\n";
    for (size_t i = 0; i < lineCount; i++) {
      if (i == 1) {
        std::cout << std::setfill('-');
      } else {
        std::cout << std::setfill(' ');
      }
      const auto expectedLine =
          i < expectedLines.size() ? expectedLines[i] : "";
      const auto actualLine = i < actualLines.size() ? actualLines[i] : "";
      std::cout << std::left << std::setw(static_cast<int>(maxLenExpected + 3))
                << expectedLine << " | "
                << std::setw(static_cast<int>(maxLenActual + 3)) << actualLine
                << "\n";
    }
    std::cout << "\n";
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
    const auto receivedCode = std::string(buffer.data());
    prettyPrintComparison(expectedCode, receivedCode);
    ASSERT_EQ(expectedCode, receivedCode)
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
      realPreamble({"test_q0"}, {0.0, 1.0}, 1.0)};

  checkCompilation(settings,
                   "creg test_q0[1];\n"
                   "qreg q[1];\n"
                   "x q[0];\n"
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
      realPreamble({"test_q0"}, {0.499849, 0.499849}, 0.9)};

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
      realPreamble({"test_q0", "test_q1"}, {0.499849, 0, 0, 0.499849}, 0.9)};

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
  const std::vector<PreambleEntry> preamble = {
      realPreamble({"test_q0"}, {0.499849, 0.499849}, 0.9)};
  checkCompilation(settings1,
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
  checkCompilation(settings2,
                   "creg test_q0[1];\n"
                   "qreg q[1];\n"
                   "h q[0];\n"
                   "measure q[0] -> test_q0[0];\n",
                   preamble);

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
      realPreamble({"test_q0"}, {0, 1}, 1.0)};
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
      realPreamble({"test_q0"}, {1, 0}, 1.0)};
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
 * @brief Tests the compilation of two directly consecutive equality
 * assertions using statistical slices, with optimization when assertions are
 * equal.
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
  const std::vector<PreambleEntry> preamble = {
      realPreamble({"test_q0"}, {0, 1}, 1.0)};
  checkCompilation(settings1,
                   "creg test_q0[1];\n"
                   "qreg q[1];\n"
                   "x q[0];\n"
                   "measure q[0] -> test_q0[0];\n",
                   preamble);

  const CompilationSettings settings2 = {
      /*mode=*/CompilationMode::STATISTICAL_SLICES,
      /*opt=*/1,
      /*sliceIndex=*/1,
  };
  checkNoCompilation(settings2);
}

/**
 * @brief Tests the compilation of two directly consecutive equality
 * assertions using statistical slices, with optimization when the second
 * assertion is a subset of the first.
 *
 * The second assertion should not be included in the compiled code as it is
 * removed.
 */
TEST_F(CompilationTest,
       StatisticalConsecutiveMultiSliceEqualitySmallerScopeOpt) {
  loadCode("qreg q[2];\n"
           "x q[0];\n"
           "assert-eq q[0], q[1] { 0, 1, 0, 0 }\n"
           "assert-eq q[0] { 0, 1 }\n");

  const CompilationSettings settings = {
      /*mode=*/CompilationMode::STATISTICAL_SLICES,
      /*opt=*/1,
      /*sliceIndex=*/1,
  };
  checkNoCompilation(settings);
}

/**
 * @brief Tests the compilation of two directly consecutive equality
 * assertions using statistical slices, with optimization when the assertions
 * are not exactly equal but similarity thresholds are still met.
 *
 * The second assertion should not be included in the compiled code as it is
 * removed.
 */
TEST_F(CompilationTest, StatisticalConsecutiveMultiSliceEqualityUnpreciseOpt) {
  loadCode("qreg q[1];\n"
           "x q[0];\n"
           "assert-eq 0.9, q[0] { 0, 0.9 }\n"
           "assert-eq 0.8, q[0] { 0, 1 }\n");

  const CompilationSettings settings = {
      /*mode=*/CompilationMode::STATISTICAL_SLICES,
      /*opt=*/1,
      /*sliceIndex=*/1,
  };
  checkNoCompilation(settings);
}

/**
 * @brief Tests the compilation of two directly consecutive equality
 * assertions using statistical slices, with optimization when similarity
 * thresholds do not allow for the second assertion to be removed.
 */
TEST_F(CompilationTest,
       StatisticalConsecutiveMultiSliceEqualityWithOptDifferentSimilarity) {
  loadCode("qreg q[1];\n"
           "x q[0];\n"
           "assert-eq 0.9, q[0] { 0, 1 }\n"
           "assert-eq 0.99, q[0] { 0, 1 }\n");

  const CompilationSettings settings = {
      /*mode=*/CompilationMode::STATISTICAL_SLICES,
      /*opt=*/1,
      /*sliceIndex=*/1,
  };
  const std::vector<PreambleEntry> preamble = {
      realPreamble({"test_q0"}, {0, 1}, 0.99)};
  checkCompilation(settings,
                   "creg test_q0[1];\n"
                   "qreg q[1];\n"
                   "x q[0];\n"
                   "measure q[0] -> test_q0[0];\n",
                   preamble);
}

/**
 * @brief Tests the compilation of two directly consecutive equality
 * assertions using statistical slices, with optimization when different target
 * qubits do not allow for the second assertion to be removed.
 */
TEST_F(CompilationTest,
       StatisticalConsecutiveMultiSliceEqualityWithOptDifferentTargets) {
  loadCode("qreg q[2];\n"
           "x q[0];\n"
           "assert-eq q[0] { 0, 1 }\n"
           "assert-eq q[0], q[1] { 0, 0, 1, 0 }\n");

  const CompilationSettings settings = {
      /*mode=*/CompilationMode::STATISTICAL_SLICES,
      /*opt=*/1,
      /*sliceIndex=*/1,
  };
  const std::vector<PreambleEntry> preamble = {
      realPreamble({"test_q0", "test_q1"}, {0, 0, 1, 0}, 1.0)};
  checkCompilation(settings,
                   "creg test_q0[1];\n"
                   "creg test_q1[1];\n"
                   "qreg q[2];\n"
                   "x q[0];\n"
                   "measure q[0] -> test_q0[0];\n"
                   "measure q[1] -> test_q1[0];\n",
                   preamble);
}
