#include "common/parsing/AssertionTools.hpp"

#include "common/parsing/AssertionParsing.hpp"
#include "common/parsing/CodePreprocessing.hpp"
#include "common/parsing/Utils.hpp"

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#define YES(expression)                                                        \
  (expression) ? CommutativityResult::Commutes : CommutativityResult::Unknown;
#define NO(expression)                                                         \
  (expression) ? CommutativityResult::DoesNotCommute                           \
               : CommutativityResult::Unknown;

//------------------------------------------------------------------------------
// General rules
//------------------------------------------------------------------------------

// Barrier instructions will not remove or create entanglement or superposition
static COMMUTATIVITY_RULE_GENERAL(BARRIER, YES(instructionName == "barrier"));

//------------------------------------------------------------------------------
// Entanglement rules
//------------------------------------------------------------------------------

// 1-qubit gates will not remove or create entanglement
static COMMUTATIVITY_RULE_ENT(TWO_OR_MORE_TARGETS, YES(arguments.size() < 2));

//------------------------------------------------------------------------------
// Superposition rules
//------------------------------------------------------------------------------

// Pauli-Instructions will not remove or create superposition
static COMMUTATIVITY_RULE_SUP(PAULI_INVARIANT, YES(instructionName == "x" ||
                                                   instructionName == "y" ||
                                                   instructionName == "z"));

// `S` and `T` gates will not remove or create superposition
static COMMUTATIVITY_RULE_SUP(OTHER_1Q_GATE_INVARIANTS,
                              YES(instructionName == "s" ||
                                  instructionName == "t" ||
                                  instructionName == "sdg" ||
                                  instructionName == "tdg"));

//------------------------------------------------------------------------------

static const std::vector<std::function<CommutativityResult(
    const Assertion*, const std::string&, const std::vector<std::string>&)>>
    GENERAL_COMMUTATIVITY_RULES = {
        BARRIER,
};

static const std::vector<std::function<CommutativityResult(
    const EntanglementAssertion*, const std::string&,
    const std::vector<std::string>&)>>
    ENTANGLEMENT_COMMUTATIVITY_RULES = {
        TWO_OR_MORE_TARGETS,
};

static const std::vector<std::function<CommutativityResult(
    const SuperpositionAssertion*, const std::string&,
    const std::vector<std::string>&)>>
    SUPERPOSITION_COMMUTATIVITY_RULES = {
        PAULI_INVARIANT,
        OTHER_1Q_GATE_INVARIANTS,
};

bool doesCommuteEnt(const EntanglementAssertion* assertion,
                    const std::string& instructionName,
                    const std::vector<std::string>& targets) {
  for (const auto& rule : ENTANGLEMENT_COMMUTATIVITY_RULES) {
    const auto result = rule(assertion, instructionName, targets);
    if (result != CommutativityResult::Unknown) {
      return result == CommutativityResult::Commutes;
    }
  }
  return false;
}

bool doesCommuteSup(const SuperpositionAssertion* assertion,
                    const std::string& instructionName,
                    const std::vector<std::string>& targets) {
  for (const auto& rule : SUPERPOSITION_COMMUTATIVITY_RULES) {
    const auto result = rule(assertion, instructionName, targets);
    if (result != CommutativityResult::Unknown) {
      return result == CommutativityResult::Commutes;
    }
  }
  return false;
}

bool doesCommute(const std::unique_ptr<Assertion>& assertion,
                 const std::string& instruction) {
  const auto targets = parseParameters(instruction);
  const auto instructionName = splitString(trim(instruction), ' ')[0];
  for (const auto& rule : GENERAL_COMMUTATIVITY_RULES) {
    const auto result = rule(assertion.get(), instructionName, targets);
    if (result != CommutativityResult::Unknown) {
      return result == CommutativityResult::Commutes;
    }
  }

  if (assertion->getType() == AssertionType::Entanglement) {
    const auto* entAssertion =
        dynamic_cast<EntanglementAssertion*>(assertion.get());
    const auto result = doesCommuteEnt(entAssertion, instructionName, targets);
    return result;
  }
  if (assertion->getType() == AssertionType::Superposition) {
    const auto* supAssertion =
        dynamic_cast<SuperpositionAssertion*>(assertion.get());
    const auto result = doesCommuteSup(supAssertion, instructionName, targets);
    return result;
  }
  if (assertion->getType() == AssertionType::CircuitEquality ||
      assertion->getType() == AssertionType::StatevectorEquality) {
    // Equality assertions are not commutative with dependent operations, as any
    // dependent operation will (likely) change the state of the qubits.
    return false;
  }
  return false;
}

bool doesCommute(const std::unique_ptr<Assertion>& assertion,
                 const Instruction& instruction) {
  const auto& code = instruction.code;
  if (instruction.assertion != nullptr) {
    return false; // The order of assertions should remain stable.
  }
  if (instruction.isFunctionDefinition) {
    return true; // Order of function definitions does not matter.
  }
  if (isVariableDeclaration(code)) {
    // Order of unrelated variable declarations does not matter.
    const auto targets = parseParameters(code);
    const auto registerName = variableBaseName(targets[0]);
    return std::none_of(assertion->getTargetQubits().begin(),
                        assertion->getTargetQubits().end(),
                        [&registerName](const auto& target) {
                          return registerName == variableBaseName(target);
                        });
  }
  if (isMeasurement(code)) {
    return false; // Assertions should never be moved above measurements.
                  // [UNLESS measurement and assertion targets are not
                  // entangled, but this cannot be checked in advance.]
  }
  if (isReset(code)) {
    return false; // Assertions should never be moved above resets. [UNLESS
                  // measurement and assertion targets are not entangled, but
                  // this cannot be checked in advance.]
  }
  if (isClassicControlledGate(code)) {
    // For classic-controlled gates, the classical parts do not matter, so we
    // only need to focus on the quantum parts.
    // NOTE: This will not work, if the operation is a function call and
    // requires
    //       issue https://github.com/cda-tum/mqt-debugger/issues/29 to first be
    //       resolved in that situation.
    const auto c = parseClassicControlledGate(code).operations;
    return std::all_of(c.begin(), c.end(), [&assertion](const auto& operation) {
      return doesCommute(assertion, operation);
    });
  }

  const auto& assertionTargets = assertion->getTargetQubits();
  const auto& instructionTargets = instruction.targets;
  if (std::none_of(assertionTargets.begin(), assertionTargets.end(),
                   [&instructionTargets](const auto& target) {
                     return std::any_of(
                         instructionTargets.begin(), instructionTargets.end(),
                         [&target](const std::string& instrTarget) {
                           return (instrTarget.find('[') != std::string::npos &&
                                   instrTarget == target) ||
                                  (instrTarget.find('[') == std::string::npos &&
                                   variableBaseName(target) == instrTarget);
                         });
                   })) {
    return true; // If the assertion does not target any of the qubits in the
                 // instruction, the order does not matter.
  }

  return doesCommute(assertion, instruction.code);
}
