/**
 * @file InterfaceBindings.hpp
 * @brief This file defines methods to be used for defining Python bindings for
 * the debugging and diagnostics backends.
 */

#pragma once

#include "pybind11/pybind11.h"

void bindFramework(pybind11::module& m);

void bindDiagnostics(pybind11::module& m);
