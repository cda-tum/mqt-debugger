/**
 * @file bindings.cpp
 * @brief Python bindings for the debugger module.
 *
 * Central file for defining the Python bindings for the framework.
 */

#include "python/InterfaceBindings.hpp"
#include "python/dd/DDSimDebugBindings.hpp"

#include <pybind11/pybind11.h>

PYBIND11_MODULE(pydebugger, m) {
  bindDiagnostics(m);
  bindFramework(m);
  bindBackend(m);
}
