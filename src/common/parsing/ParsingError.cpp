#include "common/parsing/ParsingError.hpp"

ParsingError::ParsingError(const std::string& msg) : std::runtime_error(msg) {}
