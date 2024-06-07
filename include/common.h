typedef char bool;

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
    bool bool_value;
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