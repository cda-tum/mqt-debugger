# Copyright (c) 2024 - 2025 Chair for Design Automation, TUM Copyright (c) 2025 Munich Quantum
# Software Company GmbH All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

pybind11_add_module(
  pydebugger
  # Prefer thin LTO if available
  THIN_LTO
  # Optimize the bindings for size
  OPT_SIZE
  # Source code goes here
  bindings.cpp
  InterfaceBindings.cpp
  dd/DDSimDebugBindings.cpp)
target_link_libraries(pydebugger PRIVATE MQT::Debugger pybind11_json MQT::ProjectOptions
                                         MQT::ProjectWarnings)
message(STATUS "CMAKE_INSTALL_PREFIX is set to ${CMAKE_INSTALL_PREFIX}")
# Install directive for scikit-build-core
install(
  TARGETS pydebugger
  DESTINATION .
  COMPONENT mqt-debugger_Python)
