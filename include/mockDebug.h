#include "debug.h"

typedef struct
{
  SimulationState interface;
  int currentLine;

  Statevector stateVector;
  char assertionFailed;

  char useCorrectAction3;
} MockSimulationState;

Result mockInit(SimulationState* self);
Result mockStepForward(SimulationState* self);
Result mockStepBackward(SimulationState* self);
Result mockRunSimulation(SimulationState* self);
Result mockResetSimulation(SimulationState* self);
char mockCanStepForward(SimulationState* self);
char mockCanStepBackward(SimulationState* self);
char mockIsFinished(SimulationState* self);

int mockGetCurrentLine(SimulationState* self);
Result mockGetAmplitudeIndex(SimulationState* self, int qubit, Complex* output);
Result mockGetAmplitudeBitstring(SimulationState* self, char* bitstring, Complex* output);
Result mockGetClassicalVariable(SimulationState* self, char* name, Variable* output);
Result mockGetStateVectorFull(SimulationState* self, Statevector* output);
Result mockGetStateVectorSub(SimulationState* self, int substateSize, int* qubits, Statevector* output);

Result createMockSimulationState(MockSimulationState* self);
Result destroyMockSimulationState(MockSimulationState* self);