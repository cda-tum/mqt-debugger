/**
 * @file utils_test.cpp
 * @brief Utility functions for testing.
 */

#pragma once
#include "common.h"

#include <algorithm>
#include <string>
#include <vector>

/**
 * @brief Check if the given complex number is equal to the given real and
 * imaginary parts.
 *
 * Equality is tested up to an epsilon of 0.001.
 *
 * @param c The complex number to check.
 * @param real The real part to compare to.
 * @param imaginary The imaginary part to compare to.
 * @return True if the complex number is equal to the given parts, false
 * otherwise.
 */
bool complexEquality(const Complex& c, double real, double imaginary);

/**
 * @brief Check if the given variable is equal to the given boolean value.
 *
 * @param v The variable to check.
 * @param value The value to compare to.
 * @return True if the variable is equal to the given value, false otherwise.
 */
bool classicalEquals(const Variable& v, bool value);

/**
 * @brief Read the content of the `.qasm` file with the given name from the
 * `circuits` directory.
 * @param testName The name of the file to read (not including the `circuits`
 * directory path and the extension).
 * @return The content of the file as a string.
 */
std::string readFromCircuitsPath(const std::string& testName);

/**
 * @brief Generate a string representation of a complex number.
 * @param c The complex number.
 * @return The string representation of the complex number.
 */
std::string complexToStringTest(const Complex& c);

/**
 * @brief Represents a preamble entry in the compiled code.
 */
struct PreambleEntry {
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
};

/**
 * @brief Create a preamble entry consisting of only real values.
 * @param names The name of the variable the preamble entry is for.
 * @param distribution
 * @param fidelity
 * @return
 */
PreambleEntry realPreamble(std::vector<std::string> names,
                           std::vector<double> distribution, double fidelity);
