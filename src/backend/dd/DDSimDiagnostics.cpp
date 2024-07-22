#pragma ide diagnostic ignored "cppcoreguidelines-pro-type-reinterpret-cast"

#include "backend/dd/DDSimDiagnostics.hpp"

#include "backend/dd/DDSimDebug.hpp"

#include <span>

Result createDDDiagnostics(DDDiagnostics* self, DDSimulationState* state) {
  self->simulationState = state;

  self->interface.init = dddiagnosticsInit;
  self->interface.getNumQubits = dddiagnosticsGetNumQubits;
  self->interface.getInteractions = dddiagnosticsGetInteractions;
  self->interface.getDataDependencies = dddiagnosticsGetDataDependencies;

  return self->interface.init(&self->interface);
}

Result destroyDDDiagnostics([[maybe_unused]] DDDiagnostics* self) { return OK; }

size_t dddiagnosticsGetNumQubits(Diagnostics* self) {
  const auto* ddd = reinterpret_cast<DDDiagnostics*>(self);
  return ddd->simulationState->interface.getNumQubits(
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
  std::vector<size_t> interactions;
  for (auto i = 0ULL; i < ddsim->instructionTypes.size(); i++) {
    if (i == beforeInstruction) {
      break;
    }
    if (ddsim->instructionTypes[i] != SIMULATE) {
      continue;
    }
    auto& targets = ddsim->targetQubits[i];
    std::set<size_t> targetQubits;
    for (auto target : targets) {
      targetQubits.insert(variableToQubit(ddsim, target));
    }
    if (targetQubits.find(qubit) != targetQubits.end()) {
      for (const auto& target : targetQubits) {
        interactions.push_back(target);
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
