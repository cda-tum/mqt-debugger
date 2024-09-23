/**
 * @file bindings.cpp
 * @brief Python bindings for the debug module.
 *
 * Central file for defining the Python bindings for the framework.
 */

#include "python/InterfaceBindings.hpp"
#include "python/dd/DDSimDebugBindings.hpp"

#include <pybind11/pybind11.h>

PYBIND11_MODULE(pydebug, m) {
  bindDiagnostics(m);
  bindFramework(m);
  bindBackend(m);
}
