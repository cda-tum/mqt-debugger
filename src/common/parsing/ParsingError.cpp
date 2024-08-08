#include "common/parsing/ParsingError.hpp"

#include <stdexcept>
#include <string>

ParsingError::ParsingError(const std::string& msg) : std::runtime_error(msg) {}
