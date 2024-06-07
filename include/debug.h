#ifndef DEBUG_H
#define DEBUG_H

typedef enum {
    OK,
    ERROR,
} Result;

typedef enum {
    VAR_BOOL,
    VAR_INT,
    VAR_FLOAT
} VariableType;

typedef union {
    char bool_value;
    int int_value;
    float float_value;
} VariableValue;

typedef struct {
    char* name;
    VariableType type;
    VariableValue value;
} Variable;

typedef struct {
    Variable* variables;
    int numVariables;
    int maxVariables;
} VariableList;

typedef struct {
    float real;
    float imaginary;
} Complex;

typedef struct {
    int numQubits;
    int numStates;
    Complex* amplitudes;
} Statevector;

typedef struct SimulationState SimulationState;

struct SimulationState {
    Result (*init)(SimulationState* self);
    Result (*stepForward)(SimulationState* self);
    Result (*stepBackward)(SimulationState* self);
    Result (*runSimulation)(SimulationState* self);
    Result (*resetSimulation)(SimulationState* self);
    char (*canStepForward)(SimulationState* self);
    char (*canStepBackward)(SimulationState* self);
    char (*isFinished)(SimulationState* self);

    int (*getCurrentLine)(SimulationState* self);
    Result (*getAmplitudeIndex)(SimulationState* self, int qubit, Complex* output);
    Result (*getAmplitudeBitstring)(SimulationState* self, char* bitstring, Complex* output);
    Result (*getClassicalVariable)(SimulationState* self, char* name, Variable* output);
    Result (*getStateVectorFull)(SimulationState* self, Statevector* output);
    Result (*getStateVectorSub)(SimulationState* self, int substateSize, int* qubits, Statevector* output);
};

#endif // DEBUG_H