#include "common/parsing/CodePreprocessing.hpp"

#include "common/parsing/AssertionParsing.hpp"
#include "common/parsing/ParsingError.hpp"
#include "common/parsing/Utils.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

Instruction::Instruction(size_t inputLineNumber, std::string inputCode,
                         std::unique_ptr<Assertion>& inputAssertion,
                         std::set<std::string> inputTargets, size_t startPos,
                         size_t endPos, size_t successor, bool isFuncCall,
                         std::string function, bool inFuncDef, Block inputBlock)
    : lineNumber(inputLineNumber), code(std::move(inputCode)),
      assertion(std::move(inputAssertion)), targets(std::move(inputTargets)),
      originalCodeStartPosition(startPos), originalCodeEndPosition(endPos),
      successorIndex(successor), isFunctionCall(isFuncCall),
      calledFunction(std::move(function)), inFunctionDefinition(inFuncDef),
      block(std::move(inputBlock)) {}

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
            "$__block" + std::to_string(blocks.size()) + "$;";
        blocks[blockName] = block;
        result.replace(start, pos + 1 - start, blockName);
        pos = start;
      }
    }
  }
  return result;
}

std::string removeComments(const std::string& code) {
  std::string result = code;
  for (size_t pos = 0; pos < result.size(); pos++) {
    auto nextComment = result.find("//", pos);
    if (nextComment == std::string::npos) {
      break;
    }
    auto commentEnd = result.find('\n', nextComment);
    if (commentEnd == std::string::npos) {
      commentEnd = result.size();
    }
    const std::string spaces(commentEnd - nextComment, ' ');
    result.replace(nextComment, commentEnd - nextComment, spaces);
  }
  return result;
}

bool isFunctionDefinition(const std::string& line) {
  return startsWith(trim(line), "gate ");
}

std::vector<std::string> parseParameters(const std::string& instruction) {
  auto parts = splitString(
      replaceString(
          replaceString(replaceString(instruction, ";", " "), "\n", " "), "\t",
          " "),
      ' ');
  size_t index = 0;
  for (auto& part : parts) {
    index++;
    if (!part.empty()) {
      break;
    }
  }

  std::string parameterParts;
  for (size_t i = index; i < parts.size(); i++) {
    if (parts[i].empty()) {
      continue;
    }
    parameterParts += parts[i];
  }
  auto parameters = splitString(removeWhitespace(parameterParts), ',');

  return parameters;
}

FunctionDefinition parseFunctionDefinition(const std::string& signature) {
  auto parts = splitString(
      replaceString(replaceString(signature, "\n", " "), "\t", " "), ' ');
  std::string name;
  size_t index = 0;
  for (auto& part : parts) {
    index++;
    if (part != "gate" && !part.empty()) {
      name = part;
      break;
    }
  }

  std::string parameterParts;
  for (size_t i = index; i < parts.size(); i++) {
    parameterParts += parts[i];
  }
  auto parameters = splitString(removeWhitespace(parameterParts), ',');

  return {name, parameters};
}

std::vector<std::string> sweepFunctionNames(const std::string& code) {
  std::vector<std::string> result;
  const std::vector<char> delimiters{';', '}'};
  const auto instructions = splitString(code, delimiters);
  for (const auto& instruction : instructions) {
    if (isFunctionDefinition(instruction)) {
      const auto f = parseFunctionDefinition(instruction);
      result.push_back(f.name);
    }
  }
  return result;
}

std::vector<Instruction> preprocessCode(const std::string& code,
                                        std::string& processedCode) {
  return preprocessCode(code, 0, 0, {}, processedCode);
}

std::vector<Instruction>
preprocessCode(const std::string& code, size_t startIndex,
               size_t initialCodeOffset,
               const std::vector<std::string>& allFunctionNames,
               std::string& processedCode) {

  std::map<std::string, std::string> blocks;
  std::map<std::string, size_t> functionFirstLine;
  std::map<std::string, FunctionDefinition> functionDefinitions;
  std::map<size_t, std::vector<std::string>> variableUsages;

  processedCode = removeComments(code);
  const std::string blocksRemoved = sweepBlocks(processedCode, blocks);
  std::vector<std::string> functionNames = sweepFunctionNames(processedCode);
  for (const auto& name : allFunctionNames) {
    functionNames.push_back(name);
  }

  std::vector<Instruction> instructions;

  size_t pos = 0;
  size_t i = startIndex;
  size_t blocksOffset = initialCodeOffset;

  while (pos != std::string::npos) {
    const size_t end = blocksRemoved.find(';', pos);
    if (end == std::string::npos) {
      break;
    }

    std::string line = blocksRemoved.substr(pos, end - pos + 1);
    auto trimmedLine = trim(line);
    auto tokens = splitString(trimmedLine, ' ');
    auto isAssert = isAssertion(line);
    auto blockPos = line.find("$__block");
    const auto targetsVector = parseParameters(line);
    const std::set<std::string> targets(targetsVector.begin(),
                                        targetsVector.end());

    const size_t trueStart = pos + blocksOffset;

    Block block{false, ""};
    if (blockPos != std::string::npos) {
      const auto endPos = line.find('$', blockPos + 1) + 1;
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

    if (isFunctionDefinition(line)) {
      if (!block.valid) {
        throw std::runtime_error("Gate definitions require a body block");
      }
      const auto f = parseFunctionDefinition(line);
      functionDefinitions.insert({f.name, f});
      i++;
      std::string processedSubCode;
      auto subInstructions =
          preprocessCode(block.code, i, code.find('{', trueStart) + 1,
                         functionNames, processedSubCode);
      for (auto& instr : subInstructions) {
        instr.inFunctionDefinition = true;
      }
      functionFirstLine.insert({f.name, subInstructions[0].lineNumber});
      // successor 0 means "pop call stack"
      i += subInstructions.size();

      std::unique_ptr<Assertion> a(nullptr);
      instructions.emplace_back(i - subInstructions.size() - 1, line, a,
                                targets, trueStart, trueEnd, i + 1, false, "",
                                false, block);
      for (auto& instr : subInstructions) {
        instructions.back().childInstructions.push_back(instr.lineNumber);
      }
      instructions.insert(instructions.end(),
                          std::make_move_iterator(subInstructions.begin()),
                          std::make_move_iterator(subInstructions.end()));

      const auto closingBrace = code.find(
          '}', instructions[instructions.size() - 1].originalCodeEndPosition);
      const Block noBlock{false, ""};
      instructions.emplace_back(i, "RETURN", a, targets, closingBrace,
                                closingBrace, 0, false, "", true, noBlock);
      i++;
      pos = end + 1;

      continue;
    }

    bool isFunctionCall = false;
    std::string calledFunction;
    if (!tokens.empty() && std::find(functionNames.begin(), functionNames.end(),
                                     tokens[0]) != functionNames.end()) {
      isFunctionCall = true;
      calledFunction = tokens[0];
    }

    if (isAssert) {
      auto a = parseAssertion(line, block.code);
      instructions.emplace_back(i, line, a, targets, trueStart, trueEnd, i + 1,
                                isFunctionCall, calledFunction, false, block);
    } else {
      std::unique_ptr<Assertion> a(nullptr);
      instructions.emplace_back(i, line, a, targets, trueStart, trueEnd, i + 1,
                                isFunctionCall, calledFunction, false, block);

      variableUsages.insert({i, parseParameters(line)});
    }

    i++;
    pos = end + 1;
  }

  for (auto& instr : instructions) {
    auto vars = parseParameters(instr.code);
    size_t idx = instr.lineNumber - 1;
    while (!vars.empty() && (instr.lineNumber < instructions.size() ||
                             idx > instr.lineNumber - instructions.size())) {
      bool found = false;
      for (const auto& var : variableUsages[idx]) {
        if (std::find(vars.begin(), vars.end(), var) != vars.end()) {
          found = true;
          const auto newEnd = std::remove(vars.begin(), vars.end(), var);
          vars.erase(newEnd, vars.end());
        }
      }
      if (found) {
        instr.dataDependencies.push_back(idx);
      }
      if (idx - 1 == instr.lineNumber - instructions.size()) {
        break;
      }
      idx--;
    }
    if (instr.isFunctionCall) {
      instr.successorIndex = functionFirstLine[instr.calledFunction];
      if (functionDefinitions.find(instr.calledFunction) ==
          functionDefinitions.end()) {
        continue;
      }
      instr.callSubstitution.clear();
      const auto func = functionDefinitions[instr.calledFunction];
      const auto arguments = parseParameters(instr.code);
      if (func.parameters.size() != arguments.size()) {
        throw ParsingError(
            "Custom gate call uses incorrect number of arguments.");
      }
      for (size_t j = 0; j < func.parameters.size(); j++) {
        instr.callSubstitution.insert({func.parameters[j], arguments[j]});
      }
    }
  }

  return instructions;
}
