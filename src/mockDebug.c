#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "mockDebug.h"

// --------------------- Mock example ----------------------------

int N_ACTIONS = 5;
void doAction1(MockSimulationState* state) {
    // h q[0];
    state->stateVector.amplitudes[0].real = 0.7071;
    state->stateVector.amplitudes[1].real = 0.7071;
}

void doAction2(MockSimulationState* state) {
    // cx q[0], q[1];
    state->stateVector.amplitudes[1].real = 0.0;
    state->stateVector.amplitudes[3].real = 0.7071;
}

void doAction3Correct(MockSimulationState* state) {
    // cx q[0], q[2];
    state->stateVector.amplitudes[3].real = 0.0;
    state->stateVector.amplitudes[7].real = 0.7071;
}

void doAction3(MockSimulationState* state) {
    // cx q[2], q[0];
}

void doAction4(MockSimulationState* state) {
    // assert-ent q[0], q[1];
    Complex* sv = state->stateVector.amplitudes;
    float e = 0.00001;
    if(!(
            (sv[0].real > e && sv[7].real > e) || 
            ((sv[0].real > e && sv[3].real > e)) ||
            ((sv[4].real > e && sv[7].real > e)) ||
            ((sv[4].real > e && sv[3].real > e)) ||
            ((sv[1].real > e && sv[2].real > e)) ||
            ((sv[1].real > e && sv[6].real > e)) ||
            ((sv[5].real > e && sv[2].real > e)) ||
            ((sv[5].real > e && sv[6].real > e))
        ))
        state->assertionFailed = 1;
}

void doAction5(MockSimulationState* state) {
    // assert-ent q[0], q[2];
    Complex* sv = state->stateVector.amplitudes;
    float e = 0.00001;
    if(!(
            (sv[0].real > e && sv[7].real > e) || 
            ((sv[0].real > e && sv[5].real > e)) ||
            ((sv[2].real > e && sv[7].real > e)) ||
            ((sv[2].real > e && sv[5].real > e)) ||
            ((sv[1].real > e && sv[4].real > e)) ||
            ((sv[1].real > e && sv[6].real > e)) ||
            ((sv[3].real > e && sv[4].real > e)) ||
            ((sv[3].real > e && sv[6].real > e))
        ))
        state->assertionFailed = 1;
}

void undoAction1(MockSimulationState* state) {
    // h q[0];
    state->stateVector.amplitudes[0].real = 0.1;
    state->stateVector.amplitudes[1].real = 0.0;
}

void undoAction2(MockSimulationState* state) {
    // cx q[0], q[1];
    state->stateVector.amplitudes[1].real = 0.7071;
    state->stateVector.amplitudes[3].real = 0.0;
}

void undoAction3Correct(MockSimulationState* state) {
    // cx q[0], q[2];
    state->stateVector.amplitudes[3].real = 0.7071;
    state->stateVector.amplitudes[7].real = 0.0;
}

void undoAction3(MockSimulationState* state) {
    // cx q[2], q[0];
}

void undoAction4(MockSimulationState* state) {
    // assert-ent q[0], q[1];
    state->assertionFailed = 0;
}

void undoAction5(MockSimulationState* state) {
    // assert-ent q[0], q[2];
    state->assertionFailed = 0;
}

// --------------------- Mock example end -------------------------


Result createMockSimulationState(MockSimulationState* self) {
    self->interface.init = mockInit;
    self->interface.stepForward = mockStepForward;
    self->interface.stepBackward = mockStepBackward;
    self->interface.runSimulation = mockRunSimulation;
    self->interface.resetSimulation = mockResetSimulation;
    self->interface.canStepForward = mockCanStepForward;
    self->interface.canStepBackward = mockCanStepBackward;
    self->interface.isFinished = mockIsFinished;

    self->interface.getCurrentLine = mockGetCurrentLine;
    self->interface.getAmplitudeIndex = mockGetAmplitudeIndex;
    self->interface.getAmplitudeBitstring = mockGetAmplitudeBitstring;
    self->interface.getClassicalVariable = mockGetClassicalVariable;
    self->interface.getStateVectorFull = mockGetStateVectorFull;
    self->interface.getStateVectorSub = mockGetStateVectorSub;

    return self->interface.init((SimulationState*) self);
}

Result mockInit(SimulationState* self) {
    MockSimulationState* mock = (MockSimulationState*)self;
    mock->currentLine = 0;

    mock->stateVector.numQubits = 3;
    mock->stateVector.numStates = 8;
    mock->stateVector.amplitudes = (Complex*)malloc(sizeof(Complex) * mock->stateVector.numStates);
    for(int i = 0; i < mock->stateVector.numStates; i++) {
        mock->stateVector.amplitudes[i].real = i == 0 ? 1.0 : 0.0;
        mock->stateVector.amplitudes[i].imaginary = 0;
    }

    mock->assertionFailed = 0;
    mock->useCorrectAction3 = 0;

    return OK;
}

Result mockStepForward(SimulationState* self) {
    MockSimulationState* mock = (MockSimulationState*)self;
    if(!self->canStepForward(self))
        return ERROR;
    mock->currentLine++;
    switch(mock->currentLine) {
        case 1:
            doAction1(mock);
            break;
        case 2:
            doAction2(mock);
            break;
        case 3:
            mock->useCorrectAction3 ? doAction3Correct(mock) : doAction3(mock);
            break;
        case 4:
            doAction4(mock);
            break;
        case 5:
            doAction5(mock);
            break;
    }
    return OK;
}

Result mockStepBackward(SimulationState* self) {
    MockSimulationState* mock = (MockSimulationState*)self;
    if(!self->canStepBackward(self))
        return ERROR;
    mock->currentLine--;
    switch(mock->currentLine) {
        case 0:
            undoAction1(mock);
            break;
        case 1:
            undoAction2(mock);
            break;
        case 2:
            mock->useCorrectAction3 ? undoAction3Correct(mock) : undoAction3(mock);
            break;
        case 3:
            undoAction4(mock);
            break;
        case 4:
            undoAction5(mock);
            break;
    }
    return OK;
}

Result mockRunSimulation(SimulationState* self) {
    MockSimulationState* mock = (MockSimulationState*)self;
    while(!self->isFinished(self) && !mock->assertionFailed)
        self->stepForward(self);
    return OK;
}

Result mockResetSimulation(SimulationState* self) {
    MockSimulationState* mock = (MockSimulationState*)self;
    mock->currentLine = 0;
    return OK;
}

char mockCanStepForward(SimulationState* self) {
    MockSimulationState* mock = (MockSimulationState*)self;
    return mock->currentLine < N_ACTIONS;
}

char mockCanStepBackward(SimulationState* self) {
    MockSimulationState* mock = (MockSimulationState*)self;
    return mock->currentLine > 0;
}

char mockIsFinished(SimulationState* self) {
    MockSimulationState* mock = (MockSimulationState*)self;
    return mock->currentLine >= N_ACTIONS; // TODO
}

int mockGetCurrentLine(SimulationState* self) {
    MockSimulationState* mock = (MockSimulationState*)self;
    return mock->currentLine;
}

Result mockGetAmplitudeIndex(SimulationState* self, int qubit, Complex* output) {
    MockSimulationState* mock = (MockSimulationState*)self;
    *output = mock->stateVector.amplitudes[qubit];
    return OK;
}

Result mockGetAmplitudeBitstring(SimulationState* self, char* bitstring, Complex* output) {
    MockSimulationState* mock = (MockSimulationState*)self;
    char* bitstrings[] = {"000", "001", "010", "011", "100", "101", "110", "111"};
    for(int i = 0; i < 8; i++) {
        if(strcmp(bitstrings[i], bitstring) == 0) {
            return self->getAmplitudeIndex(self, i, output); 
        }
    }
    return ERROR;
}

Result mockGetClassicalVariable(SimulationState* self, char* name, Variable* output) {
    Variable v;
    v.name = "mock";
    v.type = VAR_INT;
    v.value.int_value = self->getCurrentLine(self);
    *output = v;
    return OK;
}

Result mockGetStateVectorFull(SimulationState* self, Statevector* output) {
    *output = ((MockSimulationState*)self)->stateVector;
}

Result mockGetStateVectorSub(SimulationState* self, int substateSize, int* qubits, Statevector* output) {
    return ERROR;
}


Result destroyMockSimulationState(MockSimulationState* self) {
    free(self->stateVector.amplitudes);
    return OK;
}

