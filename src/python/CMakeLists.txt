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
target_link_libraries(pydebugger PRIVATE MQT::Debugger MQT::CorePython pybind11_json
                                         MQT::ProjectOptions MQT::ProjectWarnings)
message(STATUS "CMAKE_INSTALL_PREFIX is set to ${CMAKE_INSTALL_PREFIX}")
# Install directive for scikit-build-core
install(
  TARGETS pydebugger
  DESTINATION .
  COMPONENT mqt-debugger_Python)
