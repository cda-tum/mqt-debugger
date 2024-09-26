/**
 * @file common.h
 * @brief Common types and functions used by the debugger.
 *
 * This file contains declarations for several common types required for
 * quantum computation, such as complex numbers, state vectors, etc.
 */

#pragma once
// NOLINTBEGIN(modernize-use-using, performance-enum-size)

#ifndef __cplusplus
#else
#include <cstddef>
extern "C" {
#endif

/**
 * @brief The result of an operation.
 *
 * Can be either `OK` or `ERROR`.
 */
typedef enum {
  /**
   * @brief Indicates that the operation was successful.
   */
  OK,
  /**
   * @brief Indicates that an error occurred during the operation.
   */
  ERROR,
} Result;

/**
 * @brief The type of classical variables.
 *
 * Supports Bool, Int, and Float.
 */
typedef enum { VarBool, VarInt, VarFloat } VariableType;

/**
 * @biref Represents the value of a classical variable.
 */
typedef union {
  /**
   * @brief The value represented as a boolean.
   */
  bool boolValue;
  /**
   * @brief The value represented as a integer.
   */
  int intValue;
  /**
   * @brief The value represented as a floating point number.
   */
  double floatValue;
} VariableValue;

/**
 * @brief Represents a classical variable.
 */
typedef struct {
  const char* name;
  VariableType type;
  VariableValue value;
} Variable;

/**
 * @brief Represents a complex number.
 */
typedef struct {
  /**
   * @brief The real component of the complex number.
   */
  double real;
  /**
   * @brief The imaginary component of the complex number.
   */
  double imaginary;
} Complex;

/**
 * @brief Represents a quantum statevector.
 *
 * The number of qubits is fixed and the number of states is 2^`numQubits`.
 */
typedef struct {
  /**
   * @brief The number of qubits in the statevector.
   */
  size_t numQubits;
  /**
   * @brief The number of states in the statevector.
   *
   * This is equal to 2^`numQubits`.
   */
  size_t numStates;
  /**
   * @brief An area of memory containing the amplitudes of the statevector.
   *
   * The allocated memory must be enough to store `numStates` Complex numbers.
   */
  Complex* amplitudes;
} Statevector;

#ifdef __cplusplus
}
#endif

// NOLINTEND(modernize-use-using, performance-enum-size)
