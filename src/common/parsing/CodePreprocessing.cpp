#include "common/parsing/CodePreprocessing.hpp"

#include "common/parsing/Utils.hpp"

#include <map>
#include <utility>

Instruction::Instruction(size_t inputLineNumber, std::string inputCode,
                         std::unique_ptr<Assertion>& inputAssertion,
                         size_t startPos, size_t endPos, Block inputBlock)
    : lineNumber(inputLineNumber), code(std::move(inputCode)),
      assertion(std::move(inputAssertion)), originalCodeStartPosition(startPos),
      originalCodeEndPosition(endPos), block(std::move(inputBlock)) {}

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

  size_t pos = 0;
  size_t i = 0;
  size_t blocksOffset = 0;

  while (pos != std::string::npos) {
    const size_t end = blocksRemoved.find(';', pos);
    if (end == std::string::npos) {
      break;
    }

    std::string line = blocksRemoved.substr(pos, end - pos + 1);
    auto isAssert = isAssertion(line);
    auto blockPos = line.find("$__block");

    const size_t trueStart = pos + blocksOffset;

    Block block{false, ""};
    if (blockPos != std::string::npos) {
      const auto endPos = line.find('$', blockPos + 1);
      const auto blockName = line.substr(blockPos, endPos - blockPos + 1);
      const auto blockContent = blocks[blockName];

      // in the actual code, the current instruction is longer, because we
      // replaced the block with its name. Also, we add +2 because the block
      // also had `{` and `}`, which is not included in `blockContent`.
      blocksOffset += blockContent.size() + 2 - blockName.size();
      block.code = blockContent;
      block.valid = true;
      line.replace(blockPos, endPos - blockPos + 1, "");
    }

    const size_t trueEnd = end + blocksOffset;

    if (isAssert) {
      auto a = parseAssertion(line, block.code);
      instructions.emplace_back(i, line, a, trueStart, trueEnd, block);
    } else {
      std::unique_ptr<Assertion> a(nullptr);
      instructions.emplace_back(i, line, a, trueStart, trueEnd, block);
    }
    i++;
    pos = end + 1;
  }

  return instructions;
}
