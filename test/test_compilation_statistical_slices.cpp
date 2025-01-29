/**
 * @file test_compilation_statistical_slices.cpp
 * @brief Tests the correctness of the assertion compilation process for
 * assertion programs using statistical slices.
 */

#include "common.h"
#include "common_fixtures.hpp"
#include "utils_test.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>
#include <iterator>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

/**
 * @brief A preamble entry for statistical equality assertions.
 */
class StatEqPreambleEntry : public PreambleEntry {
  /**
   * @brief The name of the variable the preamble entry is for.
   */
  std::vector<std::string> names;
  /**
   * @brief The expected ratio of |1> results for the variable's measurement or
   * another variable it is related to.
   */
  std::vector<Complex> distribution;
  /**
   * @brief The required fidelity for the variable's measurement outcomes.
   */
  double fidelity;

public:
  /**
   * @brief Constructs a new StatEqPreambleEntry with the given names,
   * distribution, and fidelity.
   * @param n The names of the variables the preamble entry is for.
   * @param dist The expected distribution of the preamble entry as complex
   * numbers.
   * @param fid The required fidelity for the preamble entry.
   */
  StatEqPreambleEntry(std::vector<std::string> n, std::vector<Complex> dist,
                      double fid)
      : names(std::move(n)), distribution(std::move(dist)), fidelity(fid) {}

  /**
   * @brief Constructs a new StatEqPreambleEntry with the given names,
   * distribution, and fidelity.
   * @param n The names of the variables the preamble entry is for.
   * @param dist The expected distribution of the preamble entry as real
   * numbers.
   * @param fid The required fidelity for the preamble entry.
   */
  StatEqPreambleEntry(std::vector<std::string> n, std::vector<double> dist,
                      double fid)
      : names(std::move(n)), distribution(dist.size()), fidelity(fid) {
    std::transform(dist.begin(), dist.end(), this->distribution.begin(),
                   [](double value) { return Complex{value, 0.0}; });
  }

  [[nodiscard]] std::string toString() const override {
    std::stringstream ss;
    ss << "// ASSERT: (";
    for (size_t i = 0; i < names.size(); i++) {
      ss << names[i];
      if (i < names.size() - 1) {
        ss << ",";
      }
    }
    ss << ") {";
    for (size_t i = 0; i < distribution.size(); i++) {
      ss << complexToStringTest(distribution[i]);
      if (i < distribution.size() - 1) {
        ss << ",";
      }
    }
    ss << "} " << fidelity << "\n";

    return ss.str();
  }
};

/**
 * @brief A preamble entry for statistical superposition assertions.
 */
class StatSupPreambleEntry : public PreambleEntry {
  /**
   * @brief The name of the variable the preamble entry is for.
   */
  std::vector<std::string> names;

public:
  /**
   * @brief Constructs a new StatSupPreambleEntry with the given names
   * @param n The names of the variables the preamble entry is for.
   */
  StatSupPreambleEntry(std::vector<std::string> n) : names(std::move(n)) {}

  [[nodiscard]] std::string toString() const override {
    std::stringstream ss;
    ss << "// ASSERT: (";
    for (size_t i = 0; i < names.size(); i++) {
      ss << names[i];
      if (i < names.size() - 1) {
        ss << ",";
      }
    }
    ss << ") {superposition}\n";
    return ss.str();
  }
};

/**
 * @brief A test fixture for testing the compilation of assertion programs using
 * statistical slices.
 */
class StatisticalSlicesCompilationTest : public CompilationTest {
public:
  /**
   * @brief Creates a new CompilationSettings object for statistical slices.
   * @param opt The optimization level to use.
   * @param slice The slice index to use.
   * @return The created CompilationSettings object.
   */
  static CompilationSettings makeSettings(uint8_t opt, size_t slice) {
    return {
        /*mode=*/CompilationMode::STATISTICAL_SLICES,
        /*opt=*/opt,
        /*sliceIndex=*/slice,
    };
  }

  /**
   * @brief Check the compilation of the loaded code with the given settings.
   * @param settings The settings to use for the compilation.
   * @param expected The expected compiled code.
   * @param expectedPreamble The expected preamble entries for equality
   * assertions.
   */
  void
  checkCompilation(const CompilationSettings& settings,
                   const std::string& expected,
                   const std::vector<StatEqPreambleEntry>& expectedPreamble) {
    auto pointerVector =
        std::vector<const PreambleEntry*>(expectedPreamble.size());
    std::transform(expectedPreamble.begin(), expectedPreamble.end(),
                   pointerVector.begin(),
                   [](const StatEqPreambleEntry& entry) { return &entry; });
    CompilationTest::checkCompilation(settings, expected, pointerVector);
  }

  /**
   * @brief Check the compilation of the loaded code with the given settings.
   * @param settings The settings to use for the compilation.
   * @param expected The expected compiled code.
   * @param expectedPreamble The expected preamble entries for entanglement
   * assertions.
   */
  void
  checkCompilation(const CompilationSettings& settings,
                   const std::string& expected,
                   const std::vector<StatSupPreambleEntry>& expectedPreamble) {
    auto pointerVector =
        std::vector<const PreambleEntry*>(expectedPreamble.size());
    std::transform(expectedPreamble.begin(), expectedPreamble.end(),
                   pointerVector.begin(),
                   [](const StatSupPreambleEntry& entry) { return &entry; });
    CompilationTest::checkCompilation(settings, expected, pointerVector);
  }
};

/**
 * @brief Tests the compilation of a simple equality assertion using
 * statistical slices, no optimization, with a certain outcome.
 */
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalSingleEqualityCertainNoOpt) {
  loadCode("qreg q[1];\n"
           "x q[0];\n"
           "assert-eq q[0] { 0, 1 }\n");

  const std::vector<StatEqPreambleEntry> preamble = {
      StatEqPreambleEntry({"test_q0"}, {0.0, 1.0}, 1.0)};

  checkCompilation(makeSettings(/*opt=*/0, /*slice=*/0),
                   "creg test_q0[1];\n"
                   "qreg q[1];\n"
                   "x q[0];\n"
                   "measure q[0] -> test_q0[0];\n",
                   preamble);

  checkNoCompilation(makeSettings(/*opt=*/0, /*slice=*/1));
}

/**
 * @brief Tests the compilation of a simple equality assertion using
 * statistical slices, no optimization, with an uncertain outcome.
 */
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalSingleEqualityUncertainNoOpt) {
  loadCode("qreg q[1];\n"
           "h q[0];\n"
           "assert-eq 0.9, q[0] { 0.707, 0.707 }\n");

  const std::vector<StatEqPreambleEntry> preamble = {
      StatEqPreambleEntry({"test_q0"}, {0.499849, 0.499849}, 0.9)};

  checkCompilation(makeSettings(0, 0),
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
TEST_F(StatisticalSlicesCompilationTest, StatisticalTwoQubitEqualityNoOpt) {
  loadCode("qreg q[2];\n"
           "h q[0];\n"
           "cx q[0], q[1];\n"
           "assert-eq 0.9, q[0], q[1] { 0.707, 0, 0, 0.707 }\n");

  const std::vector<StatEqPreambleEntry> preamble = {StatEqPreambleEntry(
      {"test_q0", "test_q1"}, {0.499849, 0, 0, 0.499849}, 0.9)};

  checkCompilation(makeSettings(0, 0),
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
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalConsecutiveMultiSliceEqualityNoOpt) {
  loadCode("qreg q[1];\n"
           "h q[0];\n"
           "assert-eq 0.9, q[0] { 0.707, 0.707 }\n"
           "assert-eq 0.9, q[0] { 0.707, 0.707 }\n");

  const std::vector<StatEqPreambleEntry> preamble = {
      StatEqPreambleEntry({"test_q0"}, {0.499849, 0.499849}, 0.9)};
  checkCompilation(makeSettings(0, 0),
                   "creg test_q0[1];\n"
                   "qreg q[1];\n"
                   "h q[0];\n"
                   "measure q[0] -> test_q0[0];\n",
                   preamble);

  checkCompilation(makeSettings(0, 1),
                   "creg test_q0[1];\n"
                   "qreg q[1];\n"
                   "h q[0];\n"
                   "measure q[0] -> test_q0[0];\n",
                   preamble);

  checkNoCompilation(makeSettings(0, 2));
}

/**
 * @brief Tests the compilation of a two non-consecutive equality assertions
 * using statistical slices, no optimization, with an uncertain outcome.
 */
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalNonConsecutiveMultiSliceEqualityNoOpt) {
  loadCode("qreg q[1];\n"
           "x q[0];\n"
           "assert-eq q[0] { 0, 1 }\n"
           "x q[0];\n"
           "assert-eq q[0] { 1, 0 }\n");

  const std::vector<StatEqPreambleEntry> preamble1 = {
      StatEqPreambleEntry({"test_q0"}, {0, 1}, 1.0)};
  checkCompilation(makeSettings(0, 0),
                   "creg test_q0[1];\n"
                   "qreg q[1];\n"
                   "x q[0];\n"
                   "measure q[0] -> test_q0[0];\n",
                   preamble1);

  const std::vector<StatEqPreambleEntry> preamble2 = {
      StatEqPreambleEntry({"test_q0"}, {1, 0}, 1.0)};
  checkCompilation(makeSettings(0, 1),
                   "creg test_q0[1];\n"
                   "qreg q[1];\n"
                   "x q[0];\n"
                   "x q[0];\n"
                   "measure q[0] -> test_q0[0];\n",
                   preamble2);

  checkNoCompilation(makeSettings(0, 2));
}

/**
 * @brief Tests the compilation of two directly consecutive equality
 * assertions using statistical slices, with optimization when assertions are
 * equal.
 *
 * The second assertion should not be included in the compiled code as it is
 * removed.
 */
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalConsecutiveMultiSliceEqualityWithOpt) {
  loadCode("qreg q[1];\n"
           "x q[0];\n"
           "assert-eq q[0] { 0, 1 }\n"
           "assert-eq q[0] { 0, 1 }\n");

  const std::vector<StatEqPreambleEntry> preamble = {
      StatEqPreambleEntry({"test_q0"}, {0, 1}, 1.0)};
  checkCompilation(makeSettings(1, 0),
                   "creg test_q0[1];\n"
                   "qreg q[1];\n"
                   "x q[0];\n"
                   "measure q[0] -> test_q0[0];\n",
                   preamble);

  checkNoCompilation(makeSettings(1, 1));
}

/**
 * @brief Tests the compilation of two directly consecutive equality
 * assertions using statistical slices, with optimization when the second
 * assertion is a subset of the first.
 *
 * The second assertion should not be included in the compiled code as it is
 * removed.
 */
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalConsecutiveMultiSliceEqualitySmallerScopeOpt) {
  loadCode("qreg q[2];\n"
           "x q[0];\n"
           "assert-eq q[0], q[1] { 0, 1, 0, 0 }\n"
           "assert-eq q[0] { 0, 1 }\n");

  checkNoCompilation(makeSettings(1, 1));
}

/**
 * @brief Tests the compilation of two directly consecutive equality
 * assertions using statistical slices, with optimization when the assertions
 * are not exactly equal but similarity thresholds are still met.
 *
 * The second assertion should not be included in the compiled code as it is
 * removed.
 */
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalConsecutiveMultiSliceEqualityUnpreciseOpt) {
  loadCode("qreg q[1];\n"
           "x q[0];\n"
           "assert-eq 0.9, q[0] { 0, 0.9 }\n"
           "assert-eq 0.8, q[0] { 0, 1 }\n");

  checkNoCompilation(makeSettings(1, 1));
}

/**
 * @brief Tests the compilation of two directly consecutive equality
 * assertions using statistical slices, with optimization when similarity
 * thresholds do not allow for the second assertion to be removed.
 */
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalConsecutiveMultiSliceEqualityWithOptDifferentSimilarity) {
  loadCode("qreg q[1];\n"
           "x q[0];\n"
           "assert-eq 0.9, q[0] { 0, 1 }\n"
           "assert-eq 0.99, q[0] { 0, 1 }\n");

  const std::vector<StatEqPreambleEntry> preamble = {
      StatEqPreambleEntry({"test_q0"}, {0, 1}, 0.99)};
  checkCompilation(makeSettings(1, 1),
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
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalConsecutiveMultiSliceEqualityWithOptDifferentTargets) {
  loadCode("qreg q[2];\n"
           "x q[0];\n"
           "assert-eq q[0] { 0, 1 }\n"
           "assert-eq q[0], q[1] { 0, 0, 1, 0 }\n");

  const std::vector<StatEqPreambleEntry> preamble = {
      StatEqPreambleEntry({"test_q0", "test_q1"}, {0, 0, 1, 0}, 1.0)};
  checkCompilation(makeSettings(1, 1),
                   "creg test_q0[1];\n"
                   "creg test_q1[1];\n"
                   "qreg q[2];\n"
                   "x q[0];\n"
                   "measure q[0] -> test_q0[0];\n"
                   "measure q[1] -> test_q1[0];\n",
                   preamble);
}

/**
 * @brief Tests the compilation of a simple superposition assertion using
 * statistical slices and no optimization.
 */
TEST_F(StatisticalSlicesCompilationTest, StatisticalSingleSuperpositionNoOpt) {
  loadCode("qreg q[1];\n"
           "h q[0];\n"
           "assert-sup q[0];\n");

  const std::vector<StatSupPreambleEntry> preamble = {
      StatSupPreambleEntry({"test_q0"})};

  checkCompilation(makeSettings(0, 0),
                   "creg test_q0[1];\n"
                   "qreg q[1];\n"
                   "h q[0];\n"
                   "measure q[0] -> test_q0[0];\n",
                   preamble);

  checkNoCompilation(makeSettings(0, 1));
}

/**
 * @brief Tests the compilation of a 2-qubit superposition assertion using
 * statistical slices and no optimization.
 */
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalMultiQubitSuperpositionNoOpt) {
  loadCode("qreg q[2];\n"
           "h q[0];\n"
           "assert-sup q[0], q[1];\n");

  const std::vector<StatSupPreambleEntry> preamble = {
      StatSupPreambleEntry({"test_q0", "test_q1"})};

  checkCompilation(makeSettings(0, 0),
                   "creg test_q0[1];\n"
                   "creg test_q1[1];\n"
                   "qreg q[2];\n"
                   "h q[0];\n"
                   "measure q[0] -> test_q0[0];\n"
                   "measure q[1] -> test_q1[0];\n",
                   preamble);

  checkNoCompilation(makeSettings(0, 1));
}

/**
 * @brief Tests the compilation of two consecutive superposition assertions
 * using statistical slices and no optimization.
 */
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalMultipleSuperpositionNoOpt) {
  loadCode("qreg q[2];\n"
           "h q[0];\n"
           "h q[1];\n"
           "assert-sup q[0];\n"
           "assert-sup q[1];\n");

  const std::vector<StatSupPreambleEntry> preamble1 = {
      StatSupPreambleEntry({"test_q0"})};

  checkCompilation(makeSettings(0, 0),
                   "creg test_q0[1];\n"
                   "qreg q[2];\n"
                   "h q[0];\n"
                   "h q[1];\n"
                   "measure q[0] -> test_q0[0];\n",
                   preamble1);

  const std::vector<StatSupPreambleEntry> preamble2 = {
      StatSupPreambleEntry({"test_q1"})};
  checkCompilation(makeSettings(0, 1),
                   "creg test_q1[1];\n"
                   "qreg q[2];\n"
                   "h q[0];\n"
                   "h q[1];\n"
                   "measure q[1] -> test_q1[0];\n",
                   preamble2);

  checkNoCompilation(makeSettings(0, 2));
}

/**
 * @brief Tests the compilation of two consecutive superposition assertions when
 * the second assertion can be trivially canceled out by the first one.
 */
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalSuperpositionOptIncludedInSup) {
  loadCode("qreg q[1];\n"
           "h q[0];\n"
           "assert-sup q[0];\n"
           "assert-sup q[0];\n");

  const std::vector<StatSupPreambleEntry> preamble = {
      StatSupPreambleEntry({"test_q0"})};

  checkCompilation(makeSettings(1, 0),
                   "creg test_q0[1];\n"
                   "qreg q[1];\n"
                   "h q[0];\n"
                   "measure q[0] -> test_q0[0];\n",
                   preamble);

  checkNoCompilation(makeSettings(1, 1));
}

/**
 * @brief Tests the compilation of two superposition assertions when the second
 * assertion can be canceled out by the first one after applying commutation
 * rules.
 */
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalSuperpositionOptIncludedInSupWithCommutation) {
  loadCode("qreg q[1];\n"
           "h q[0];\n"
           "assert-sup q[0];\n"
           "x q[0];\n"
           "assert-sup q[0];\n");

  const std::vector<StatSupPreambleEntry> preamble = {
      StatSupPreambleEntry({"test_q0"})};

  checkCompilation(makeSettings(1, 0),
                   "creg test_q0[1];\n"
                   "qreg q[1];\n"
                   "h q[0];\n"
                   "measure q[0] -> test_q0[0];\n",
                   preamble);

  checkNoCompilation(makeSettings(1, 1));
}

/**
 * @brief Tests the compilation of two superposition assertions when the second
 * assertion can be canceled out by the first one as its target qubits
 * are a superset of the first assertion's target qubits.
 */
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalSuperpositionOptIncludedInSupSuperset) {
  loadCode("qreg q[2];\n"
           "h q[0];\n"
           "assert-sup q[0];\n"
           "x q[0];\n"
           "assert-sup q[0], q[1];\n");

  const std::vector<StatSupPreambleEntry> preamble = {
      StatSupPreambleEntry({"test_q0"})};

  checkCompilation(makeSettings(1, 0),
                   "creg test_q0[1];\n"
                   "qreg q[2];\n"
                   "h q[0];\n"
                   "measure q[0] -> test_q0[0];\n",
                   preamble);

  checkNoCompilation(makeSettings(1, 1));
}

/**
 * @brief Tests the compilation of two superposition assertions when the second
 * assertion cannot be canceled out by the first one as its target qubits
 * are not a full superset of the first assertion's target qubits.
 */
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalSuperpositionOptNotFullyIncludedInSupSuperset) {
  loadCode("qreg q[3];\n"
           "h q[0];\n"
           "assert-sup q[0], q[2];\n"
           "assert-sup q[0], q[1];\n");

  const std::vector<StatSupPreambleEntry> preamble1 = {
      StatSupPreambleEntry({"test_q0", "test_q2"})};

  checkCompilation(makeSettings(1, 0),
                   "creg test_q0[1];\n"
                   "creg test_q2[1];\n"
                   "qreg q[3];\n"
                   "h q[0];\n"
                   "measure q[0] -> test_q0[0];\n"
                   "measure q[2] -> test_q2[0];\n",
                   preamble1);

  const std::vector<StatSupPreambleEntry> preamble2 = {
      StatSupPreambleEntry({"test_q0", "test_q1"})};

  checkCompilation(makeSettings(1, 1),
                   "creg test_q0[1];\n"
                   "creg test_q1[1];\n"
                   "qreg q[3];\n"
                   "h q[0];\n"
                   "measure q[0] -> test_q0[0];\n"
                   "measure q[1] -> test_q1[0];\n",
                   preamble2);
}

/**
 * @brief Tests the compilation of two superposition assertions when the second
 * assertion cannot be canceled out by the first one because there is a
 * non-commuting gate between them.
 */
TEST_F(
    StatisticalSlicesCompilationTest,
    StatisticalSuperpositionOptIncludedInSupSupersetButCommutationNotPossible) {
  loadCode("qreg q[2];\n"
           "h q[0];\n"
           "assert-sup q[0];\n"
           "h q[0];\n"
           "assert-sup q[0], q[1];\n");

  const std::vector<StatSupPreambleEntry> preamble1 = {
      StatSupPreambleEntry({"test_q0"})};

  checkCompilation(makeSettings(1, 0),
                   "creg test_q0[1];\n"
                   "qreg q[2];\n"
                   "h q[0];\n"
                   "measure q[0] -> test_q0[0];\n",
                   preamble1);

  const std::vector<StatSupPreambleEntry> preamble2 = {
      StatSupPreambleEntry({"test_q0", "test_q1"})};

  checkCompilation(makeSettings(1, 1),
                   "creg test_q0[1];\n"
                   "creg test_q1[1];\n"
                   "qreg q[2];\n"
                   "h q[0];\n"
                   "h q[0];\n"
                   "measure q[0] -> test_q0[0];\n"
                   "measure q[1] -> test_q1[0];\n",
                   preamble2);

  checkNoCompilation(makeSettings(1, 2));
}

/**
 * @brief Tests the compilation of two superposition assertions when the second
 * assertion cannot be canceled out by the first one because there is a
 * non-commuting gate between them, even though it is unrelated.
 */
TEST_F(
    StatisticalSlicesCompilationTest,
    StatisticalSuperpositionOptIncludedInSupSupersetButCommutationNotPossibleButUnrelated) {
  loadCode("qreg q[2];\n"
           "h q[0];\n"
           "assert-sup q[0];\n"
           "h q[1];\n"
           "assert-sup q[0], q[1];\n");

  const std::vector<StatSupPreambleEntry> preamble1 = {
      StatSupPreambleEntry({"test_q0"})};

  checkCompilation(makeSettings(1, 0),
                   "creg test_q0[1];\n"
                   "qreg q[2];\n"
                   "h q[0];\n"
                   "measure q[0] -> test_q0[0];\n",
                   preamble1);

  const std::vector<StatSupPreambleEntry> preamble2 = {
      StatSupPreambleEntry({"test_q0", "test_q1"})};

  checkCompilation(makeSettings(1, 1),
                   "creg test_q0[1];\n"
                   "creg test_q1[1];\n"
                   "qreg q[2];\n"
                   "h q[0];\n"
                   "h q[1];\n"
                   "measure q[0] -> test_q0[0];\n"
                   "measure q[1] -> test_q1[0];\n",
                   preamble2);

  checkNoCompilation(makeSettings(1, 2));
}

/**
 * @brief Tests the compilation of an equality assertion followed by a
 * superposition assertions when the second assertion can be canceled out by the
 * first one.
 */
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalSuperpositionOptIncludedInEq) {
  loadCode("qreg q[1];\n"
           "h q[0];\n"
           "assert-eq 0.9, q[0] { 0.707, 0.707 }\n"
           "x q[0];\n"
           "assert-sup q[0];\n");

  const std::vector<StatEqPreambleEntry> preamble = {
      StatEqPreambleEntry({"test_q0"}, {0.499849, 0.499849}, 0.9)};

  checkCompilation(makeSettings(1, 0),
                   "creg test_q0[1];\n"
                   "qreg q[1];\n"
                   "h q[0];\n"
                   "measure q[0] -> test_q0[0];\n",
                   preamble);

  checkNoCompilation(makeSettings(1, 1));
}

/**
 * @brief Tests the compilation of an equality assertion followed by a
 * superposition assertions when the second assertion can be canceled out by the
 * first one as a subset of it was in a superposition in the first assertion.
 */
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalSuperpositionOptSubIncludedInEq) {
  loadCode("qreg q[3];\n"
           "h q[0];\n"
           "assert-eq 0.9, q[0], q[1] { 0.707, 0.707, 0, 0 }\n"
           "x q[0];\n"
           "assert-sup q[0], q[2];\n");

  const std::vector<StatEqPreambleEntry> preamble = {StatEqPreambleEntry(
      {"test_q0", "test_q1"}, {0.499849, 0.499849, 0, 0}, 0.9)};

  checkCompilation(makeSettings(1, 0),
                   "creg test_q0[1];\n"
                   "creg test_q1[1];\n"
                   "qreg q[3];\n"
                   "h q[0];\n"
                   "measure q[0] -> test_q0[0];\n"
                   "measure q[1] -> test_q1[0];\n",
                   preamble);

  checkNoCompilation(makeSettings(1, 1));
}

/**
 * @brief Tests the compilation of an equality assertion followed by a
 * superposition assertions when the second assertion cannot be canceled out by
 * the first one as their intersection is was not in a superposition in the
 * first assertion.
 */
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalSuperpositionOptSubNotEntangledInEq) {
  loadCode("qreg q[3];\n"
           "h q[1];\n"
           "assert-eq 0.9, q[0], q[1] { 0.707, 0, 0.707, 0 }\n"
           "x q[0];\n"
           "assert-sup q[0], q[2];\n");

  const std::vector<StatEqPreambleEntry> preamble1 = {StatEqPreambleEntry(
      {"test_q0", "test_q1"}, {0.499849, 0, 0.499849, 0}, 0.9)};

  checkCompilation(makeSettings(1, 0),
                   "creg test_q0[1];\n"
                   "creg test_q1[1];\n"
                   "qreg q[3];\n"
                   "h q[1];\n"
                   "measure q[0] -> test_q0[0];\n"
                   "measure q[1] -> test_q1[0];\n",
                   preamble1);

  const std::vector<StatSupPreambleEntry> preamble2 = {
      StatSupPreambleEntry({"test_q0", "test_q2"})};

  checkCompilation(makeSettings(1, 1),
                   "creg test_q0[1];\n"
                   "creg test_q2[1];\n"
                   "qreg q[3];\n"
                   "h q[1];\n"
                   "x q[0];\n"
                   "measure q[0] -> test_q0[0];\n"
                   "measure q[2] -> test_q2[0];\n",
                   preamble2);

  checkNoCompilation(makeSettings(1, 2));
}

/**
 * @brief Tests the compilation of an equality assertion followed by an
 * entanglement assertions when the second assertion can be canceled out by the
 * first one.
 */
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalEntanglementOptIncludedInEq) {
  loadCode(
      "qreg q[2];\n"
      "h q[0];\n"
      "cx q[0], q[1];\n"
      "assert-eq 0.99999, q[0], q[1] { 0.7071067812, 0, 0, 0.7071067812 }\n"
      "x q[0];\n"
      "assert-ent q[0], q[1];\n");

  const std::vector<StatEqPreambleEntry> preamble = {
      StatEqPreambleEntry({"test_q0", "test_q1"}, {0.5, 0, 0, 0.5}, 0.99999)};

  checkCompilation(makeSettings(1, 0),
                   "creg test_q0[1];\n"
                   "creg test_q1[1];\n"
                   "qreg q[2];\n"
                   "h q[0];\n"
                   "cx q[0], q[1];\n"
                   "measure q[0] -> test_q0[0];\n"
                   "measure q[1] -> test_q1[0];\n",
                   preamble);

  checkNoCompilation(makeSettings(1, 1));
}

/**
 * @brief Tests the compilation of two equality assertions that can be
 * combined into one execution at opt-level 2.
 */
TEST_F(StatisticalSlicesCompilationTest, StatisticalEqualityOptCombined) {
  loadCode("qreg q[3];\n"
           "x q[0];\n"
           "assert-eq q[0], q[1] { 0, 1, 0, 0 }\n"
           "assert-eq q[2] { 0, 1 }\n");

  const std::vector<StatEqPreambleEntry> preamble = {
      StatEqPreambleEntry({"test_q0", "test_q1"}, {0, 1, 0, 0}, 1),
      StatEqPreambleEntry({"test_q2"}, {0, 1}, 1)};

  checkCompilation(makeSettings(2, 0),
                   "creg test_q0[1];\n"
                   "creg test_q1[1];\n"
                   "creg test_q2[1];\n"
                   "qreg q[3];\n"
                   "x q[0];\n"
                   "measure q[0] -> test_q0[0];\n"
                   "measure q[1] -> test_q1[0];\n"
                   "measure q[2] -> test_q2[0];\n",
                   preamble);

  checkNoCompilation(makeSettings(2, 1));
}

/**
 * @brief Tests the compilation of two assertions that can be
 * combined into one execution at opt-level 2 with an instruction between.
 */
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalOptCombinedWithOperationBetween) {
  loadCode("qreg q[2];\n"
           "h q[0];\n"
           "assert-sup q[0];\n"
           "h q[0];\n"
           "assert-sup q[1];\n");

  const std::vector<StatSupPreambleEntry> preamble = {
      StatSupPreambleEntry({"test_q0"}), StatSupPreambleEntry({"test_q1"})};

  checkCompilation(makeSettings(2, 0),
                   "creg test_q0[1];\n"
                   "creg test_q1[1];\n"
                   "qreg q[2];\n"
                   "h q[0];\n"
                   "measure q[0] -> test_q0[0];\n"
                   "h q[0];\n"
                   "measure q[1] -> test_q1[0];\n",
                   preamble);

  checkNoCompilation(makeSettings(2, 1));
}

/**
 * @brief Tests the compilation of two assertions that can be
 * combined into one execution at opt-level 2 with an instruction between.
 */
TEST_F(StatisticalSlicesCompilationTest, StatisticalOptCombinedRemeasure) {
  loadCode("qreg q[2];\n"
           "h q[0];\n"
           "h q[1];\n"
           "assert-sup q[0], q[1];\n"
           "assert-sup q[0];\n");

  const std::vector<StatSupPreambleEntry> preamble = {
      StatSupPreambleEntry({"test_q0", "test_q1"}),
      StatSupPreambleEntry({"test_q0_"})};

  checkCompilation(makeSettings(2, 0),
                   "creg test_q0[1];\n"
                   "creg test_q0_[1];\n"
                   "creg test_q1[1];\n"
                   "qreg q[2];\n"
                   "h q[0];\n"
                   "h q[1];\n"
                   "measure q[0] -> test_q0[0];\n"
                   "measure q[1] -> test_q1[0];\n"
                   "measure q[0] -> test_q0_[0];\n",
                   preamble);

  checkNoCompilation(makeSettings(2, 1));
}

/**
 * @brief Tests the compilation of three assertions, where the first two can be
 * combined into one execution at opt-level 2 with an instruction between.
 */
TEST_F(StatisticalSlicesCompilationTest,
       StatisticalMixedOptCombinedWithBlockingOperationBetween) {
  loadCode("qreg q[2];\n"
           "x q[0];\n"
           "assert-eq q[0] { 0, 1 }\n"
           "h q[0];\n"
           "assert-sup q[1];\n"
           "assert-sup q[0];\n");

  const auto preamble1 = StatEqPreambleEntry({"test_q0"}, {0, 1}, 1.0);
  const auto preamble2 = StatSupPreambleEntry({"test_q1"});
  const auto preambles =
      std::vector<const PreambleEntry*>{&preamble1, &preamble2};

  CompilationTest::checkCompilation(makeSettings(2, 0),
                                    "creg test_q0[1];\n"
                                    "creg test_q1[1];\n"
                                    "qreg q[2];\n"
                                    "x q[0];\n"
                                    "measure q[0] -> test_q0[0];\n"
                                    "h q[0];\n"
                                    "measure q[1] -> test_q1[0];\n",
                                    preambles);

  const std::vector<StatSupPreambleEntry> lastPreamble = {
      StatSupPreambleEntry({"test_q0"}),
  };

  checkCompilation(makeSettings(2, 1),
                   "creg test_q0[1];\n"
                   "qreg q[2];\n"
                   "x q[0];\n"
                   "h q[0];\n"
                   "measure q[0] -> test_q0[0];\n",
                   lastPreamble);

  checkNoCompilation(makeSettings(2, 2));
}
