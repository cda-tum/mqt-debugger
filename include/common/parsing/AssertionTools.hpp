#pragma once

#include "AssertionParsing.hpp"
#include "CodePreprocessing.hpp"

#include <memory>

#define COMMUTATIVITY_RULE_GENERAL(name, expression)                           \
  COMMUTATIVITY_RULE_TEMPLATE(Assertion, name, expression)

#define COMMUTATIVITY_RULE_ENT(name, expression)                               \
  COMMUTATIVITY_RULE_TEMPLATE(EntanglementAssertion, name, expression)

#define COMMUTATIVITY_RULE_SUP(name, expression)                               \
  COMMUTATIVITY_RULE_TEMPLATE(SuperpositionAssertion, name, expression)

#define COMMUTATIVITY_RULE_TEMPLATE(type, name, expression)                    \
  const auto name = [](const type* assertion,                                  \
                       const std::string& instructionName,                     \
                       const std::vector<std::string>& arguments) {            \
    (void)assertion;                                                           \
    (void)instructionName;                                                     \
    (void)arguments;                                                           \
    return expression;                                                         \
  }

enum class CommutativityResult {
  Commutes,
  DoesNotCommute,
  Unknown,
};

bool doesCommute(const std::unique_ptr<Assertion>& assertion,
                 const Instruction& instruction);
