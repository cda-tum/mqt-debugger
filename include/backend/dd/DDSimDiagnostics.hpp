#pragma once

#include "backend/diagnostics.h"
#include "common.h"
#include "common/parsing/AssertionParsing.hpp"

#include <cstddef>
#include <map>
#include <memory>
#include <set>
#include <vector>

struct DDSimulationState;

struct DDDiagnostics {
  Diagnostics interface;

  DDSimulationState* simulationState;
  std::map<size_t, std::set<size_t>> zeroControls;
  std::map<size_t, std::set<size_t>> nonZeroControls;

  std::map<size_t, std::set<std::vector<size_t>>> actualQubits;
};

size_t dddiagnosticsGetNumQubits(Diagnostics* self);
size_t dddiagnosticsGetInstructionCount(Diagnostics* self);
Result dddiagnosticsInit(Diagnostics* self);
Result dddiagnosticsGetDataDependencies(Diagnostics* self, size_t instruction,
                                        bool includeCallers,
                                        bool* instructions);
Result dddiagnosticsGetInteractions(Diagnostics* self, size_t beforeInstruction,
                                    size_t qubit, bool* qubitsAreInteracting);
Result dddiagnosticsGetZeroControlInstructions(Diagnostics* self,
                                               bool* instructions);
size_t dddiagnosticsPotentialErrorCauses(Diagnostics* self, ErrorCause* output,
                                         size_t count);

Result createDDDiagnostics(DDDiagnostics* self, DDSimulationState* state);
Result destroyDDDiagnostics([[maybe_unused]] DDDiagnostics* self);

void dddiagnosticsOnStepForward(DDDiagnostics* diagnostics, size_t instruction);
size_t tryFindMissingInteraction(DDDiagnostics* diagnostics,
                                 DDSimulationState* state, size_t instruction,
                                 const std::unique_ptr<Assertion>& assertion,
                                 ErrorCause* output, size_t count);
size_t tryFindZeroControls(DDDiagnostics* diagnostics, size_t instruction,
                           ErrorCause* output, size_t count);
