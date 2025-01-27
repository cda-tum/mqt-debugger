/**
 * @file utils_test.cpp
 * @brief Implementation of utility functions for testing.
 *
 * This file implements several utility functions that are used in the test
 * files.
 */

#include "utils/utils_test.hpp"

#include "common.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

bool complexEquality(const Complex& c, double real, double imaginary) {
  const double epsilon = 0.001;
  if (real - c.real > epsilon || c.real - real > epsilon) {
    return false;
  }
  if (imaginary - c.imaginary > epsilon || c.imaginary - imaginary > epsilon) {
    return false;
  }
  return true;
}

bool classicalEquals(const Variable& v, bool value) {
  return v.type == VarBool && v.value.boolValue == value;
}

std::string readFromCircuitsPath(const std::string& testName) {
  const std::filesystem::path localPath =
      std::filesystem::path("circuits") / (testName + std::string(".qasm"));
  std::ifstream file(localPath);
  if (!file.is_open()) {
    file = std::ifstream(std::filesystem::path("../../test/circuits") /
                         (testName + std::string(".qasm")));
    if (!file.is_open()) {
      std::cerr << "Could not open file\n";
      file.close();
      return "";
    }
  }

  const std::string code((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
  file.close();
  return code;
}

/**
 * @brief Generate a string representation of a double without trailing zeros.
 *
 * @param d The double to convert to a string.
 * @return The string representation of the double.
 */
std::string doubleToStringTest(const double d) {
  auto string = std::to_string(d);
  while (string.back() == '0') {
    string.pop_back();
  }
  if (string.back() == '.') {
    string.pop_back();
  }
  return string;
}

std::string complexToStringTest(const Complex& c) {
  const double epsilon = 0.0000001;
  if (c.imaginary < epsilon && c.imaginary > -epsilon) {
    return doubleToStringTest(c.real);
  }
  if (c.real < epsilon && c.real > -epsilon) {
    return doubleToStringTest(c.imaginary) + "i";
  }
  return doubleToStringTest(c.real) + " + " + doubleToStringTest(c.imaginary) +
         "i";
}

PreambleEntry realPreamble(std::vector<std::string> names,
                           std::vector<double> distribution, double fidelity) {
  std::vector<Complex> complexDistribution(distribution.size());
  std::transform(distribution.begin(), distribution.end(),
                 complexDistribution.begin(),
                 [](double value) { return Complex{value, 0.0}; });
  return {std::move(names), complexDistribution, fidelity};
}
