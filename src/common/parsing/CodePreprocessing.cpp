#include "common/parsing/CodePreprocessing.hpp"

#include "common/parsing/Utils.hpp"

#include <map>
#include <sstream>
#include <utility>

Instruction::Instruction(size_t inputLineNumber, std::string inputCode,
                         std::unique_ptr<Assertion>& inputAssertion,
                         Block inputBlock)
    : lineNumber(inputLineNumber), code(std::move(inputCode)),
      assertion(std::move(inputAssertion)), block(std::move(inputBlock)) {}

std::string sweepBlocks(const std::string& code,
                        std::map<std::string, std::string>& blocks) {
  std::string result = code;
  size_t start = 0;
  int level = 0;
  for (size_t pos = 0; pos < result.size(); pos++) {
    auto c = result[pos];
    if (c == '{') {
      if (level == 0) {
        start = pos;
      }
      level++;
    } else if (c == '}') {
      level--;
      if (level == 0) {
        const std::string block = result.substr(start + 1, pos - 1 - start);
        const std::string blockName =
            "$__block" + std::to_string(blocks.size()) + "$";
        blocks[blockName] = block;
        result.replace(start, pos + 1 - start, blockName);
        pos = start;
      }
    }
  }
  return result;
}

std::vector<Instruction> preprocessCode(const std::string& code) {
  std::map<std::string, std::string> blocks;
  const std::string blocksRemoved = sweepBlocks(code, blocks);

  std::vector<Instruction> instructions;

  std::vector<std::string> lines;
  std::string token;
  std::istringstream tokenStream(blocksRemoved);
  while (std::getline(tokenStream, token, ';')) {
    if (replaceString(token, "\n", "").empty()) {
      continue;
    }
    lines.push_back(replaceString(token, "\n", ""));
  }

  size_t i = 0;
  for (auto& line : lines) {
    auto isAssert = isAssertion(line);
    auto blockPos = line.find("$__block");
    Block block{false, ""};
    if (blockPos != std::string::npos) {
      const auto endPos = line.find("$", blockPos + 1);
      const auto blockName = line.substr(blockPos, endPos - blockPos + 1);
      const auto blockContent = blocks[blockName];
      block.code = blockContent;
      block.valid = true;
      line.replace(blockPos, endPos - blockPos + 1, "");
    }
    if (isAssert) {
      auto a = parseAssertion(line, block.code);
      instructions.emplace_back(i, line, a, block);
    } else {
      std::unique_ptr<Assertion> a(nullptr);
      instructions.emplace_back(i, line, a, block);
    }
    i++;
  }

  return instructions;
}
