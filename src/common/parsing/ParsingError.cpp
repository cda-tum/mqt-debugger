/**
 * @file ParsingError.cpp
 * @brief Implementation of the ParsingError class.
 */

#include "common/parsing/ParsingError.hpp"

#include <stdexcept>
#include <string>

ParsingError::ParsingError(const std::string& msg) : std::runtime_error(msg) {}
