#include "backend/dd/DDSimDiagnostics.hpp"

#include "backend/dd/DDSimDebug.hpp"
#include "backend/diagnostics.h"
#include "common.h"
#include "common/Span.hpp"
#include "common/parsing/AssertionParsing.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
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

Result dddiagnosticsInit([[maybe_unused]] Diagnostics* self) { return OK; }

Result dddiagnosticsGetDataDependencies(Diagnostics* self, size_t instruction,
                                        bool* instructions) {
  auto* ddd = toDDDiagnostics(self);
  auto* ddsim = ddd->simulationState;
  const Span<bool> isDependency(
      instructions, ddsim->interface.getInstructionCount(&ddsim->interface));
  std::set<size_t> toVisit{instruction};
  std::set<size_t> visited;
  while (!toVisit.empty()) {
    auto current = *toVisit.begin();
    isDependency[current] = true;
    toVisit.erase(toVisit.begin());
    visited.insert(current);
    for (auto dep : ddsim->dataDependencies[current]) {
      if (visited.find(dep) == visited.end()) {
        toVisit.insert(dep);
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
      if (ddsim->instructionTypes[i] != SIMULATE &&
          ddsim->instructionTypes[i] != CALL) {
        continue;
      }
      auto& targets = ddsim->targetQubits[i];
      std::set<size_t> targetQubits;
      for (const auto& target : targets) {
        targetQubits.insert(variableToQubit(ddsim, target));
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

size_t tryFindMissingInteraction(DDDiagnostics* diagnostics,
                                 DDSimulationState* state, size_t instruction,
                                 const std::unique_ptr<Assertion>& assertion,
                                 ErrorCause* output, size_t count) {
  auto targets = assertion->getTargetQubits();
  auto outputs = Span(output, count);
  std::vector<size_t> targetQubits(targets.size());
  size_t index = 0;

  std::transform(targets.begin(), targets.end(), targetQubits.begin(),
                 [&state](const std::string& target) {
                   return variableToQubit(state, target);
                 });

  std::map<size_t, std::vector<uint8_t>> allInteractions;

  for (size_t i = 0; i < targets.size(); i++) {
    std::vector<uint8_t> interactions(
        diagnostics->interface.getNumQubits(&diagnostics->interface));
    diagnostics->interface.getInteractions(&diagnostics->interface, instruction,
                                           targetQubits[i],
                                           toBoolArray(interactions.data()));
    allInteractions.insert({targetQubits[i], interactions});
  }
  for (size_t i = 0; i < targets.size(); i++) {
    for (size_t j = i + 1; j < targets.size(); j++) {
      if (allInteractions[targetQubits[i]][targetQubits[j]] == 0 &&
          allInteractions[targetQubits[j]][targetQubits[i]] == 0) {
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
  std::vector<uint8_t> dependencies(
      diagnostics->interface.getInstructionCount(&diagnostics->interface));
  diagnostics->interface.getDataDependencies(
      &diagnostics->interface, instruction, toBoolArray(dependencies.data()));
  auto outputs = Span(output, count);
  size_t index = 0;

  for (size_t i = 0; i < dependencies.size(); i++) {
    if (dependencies[i] == 0) {
      continue;
    }
    if (diagnostics->zeroControls.find(i) == diagnostics->zeroControls.end()) {
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

void dddiagnosticsOnStepForward(DDDiagnostics* diagnostics,
                                size_t instruction) {
  auto* ddsim = diagnostics->simulationState;
  if (ddsim->instructionTypes[instruction] != SIMULATE) {
    return;
  }
  const auto numQubits = ddsim->interface.getNumQubits(&ddsim->interface);
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
    }
  }
}
