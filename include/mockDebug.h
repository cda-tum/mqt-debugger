#include "debug.h"

typedef struct
{
  SimulationState interface;
  int currentLine;

  Statevector stateVector;
  bool assertionFailed;

  // Determines, whether the correct or incorrect action 3 is used. Can be used to mock errors in the code and to fix them during debugging.
  bool useCorrectAction3;
} MockSimulationState;

Result mockInit(SimulationState* self);

Result mockLoadCode(SimulationState* self, char* code);
Result mockStepForward(SimulationState* self);
Result mockStepBackward(SimulationState* self);
Result mockRunSimulation(SimulationState* self);
Result mockResetSimulation(SimulationState* self);
bool mockCanStepForward(SimulationState* self);
bool mockCanStepBackward(SimulationState* self);
bool mockIsFinished(SimulationState* self);

int mockGetCurrentLine(SimulationState* self);
Result mockGetAmplitudeIndex(SimulationState* self, int qubit, Complex* output);
Result mockGetAmplitudeBitstring(SimulationState* self, char* bitstring, Complex* output);
Result mockGetClassicalVariable(SimulationState* self, char* name, Variable* output);
Result mockGetStateVectorFull(SimulationState* self, Statevector* output);
Result mockGetStateVectorSub(SimulationState* self, int substateSize, int* qubits, Statevector* output);

Result createMockSimulationState(MockSimulationState* self);
Result destroyMockSimulationState(MockSimulationState* self);