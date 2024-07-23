#pragma once

#include "backend/debug.h"
#include "backend/diagnostics.h"
#include "common/parsing/AssertionParsing.hpp"

#include <map>
#include <set>

struct DDSimulationState;

struct DDDiagnostics {
  Diagnostics interface;

  DDSimulationState* simulationState;
  std::map<size_t, std::set<size_t>> zeroControls;
};

size_t dddiagnosticsGetNumQubits(Diagnostics* self);
size_t dddiagnosticsGetInstructionCount(Diagnostics* self);
Result dddiagnosticsInit([[maybe_unused]] Diagnostics* self);
Result dddiagnosticsGetDataDependencies(Diagnostics* self, size_t instruction,
                                        bool* instructions);
Result dddiagnosticsGetInteractions(Diagnostics* self, size_t beforeInstruction,
                                    size_t qubit, bool* qubitsAreInteracting);
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
