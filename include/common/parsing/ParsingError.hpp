#pragma once

#include <stdexcept>
#include <string>

class ParsingError : public std::runtime_error {
public:
  explicit ParsingError(const std::string& msg);
};
