#include "backend/dd/DDSimDiagnostics.hpp"

#include "backend/dd/DDSimDebug.hpp"
#include "backend/diagnostics.h"
#include "common.h"
#include "common/Span.hpp"
#include "common/parsing/AssertionParsing.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

DDDiagnostics* toDDDiagnostics(Diagnostics* diagnostics) {
  // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<DDDiagnostics*>(diagnostics);
  // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
}

bool* toBoolArray(uint8_t* array) {
  // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<bool*>(array);
  // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
}

Result createDDDiagnostics(DDDiagnostics* self, DDSimulationState* state) {
  self->simulationState = state;

  self->interface.init = dddiagnosticsInit;
  self->interface.getNumQubits = dddiagnosticsGetNumQubits;
  self->interface.getInstructionCount = dddiagnosticsGetInstructionCount;
  self->interface.getInteractions = dddiagnosticsGetInteractions;
  self->interface.getDataDependencies = dddiagnosticsGetDataDependencies;
  self->interface.getZeroControlInstructions =
      dddiagnosticsGetZeroControlInstructions;
  self->interface.potentialErrorCauses = dddiagnosticsPotentialErrorCauses;

  return self->interface.init(&self->interface);
}

Result destroyDDDiagnostics(DDDiagnostics* self) {
  self->simulationState = nullptr;
  return OK;
}

size_t dddiagnosticsGetNumQubits(Diagnostics* self) {
  const auto* ddd = toDDDiagnostics(self);
  return ddd->simulationState->interface.getNumQubits(
      &ddd->simulationState->interface);
}

size_t dddiagnosticsGetInstructionCount(Diagnostics* self) {
  const auto* ddd = toDDDiagnostics(self);
  return ddd->simulationState->interface.getInstructionCount(
      &ddd->simulationState->interface);
}

Result dddiagnosticsInit(Diagnostics* self) {
  auto* ddd = toDDDiagnostics(self);
  ddd->zeroControls.clear();
  ddd->nonZeroControls.clear();
  ddd->actualQubits.clear();
  return OK;
}

size_t findReturn(DDSimulationState* state, size_t instruction) {
  size_t current = instruction;
  while (state->instructionTypes[current] != RETURN) {
    current++;
  }
  return current;
}

void visitCall(DDSimulationState* ddsim, size_t current, size_t qubitIndex,
               std::set<size_t>& visited, std::set<size_t>& toVisit) {
  const auto gateStart = ddsim->successorInstructions[current];
  const auto gateDefinition = gateStart - 1;
  const std::string stringToSearch =
      ddsim->targetQubits[gateDefinition][qubitIndex];
  auto checkInstruction = findReturn(ddsim, gateStart);
  while (checkInstruction >= gateStart) {
    const auto found =
        std::find(ddsim->targetQubits[checkInstruction].begin(),
                  ddsim->targetQubits[checkInstruction].end(), stringToSearch);
    if (ddsim->instructionTypes[checkInstruction] != RETURN &&
        found != ddsim->targetQubits[checkInstruction].end()) {
      if (visited.find(checkInstruction) == visited.end()) {
        toVisit.insert(checkInstruction);
      }
      if (ddsim->instructionTypes[checkInstruction] == CALL) {
        const auto position =
            std::distance(ddsim->targetQubits[checkInstruction].begin(), found);
        visitCall(ddsim, checkInstruction, static_cast<size_t>(position),
                  visited, toVisit);
      }
      break;
    }
    if (checkInstruction == 0) {
      break;
    }
    checkInstruction--;
  }
}

std::set<size_t> getUnknownCallers(DDSimulationState* ddsim,
                                   size_t instruction) {
  std::set<size_t> unknownCallers;

  std::set<size_t> toVisit{};
  std::set<size_t> visited{};

  while (true) {
    instruction--;
    if (ddsim->functionDefinitions.find(instruction) !=
        ddsim->functionDefinitions.end()) {
      unknownCallers.insert(instruction);
      for (const auto caller : ddsim->functionCallers[instruction]) {
        if (visited.find(caller) == visited.end()) {
          toVisit.insert(caller);
        }
      }
    }

    if (instruction == 0 || ddsim->instructionTypes[instruction] == RETURN ||
        ddsim->functionDefinitions.find(instruction) !=
            ddsim->functionDefinitions.end()) {
      if (toVisit.empty()) {
        break;
      }

      instruction = *toVisit.begin();
      toVisit.erase(instruction);
      visited.insert(instruction);
    }
  }

  return unknownCallers;
}

Result dddiagnosticsGetDataDependencies(Diagnostics* self, size_t instruction,
                                        bool includeCallers,
                                        bool* instructions) {
  auto* ddd = toDDDiagnostics(self);
  auto* ddsim = ddd->simulationState;
  const Span<bool> isDependency(
      instructions, ddsim->interface.getInstructionCount(&ddsim->interface));
  std::set<size_t> toVisit{instruction};
  std::set<size_t> visited;

  // Stores all functions whose callers are unknown (because analysis started
  // inside them)
  const std::set<size_t> unknownCallers =
      includeCallers ? getUnknownCallers(ddsim, instruction)
                     : std::set<size_t>{};

  while (!toVisit.empty()) {
    auto current = *toVisit.begin();
    isDependency[current] = true;
    toVisit.erase(toVisit.begin());
    visited.insert(current);

    for (auto dep : ddsim->dataDependencies[current]) {
      const auto depInstruction = dep.first;
      if (ddsim->instructionTypes[depInstruction] == NOP) {
        continue; // We don't want variable declarations as dependencies.
      }
      if (visited.find(depInstruction) == visited.end()) {
        toVisit.insert(depInstruction);
      }
      if (ddsim->instructionTypes[depInstruction] == CALL) {
        visitCall(ddsim, depInstruction, dep.second, visited, toVisit);
      }
    }

    if (unknownCallers.find(current - 1) != unknownCallers.end()) {
      for (auto caller : ddsim->functionCallers[current - 1]) {
        if (visited.find(caller) == visited.end()) {
          toVisit.insert(caller);
        }
      }
    }
  }

  return OK;
}

Result dddiagnosticsGetInteractions(Diagnostics* self, size_t beforeInstruction,
                                    size_t qubit, bool* qubitsAreInteracting) {
  auto* ddd = toDDDiagnostics(self);
  auto* ddsim = ddd->simulationState;
  std::set<size_t> interactions;
  interactions.insert(qubit);
  bool found = true;

  while (found) {
    found = false;
    for (auto i = beforeInstruction - 1; i < beforeInstruction; i--) {
      if (std::find(ddsim->functionDefinitions.begin(),
                    ddsim->functionDefinitions.end(),
                    i) != ddsim->functionDefinitions.end()) {
        break;
      }
      if (ddsim->instructionTypes[i] != SIMULATE &&
          ddsim->instructionTypes[i] != CALL) {
        continue;
      }

      auto targets = getTargetVariables(ddsim, i);
      std::set<size_t> targetQubits;
      for (const auto& target : targets) {
        targetQubits.insert(variableToQubitAt(ddsim, target, i).first);
      }
      if (!std::none_of(targetQubits.begin(), targetQubits.end(),
                        [&interactions](size_t elem) {
                          return interactions.find(elem) != interactions.end();
                        })) {
        for (const auto& target : targetQubits) {
          if (interactions.find(target) == interactions.end()) {
            found = true;
          }
          interactions.insert(target);
        }
      }
    }
  }

  const auto qubits = Span<bool>(
      qubitsAreInteracting, ddsim->interface.getNumQubits(&ddsim->interface));
  for (auto interaction : interactions) {
    qubits[interaction] = true;
  }

  return OK;
}

size_t dddiagnosticsPotentialErrorCauses(Diagnostics* self, ErrorCause* output,
                                         size_t count) {
  auto* ddd = toDDDiagnostics(self);
  auto* ddsim = ddd->simulationState;
  auto outputs = Span(output, count);

  const size_t assertion = ddsim->lastFailedAssertion;
  if (assertion == -1ULL) {
    return 0;
  }
  const auto& assertionInstruction = ddsim->assertionInstructions[assertion];
  size_t index = 0;

  if (assertionInstruction->getType() == AssertionType::Entanglement) {
    index +=
        tryFindMissingInteraction(ddd, ddsim, assertion, assertionInstruction,
                                  &outputs[index], count - index);
  }
  index += tryFindZeroControls(ddd, assertion, &outputs[index], count - index);

  return index;
}

std::set<size_t> getInteractionsAtRuntime(DDDiagnostics* ddd, size_t qubit) {
  auto* ddsim = ddd->simulationState;
  std::set<size_t> interactions;
  interactions.insert(qubit);
  bool found = true;

  while (found) {
    found = false;

    for (size_t i = 0; i < ddsim->instructionTypes.size(); i++) {
      if (ddsim->instructionTypes[i] != SIMULATE) {
        continue;
      }
      if (ddd->actualQubits.find(i) == ddd->actualQubits.end()) {
        continue;
      }

      auto& actualQubits = ddd->actualQubits[i];
      for (const auto& actualQubitVector : actualQubits) {
        if (!std::none_of(actualQubitVector.begin(), actualQubitVector.end(),
                          [&interactions](size_t elem) {
                            return interactions.find(elem) !=
                                   interactions.end();
                          })) {
          for (const auto& target : actualQubitVector) {
            if (interactions.find(target) == interactions.end()) {
              found = true;
            }
            interactions.insert(target);
          }
        }
      }
    }
  }

  return interactions;
}

size_t tryFindMissingInteraction(DDDiagnostics* diagnostics,
                                 DDSimulationState* state, size_t instruction,
                                 const std::unique_ptr<Assertion>& assertion,
                                 ErrorCause* output, size_t count) {
  if (count == 0) {
    return 0;
  }

  auto targets = assertion->getTargetQubits();
  auto outputs = Span(output, count);
  std::vector<size_t> targetQubits(targets.size());
  size_t index = 0;

  std::transform(targets.begin(), targets.end(), targetQubits.begin(),
                 [&state](const std::string& target) {
                   return variableToQubit(state, target);
                 });

  std::map<size_t, std::set<size_t>> allInteractions;

  for (size_t i = 0; i < targets.size(); i++) {
    allInteractions.insert(
        {targetQubits[i],
         getInteractionsAtRuntime(diagnostics, targetQubits[i])});
  }

  for (size_t i = 0; i < targets.size(); i++) {
    for (size_t j = i + 1; j < targets.size(); j++) {
      if (allInteractions[targetQubits[i]].find(targetQubits[j]) ==
          allInteractions[targetQubits[i]].end()) {
        outputs[index].type = ErrorCauseType::MissingInteraction;
        outputs[index].instruction = instruction;
        index++;
      }
      if (index == count) {
        return index;
      }
    }
  }

  return index;
}

size_t tryFindZeroControls(DDDiagnostics* diagnostics, size_t instruction,
                           ErrorCause* output, size_t count) {
  if (count == 0) {
    return 0;
  }

  std::vector<uint8_t> dependencies(
      diagnostics->interface.getInstructionCount(&diagnostics->interface));
  diagnostics->interface.getDataDependencies(&diagnostics->interface,
                                             instruction, true,
                                             toBoolArray(dependencies.data()));
  auto outputs = Span(output, count);
  size_t index = 0;

  for (size_t i = 0; i < dependencies.size(); i++) {
    if (dependencies[i] == 0) {
      continue;
    }
    if (diagnostics->zeroControls.find(i) == diagnostics->zeroControls.end()) {
      continue;
    }
    if (diagnostics->nonZeroControls.find(i) !=
        diagnostics->nonZeroControls.end()) {
      continue;
    }
    const auto& zeroControls = diagnostics->zeroControls[i];
    if (!zeroControls.empty()) {
      outputs[index].type = ErrorCauseType::ControlAlwaysZero;
      outputs[index].instruction = i;
      index++;
      if (index == count) {
        return index;
      }
    }
  }

  return index;
}

bool isAlwaysZero(const Statevector& sv, size_t qubit, bool checkOne = false) {
  const auto epsilon = 1e-10;

  const Span<Complex> amplitudes(sv.amplitudes, sv.numStates);

  for (size_t i = 0; i < sv.numStates; i++) {
    if (((i & (1ULL << qubit)) == 0ULL && !checkOne) ||
        ((i & (1ULL << qubit)) != 0ULL && checkOne)) {
      continue;
    }
    if (amplitudes[i].real > epsilon || amplitudes[i].real < -epsilon ||
        amplitudes[i].imaginary > epsilon ||
        amplitudes[i].imaginary < -epsilon) {
      return false;
    }
  }

  return true;
}

Result dddiagnosticsGetZeroControlInstructions(Diagnostics* self,
                                               bool* instructions) {
  auto* ddd = toDDDiagnostics(self);
  const Span<bool> instructionSpan(instructions,
                                   dddiagnosticsGetInstructionCount(self));
  for (size_t i = 0; i < dddiagnosticsGetInstructionCount(self); i++) {
    instructionSpan[i] =
        (ddd->nonZeroControls.find(i) == ddd->nonZeroControls.end()) &&
        (ddd->zeroControls.find(i) != ddd->zeroControls.end());
  }

  return OK;
}

void dddiagnosticsOnStepForward(DDDiagnostics* diagnostics,
                                size_t instruction) {
  auto* ddsim = diagnostics->simulationState;
  const auto targets = getTargetVariables(ddsim, instruction);

  // Add actual qubits to tracker.
  if (ddsim->instructionTypes[instruction] == SIMULATE ||
      ddsim->instructionTypes[instruction] == CALL) {
    std::vector<size_t> targetQubits(targets.size());
    std::transform(targets.begin(), targets.end(), targetQubits.begin(),
                   [&ddsim](const std::string& target) {
                     return variableToQubit(ddsim, target);
                   });
    if (diagnostics->actualQubits.find(instruction) ==
        diagnostics->actualQubits.end()) {
      diagnostics->actualQubits[instruction] = std::set<std::vector<size_t>>();
    }
    diagnostics->actualQubits[instruction].insert(targetQubits);
  }

  // Check for zero controls.
  if (ddsim->instructionTypes[instruction] != SIMULATE) {
    return;
  }
  const auto numQubits =
      diagnostics->interface.getNumQubits(&diagnostics->interface);
  if (numQubits > 16) {
    return;
  }
  const auto& op = (*ddsim->iterator);
  const auto& controls = op->getControls();

  std::vector<Complex> amplitudes(2ULL << numQubits);
  Statevector sv{numQubits, 2ULL << numQubits, amplitudes.data()};
  ddsim->interface.getStateVectorFull(&ddsim->interface, &sv);

  for (const auto& control : controls) {
    const auto pos = control.type == qc::Control::Type::Pos;
    const auto qubit = control.qubit;
    if (isAlwaysZero(sv, qubit, !pos)) {
      if (diagnostics->zeroControls.find(instruction) ==
          diagnostics->zeroControls.end()) {
        diagnostics->zeroControls[instruction] = std::set<size_t>();
      }
      diagnostics->zeroControls[instruction].insert(qubit);
    } else {
      if (diagnostics->nonZeroControls.find(instruction) ==
          diagnostics->nonZeroControls.end()) {
        diagnostics->nonZeroControls[instruction] = std::set<size_t>();
      }
      diagnostics->nonZeroControls[instruction].insert(qubit);
    }
  }
}
