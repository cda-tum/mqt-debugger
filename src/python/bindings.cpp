#include "python/InterfaceBindings.hpp"
#include "python/dd/DDSimDebugBindings.hpp"

#include <pybind11/pybind11.h>

namespace py = pybind11;

PYBIND11_MODULE(pydebug, m) {
  bindFramework(m);
  bindBackend(m);
}
