#include "common/parsing/AssertionTools.hpp"

#include "common/parsing/AssertionParsing.hpp"
#include "common/parsing/CodePreprocessing.hpp"
#include "common/parsing/Utils.hpp"

#include <algorithm>
#include <memory>
#include <string>

bool doesCommuteEnt(const std::unique_ptr<EntanglementAssertion>& assertion,
                    const std::string& instruction) {
  const auto targets = parseParameters(instruction);
  return targets.size() < 2; // If the instruction does not target at least two
                             // qubits, it cannot influence entanglement.
  // In theory, even more could be done here, but for now we leave it like this.
}

bool doesCommuteSup(const std::unique_ptr<SuperpositionAssertion>& assertion,
                    const std::string& instruction) {
  const auto targets = parseParameters(instruction);
  if (targets.size() >= 2) {
    return false; // For controlled gates, it's hard to say how they would
                  // influence the superposition of the qubits.
  }
  const auto name = splitString(trim(instruction), ' ')[0];
  if (name == "x" || name == "y" || name == "z" || name == "s" || name == "t" ||
      name == "sdg" || name == "tdg") {
    return true; // Most common Single-qubit gates commute with superposition
                 // assertions.
  }
  return false; // For other gates, it's hard to say how they would influence
                // the superposition of the qubits.
}

bool doesCommute(const std::unique_ptr<Assertion>& assertion,
                 const std::string& instruction) {
  if (isBarrier(instruction)) {
    return true; // Order of barriers does not matter.
  }

  if (assertion->getType() == AssertionType::Entanglement) {
    return doesCommuteEnt(
        std::unique_ptr<EntanglementAssertion>(
            dynamic_cast<EntanglementAssertion*>(assertion.get())),
        instruction);
  }
  if (assertion->getType() == AssertionType::Superposition) {
    return doesCommuteSup(
        std::unique_ptr<SuperpositionAssertion>(
            dynamic_cast<SuperpositionAssertion*>(assertion.get())),
        instruction);
  }
  if (assertion->getType() == AssertionType::CircuitEquality ||
      assertion->getType() == AssertionType::StatevectorEquality) {
    // Equality assertions are not commutative with dependent operations, as any
    // dependent operation will (likely) change the state of the qubits.
    return false;
  }
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
    return true; // Order of unrelated variable declarations does not matter.
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

  if (instruction.isFunctionCall) {
    return false; // TODO this is a bit more complex
  }

  const auto& assertionTargets = assertion->getTargetQubits();
  const auto& instructionTargets = instruction.targets;
  if (std::none_of(assertionTargets.begin(), assertionTargets.end(),
                   [&instructionTargets](const auto& target) {
                     return std::find(instructionTargets.begin(),
                                      instructionTargets.end(),
                                      target) != instructionTargets.end();
                   })) {
    return true; // If the assertion does not target any of the qubits in the
                 // instruction, the order does not matter.
  }

  return doesCommute(assertion, instruction.code);
}
