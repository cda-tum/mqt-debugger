#pragma once

#include "backend/debug.h"
#include "backend/diagnostics.h"

struct DDSimulationState;

struct DDDiagnostics {
  Diagnostics interface;

  DDSimulationState* simulationState;
};

size_t dddiagnosticsGetNumQubits(Diagnostics* self);
Result dddiagnosticsInit([[maybe_unused]] Diagnostics* self);
Result dddiagnosticsGetDataDependencies(Diagnostics* self, size_t instruction,
                                        bool* instructions);
Result dddiagnosticsGetInteractions(Diagnostics* self, size_t beforeInstruction,
                                    size_t qubit, bool* qubitsAreInteracting);

Result createDDDiagnostics(DDDiagnostics* self, DDSimulationState* state);
Result destroyDDDiagnostics([[maybe_unused]] DDDiagnostics* self);
