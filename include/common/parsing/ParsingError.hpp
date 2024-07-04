#pragma once

#include <stdexcept>
class ParsingError : public std::runtime_error {
public:
  explicit ParsingError(const std::string& msg);
};
