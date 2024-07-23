#pragma ide diagnostic ignored "cppcoreguidelines-pro-type-reinterpret-cast"

#include "backend/dd/DDSimDiagnostics.hpp"

#include "backend/dd/DDSimDebug.hpp"

#include <span>

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

Result destroyDDDiagnostics([[maybe_unused]] DDDiagnostics* self) { return OK; }

size_t dddiagnosticsGetNumQubits(Diagnostics* self) {
  const auto* ddd = reinterpret_cast<DDDiagnostics*>(self);
  return ddd->simulationState->interface.getNumQubits(
      &ddd->simulationState->interface);
}

size_t dddiagnosticsGetInstructionCount(Diagnostics* self) {
  const auto* ddd = reinterpret_cast<DDDiagnostics*>(self);
  return ddd->simulationState->interface.getInstructionCount(
      &ddd->simulationState->interface);
}

Result dddiagnosticsInit([[maybe_unused]] Diagnostics* self) { return OK; }

Result dddiagnosticsGetDataDependencies(Diagnostics* self, size_t instruction,
                                        bool* instructions) {
  auto* ddd = reinterpret_cast<DDDiagnostics*>(self);
  auto* ddsim = ddd->simulationState;
  const std::span<bool> isDependency(
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
  auto* ddd = reinterpret_cast<DDDiagnostics*>(self);
  auto* ddsim = ddd->simulationState;
  std::set<size_t> interactions;
  interactions.insert(qubit);
  for (auto i = beforeInstruction - 1; i < beforeInstruction; i--) {
    if (ddsim->instructionTypes[i] != SIMULATE &&
        ddsim->instructionTypes[i] != CALL) {
      continue;
    }
    auto& targets = ddsim->targetQubits[i];
    std::set<size_t> targetQubits;
    for (auto target : targets) {
      targetQubits.insert(variableToQubit(ddsim, target));
    }
    if (!std::none_of(targetQubits.begin(), targetQubits.end(),
                      [&interactions](size_t elem) {
                        return interactions.find(elem) != interactions.end();
                      })) {
      for (const auto& target : targetQubits) {
        interactions.insert(target);
      }
    }
  }

  const auto qubits = std::span<bool>(
      qubitsAreInteracting, ddsim->interface.getNumQubits(&ddsim->interface));
  for (auto interaction : interactions) {
    qubits[interaction] = true;
  }

  return OK;
}

size_t dddiagnosticsPotentialErrorCauses(Diagnostics* self, ErrorCause* output,
                                         size_t count) {
  auto* ddd = reinterpret_cast<DDDiagnostics*>(self);
  auto* ddsim = ddd->simulationState;
  auto outputs = std::span(output, count);

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
  auto outputs = std::span(output, count);
  std::vector<size_t> targetQubits(targets.size());
  size_t index = 0;

  std::transform(targets.begin(), targets.end(), targetQubits.begin(),
                 [&state](const std::string& target) {
                   return variableToQubit(state, target);
                 });

  for (size_t i = 0; i < targets.size(); i++) {
    std::vector<uint8_t> interactions(
        diagnostics->interface.getNumQubits(&diagnostics->interface));
    diagnostics->interface.getInteractions(
        &diagnostics->interface, instruction, targetQubits[i],
        reinterpret_cast<bool*>(interactions.data()));
    for (size_t j = i + 1; j < targets.size(); j++) {
      if (interactions[targetQubits[j]] == 0) {
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
      &diagnostics->interface, instruction,
      reinterpret_cast<bool*>(dependencies.data()));
  auto outputs = std::span(output, count);
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

void dddiagnosticsOnStepForward(DDDiagnostics* diagnostics,
                                size_t instruction) {
  auto* ddsim = diagnostics->simulationState;
  if (ddsim->instructionTypes[instruction] != SIMULATE) {
    return;
  }
  const auto& op = (*ddsim->iterator);
  const auto& controls = op->getControls();

  std::vector<Complex> amplitudes(2);
  Statevector sv{1, 2, amplitudes.data()};
  std::vector<size_t> qubits{0};
  const double epsilon = 1e-10;

  for (const auto& control : controls) {
    const auto pos = control.type == qc::Control::Type::Pos;
    const auto qubit = control.qubit;
    qubits[0] = qubit;
    if (ddsim->interface.getStateVectorSub(&ddsim->interface, 1, qubits.data(),
                                           &sv) == OK) {
      const auto amplitude = amplitudes[pos ? 1 : 0];
      if (amplitude.real < epsilon && amplitude.real > -epsilon &&
          amplitude.imaginary < epsilon && amplitude.imaginary > -epsilon) {
        if (diagnostics->zeroControls.find(instruction) ==
            diagnostics->zeroControls.end()) {
          diagnostics->zeroControls[instruction] = std::set<size_t>();
        }
        diagnostics->zeroControls[instruction].insert(qubit);
      }
    }
  }
}