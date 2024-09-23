/**
 * @file DDSimDebugBindings.hpp
 * @brief This file defines methods to be used for defining Python bindings for
 * the DD Debugger.
 */

#pragma once

#include "pybind11/pybind11.h"

void bindBackend(pybind11::module& m);
