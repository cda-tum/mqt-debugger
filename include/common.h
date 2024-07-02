#ifndef COMMON_H
#define COMMON_H



#ifndef __cplusplus
typedef char bool;
#else
#include <cstddef>
#endif

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
    double float_value;
} VariableValue;

typedef struct {
    const char* name;
    VariableType type;
    VariableValue value;
} Variable;

typedef struct {
    double real;
    double imaginary;
} Complex;

typedef struct {
    size_t numQubits;
    size_t numStates;
    Complex* amplitudes;
} Statevector;

#endif // COMMON_H