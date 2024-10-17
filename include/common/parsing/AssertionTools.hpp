#pragma once

#include "AssertionParsing.hpp"
#include "CodePreprocessing.hpp"

#include <memory>

#define COMMUTATIVITY_RULE_ENT(name, expression)                               \
  const auto name = [](const EntanglementAssertion* assertion,                 \
                       const std::string& instructionName,                     \
                       const std::vector<std::string>& arguments) {            \
    (void)assertion;                                                           \
    (void)instructionName;                                                     \
    (void)arguments;                                                           \
    return expression;                                                         \
  }

#define COMMUTATIVITY_RULE_SUP(name, expression)                               \
  const auto name = [](const SuperpositionAssertion* assertion,                \
                       const std::string& instructionName,                     \
                       const std::vector<std::string>& arguments) {            \
    (void)assertion;                                                           \
    (void)instructionName;                                                     \
    (void)arguments;                                                           \
    return expression;                                                         \
  }

bool doesCommute(const std::unique_ptr<Assertion>& assertion,
                 const Instruction& instruction);
