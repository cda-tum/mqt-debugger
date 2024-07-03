#pragma once

#ifndef __cplusplus
typedef char bool;
#else
#include <cstddef>
extern "C" {
#endif

typedef enum {
  OK,
  ERROR,
} Result;

typedef enum { VarBool, VarInt, VarFloat } VariableType;

typedef union {
  bool boolValue;
  int intValue;
  double floatValue;
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

#ifdef __cplusplus
}
#endif
