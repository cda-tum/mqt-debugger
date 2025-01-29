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
};
