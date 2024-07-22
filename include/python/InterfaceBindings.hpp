#pragma once

#include <pybind11/pybind11.h>

void bindFramework(pybind11::module& m);

void bindDiagnostics(pybind11::module& m);
