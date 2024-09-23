/**
 * @file InterfaceBindings.hpp
 * @brief This file defines methods to be used for defining Python bindings for
 * the debugging and diagnostics backends.
 */

#pragma once

#include "pybind11/pybind11.h"

/**
 * @brief Binds the main debugging framework to Python.
 * @param m The `pybind11` module.
 */
void bindFramework(pybind11::module& m);

/**
 * @brief Binds the diagnostics backend to Python.
 * @param m The `pybind11` module.
 */
void bindDiagnostics(pybind11::module& m);
