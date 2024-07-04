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

  Block block;
  Instruction(size_t lineNumber, std::string code,
              std::unique_ptr<Assertion>& assertion, Block block);
};

std::vector<Instruction> preprocessCode(const std::string& code);
