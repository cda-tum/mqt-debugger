/**
 * @file ParsingError.hpp
 * @brief Header file for the ParsingError class
 */

#pragma once

#include <stdexcept>
#include <string>

/**
 * @brief Represents an error that occurred during parsing.
 */
class ParsingError : public std::runtime_error {
public:
  /**
   * @brief Constructs a new ParsingError with the given message.
   * @param msg The error message.
   */
  explicit ParsingError(const std::string& msg);
};
