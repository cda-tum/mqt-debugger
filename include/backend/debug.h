#pragma once

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SimulationState SimulationState;

struct SimulationState {
  Result (*init)(SimulationState* self);

  Result (*loadCode)(SimulationState* self, const char* code);

  Result (*stepForward)(SimulationState* self);
  Result (*stepOverForward)(SimulationState* self);
  Result (*stepBackward)(SimulationState* self);
  Result (*stepOverBackward)(SimulationState* self);
  Result (*runSimulation)(SimulationState* self);
  Result (*resetSimulation)(SimulationState* self);
  bool (*canStepForward)(SimulationState* self);
  bool (*canStepBackward)(SimulationState* self);
  bool (*isFinished)(SimulationState* self);
  bool (*didAssertionFail)(SimulationState* self);

  size_t (*getCurrentInstruction)(SimulationState* self);
  size_t (*getPreviousInstruction)(SimulationState* self);
  size_t (*getInstructionCount)(SimulationState* self);
  Result (*getInstructionPosition)(SimulationState* self, size_t instruction,
                                   size_t* start, size_t* end);
  size_t (*getNumQubits)(SimulationState* self);
  Result (*getAmplitudeIndex)(SimulationState* self, size_t qubit,
                              Complex* output);
  Result (*getAmplitudeBitstring)(SimulationState* self, const char* bitstring,
                                  Complex* output);
  Result (*getClassicalVariable)(SimulationState* self, const char* name,
                                 Variable* output);
  Result (*getStateVectorFull)(SimulationState* self, Statevector* output);
  Result (*getStateVectorSub)(SimulationState* self, size_t subStateSize,
                              const size_t* qubits, Statevector* output);
  Result (*getDataDependencies)(SimulationState* self, size_t instruction,
                                bool* instructions);
};

#ifdef __cplusplus
}
#endif
