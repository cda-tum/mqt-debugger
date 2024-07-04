#pragma once

#include "AssertionParsing.hpp"

#include <string>

struct Block {
  bool valid;
  std::string code;
};

struct Instruction {
  size_t lineNumber;
  std::string code;
  std::unique_ptr<Assertion> assertion;

  size_t originalCodeStartPosition;
  size_t originalCodeEndPosition;

  Block block;
  Instruction(size_t lineNumber, std::string code,
              std::unique_ptr<Assertion>& assertion,
              size_t originalCodeStartPosition, size_t originalCodeEndPosition,
              Block block);
};

std::vector<Instruction> preprocessCode(const std::string& code);
