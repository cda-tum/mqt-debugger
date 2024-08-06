#pragma once
#include "common.h"

#include <string>

bool complexEquality(const Complex& c, double real, double imaginary);
bool classicalEquals(const Variable& v, bool value);
std::string readFromCircuitsPath(const std::string& testName);
