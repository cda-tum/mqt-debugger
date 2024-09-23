/**
 * @file DDSimDebugBindings.hpp
 * @brief This file defines methods to be used for defining Python bindings for
 * the DD Debugger.
 */

#pragma once

#include "pybind11/pybind11.h"

/**
 * @brief Binds the dd debugging backend to Python.
 * @param m The `pybind11` module.
 */
void bindBackend(pybind11::module& m);
