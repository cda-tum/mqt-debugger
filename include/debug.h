#ifndef DEBUG_H
#define DEBUG_H

#include "common.h"

typedef struct SimulationState SimulationState;

struct SimulationState {
    Result (*init)(SimulationState* self);
    Result (*stepForward)(SimulationState* self);
    Result (*stepBackward)(SimulationState* self);
    Result (*runSimulation)(SimulationState* self);
    Result (*resetSimulation)(SimulationState* self);
    bool (*canStepForward)(SimulationState* self);
    bool (*canStepBackward)(SimulationState* self);
    bool (*isFinished)(SimulationState* self);

    int (*getCurrentLine)(SimulationState* self);
    Result (*getAmplitudeIndex)(SimulationState* self, int qubit, Complex* output);
    Result (*getAmplitudeBitstring)(SimulationState* self, char* bitstring, Complex* output);
    Result (*getClassicalVariable)(SimulationState* self, char* name, Variable* output);
    Result (*getStateVectorFull)(SimulationState* self, Statevector* output);
    Result (*getStateVectorSub)(SimulationState* self, int substateSize, int* qubits, Statevector* output);
};

#endif // DEBUG_H