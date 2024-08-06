#include "utils/utils_test.hpp"

#include <fstream>
#include <iostream>

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
  std::ifstream file("circuits/" + testName + ".qasm");
  if (!file.is_open()) {
    file = std::ifstream("../../test/circuits/" + testName + ".qasm");
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
