/**
 * @file test_compilation_projective_measurements.cpp
 * @brief Tests the correctness of the assertion compilation process for
 * assertion programs using projective measurements.
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
 * @brief A preamble entry for projective measurement assertions.
 */
class ProjPreambleEntry : public PreambleEntry {
  /**
   * @brief The name of the variable the preamble entry is for.
   */
  std::vector<std::string> names;

public:
  /**
   * @brief Constructs a new ProjPreambleEntry with the given names
   * @param n The names of the variables the preamble entry is for.
   */
  explicit ProjPreambleEntry(std::vector<std::string> n)
      : names(std::move(n)) {}

  [[nodiscard]] std::string toString() const override {
    std::stringstream ss;
    ss << "// ASSERT: (";
    for (size_t i = 0; i < names.size(); i++) {
      ss << names[i];
      if (i < names.size() - 1) {
        ss << ",";
      }
    }
    ss << ") {zero}\n";
    return ss.str();
  }
};

/**
 * @brief A test fixture for testing the compilation of assertion programs using
 * projective measurements.
 */
class ProjectiveMeasurementsCompilationTest : public CompilationTest {
public:
  /**
   * @brief Creates a new CompilationSettings object for projective
   * measurements.
   * @param opt The optimization level to use.
   * @return The created CompilationSettings object.
   */
  static CompilationSettings makeSettings(uint8_t opt) {
    return {
        /*mode=*/CompilationMode::PROJECTIVE_MEASUREMENTS,
        /*opt=*/opt,
        /*sliceIndex=*/0,
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
                   const std::vector<ProjPreambleEntry>& expectedPreamble) {
    auto pointerVector =
        std::vector<const PreambleEntry*>(expectedPreamble.size());
    std::transform(expectedPreamble.begin(), expectedPreamble.end(),
                   pointerVector.begin(),
                   [](const ProjPreambleEntry& entry) { return &entry; });
    CompilationTest::checkCompilation(settings, expected, pointerVector);
  }
};

/**
 * @brief Tests the compilation of a simple equality assertion using
 * projective measurements and no optimization.
 */
TEST_F(ProjectiveMeasurementsCompilationTest, ProjectiveSingleOperation) {
  loadCode("qreg q[1];\n"
           "x q[0];\n"
           "assert-eq q[0] { "
           "qreg p[1];\n"
           "x p[0];\n"
           "}\n");

  const std::vector<ProjPreambleEntry> preamble = {
      ProjPreambleEntry({"test_q0"})};

  checkCompilation(makeSettings(/*opt=*/0),
                   "creg test_q0[1];\n"
                   "qreg q[1];\n"
                   "x q[0];\n"
                   "x q[0];\n"
                   "measure q[0] -> test_q0[0];\n"
                   "x q[0];\n",
                   preamble);
}

/**
 * @brief Tests the compilation of a equality assertion consisting of multiple
 * instructions using projective measurements and no optimization.
 */
TEST_F(ProjectiveMeasurementsCompilationTest,
       ProjectiveSingleQubitMultipleOperations) {
  loadCode("qreg q[1];\n"
           "h q[0];\n"
           "z q[0];\n"
           "h q[0];\n"
           "assert-eq q[0] { "
           "qreg p[1];\n"
           "x p[0];\n"
           "}\n");

  const std::vector<ProjPreambleEntry> preamble = {
      ProjPreambleEntry({"test_q0"})};

  checkCompilation(makeSettings(/*opt=*/0),
                   "creg test_q0[1];\n"
                   "qreg q[1];\n"
                   "h q[0];\n"
                   "z q[0];\n"
                   "h q[0];\n"
                   "x q[0];\n"
                   "measure q[0] -> test_q0[0];\n"
                   "x q[0];\n",
                   preamble);
}

/**
 * @brief Tests the compilation of a two-qubit equality assertion using
 * projective measurements and no optimization.
 */
TEST_F(ProjectiveMeasurementsCompilationTest,
       ProjectiveMultiQubitMultipleOperations) {
  loadCode("qreg q[2];\n"
           "h q[0];\n"
           "cx q[0], q[1];\n"
           "assert-eq q[0], q[1] { "
           "qreg p[2];\n"
           "h p[1];\n"
           "cx p[1], p[0];\n"
           "}\n");

  const std::vector<ProjPreambleEntry> preamble = {
      ProjPreambleEntry({"test_q0", "test_q1"})};

  checkCompilation(makeSettings(/*opt=*/0),
                   "creg test_q0[1];\n"
                   "creg test_q1[1];\n"
                   "qreg q[2];\n"
                   "h q[0];\n"
                   "cx q[0], q[1];\n"
                   "cx q[1], q[0];\n"
                   "h q[1];\n"
                   "measure q[0] -> test_q0[0];\n"
                   "measure q[1] -> test_q1[0];\n"
                   "h q[1];\n"
                   "cx q[1], q[0];\n",
                   preamble);
}

/**
 * @brief Tests the compilation of an equality assertion followed by some
 * additional instructions, using projective measurements and no optimization.
 */
TEST_F(ProjectiveMeasurementsCompilationTest,
       ProjectiveOperationsAfterAssertion) {
  loadCode("qreg q[1];\n"
           "x q[0];\n"
           "assert-eq q[0] { "
           "qreg p[1];\n"
           "x p[0];\n"
           "}\n"
           "h q[0];\n"
           "x q[0];\n");

  const std::vector<ProjPreambleEntry> preamble = {
      ProjPreambleEntry({"test_q0"})};

  checkCompilation(makeSettings(/*opt=*/0),
                   "creg test_q0[1];\n"
                   "qreg q[1];\n"
                   "x q[0];\n"
                   "x q[0];\n"
                   "measure q[0] -> test_q0[0];\n"
                   "x q[0];\n"
                   "h q[0];\n"
                   "x q[0];\n",
                   preamble);
}

/**
 * @brief Tests the compilation of an equality assertion that considers only a
 * substate of the full state vector, using projective measurements and no
 * optimization.
 */
TEST_F(ProjectiveMeasurementsCompilationTest, ProjectiveSubstateAssertion) {
  loadCode("qreg q[2];\n"
           "x q[1];\n"
           "h q[0];\n"
           "assert-eq q[1] { "
           "qreg p[1];\n"
           "h p[0];\n"
           "z p[0];\n"
           "h p[0];\n"
           "}\n");

  const std::vector<ProjPreambleEntry> preamble = {
      ProjPreambleEntry({"test_q1"})};

  checkCompilation(makeSettings(/*opt=*/0),
                   "creg test_q1[1];\n"
                   "qreg q[2];\n"
                   "x q[1];\n"
                   "h q[0];\n"
                   "h q[1];\n"
                   "z q[1];\n"
                   "h q[1];\n"
                   "measure q[1] -> test_q1[0];\n"
                   "h q[1];\n"
                   "z q[1];\n"
                   "h q[1];\n",
                   preamble);
}

/**
 * @brief Tests the compilation of multiple equality assertions using
 * projective measurements and no optimization.
 */
TEST_F(ProjectiveMeasurementsCompilationTest,
       ProjectiveMultiAssertionSingleOperation) {
  loadCode("qreg q[1];\n"
           "x q[0];\n"
           "assert-eq q[0] { "
           "qreg p[1];\n"
           "x p[0];\n"
           "}\n"
           "assert-eq q[0] { "
           "qreg p[1];\n"
           "h p[0];\n"
           "z p[0];\n"
           "h p[0];\n"
           "}\n");

  const std::vector<ProjPreambleEntry> preamble = {
      ProjPreambleEntry({"test_q0"}), ProjPreambleEntry({"test_q0_"})};

  checkCompilation(makeSettings(/*opt=*/0),
                   "creg test_q0[1];\n"
                   "creg test_q0_[1];\n"
                   "qreg q[1];\n"
                   "x q[0];\n"
                   "x q[0];\n"
                   "measure q[0] -> test_q0[0];\n"
                   "x q[0];\n"
                   "h q[0];\n"
                   "z q[0];\n"
                   "h q[0];\n"
                   "measure q[0] -> test_q0_[0];\n"
                   "h q[0];\n"
                   "z q[0];\n"
                   "h q[0];\n",
                   preamble);
}

/**
 * @brief Tests the compilation of multiple equality assertions with other
 * instructions between them, using projective measurements and no optimization.
 */
TEST_F(ProjectiveMeasurementsCompilationTest,
       ProjectiveMultiAssertionOperationsBetween) {
  loadCode("qreg q[1];\n"
           "x q[0];\n"
           "assert-eq q[0] { "
           "qreg p[1];\n"
           "x p[0];\n"
           "}\n"
           "z q[0];\n"
           "assert-eq q[0] { "
           "qreg p[1];\n"
           "x p[0];\n"
           "z p[0];\n"
           "}\n");

  const std::vector<ProjPreambleEntry> preamble = {
      ProjPreambleEntry({"test_q0"}), ProjPreambleEntry({"test_q0_"})};

  checkCompilation(makeSettings(/*opt=*/0),
                   "creg test_q0[1];\n"
                   "creg test_q0_[1];\n"
                   "qreg q[1];\n"
                   "x q[0];\n"
                   "x q[0];\n"
                   "measure q[0] -> test_q0[0];\n"
                   "x q[0];\n"
                   "z q[0];\n"
                   "z q[0];\n"
                   "x q[0];\n"
                   "measure q[0] -> test_q0_[0];\n"
                   "x q[0];\n"
                   "z q[0];\n",
                   preamble);
}
