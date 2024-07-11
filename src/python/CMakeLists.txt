pybind11_add_module(
  pydebug
  # Prefer thin LTO if available
  THIN_LTO
  # Optimize the bindings for size
  OPT_SIZE
  # Source code goes here
  bindings.cpp
  InterfaceBindings.cpp
  dd/DDSimDebugBindings.cpp
)
target_link_libraries(pydebug PRIVATE MQT::Debug MQT::CorePython pybind11_json MQT::ProjectOptions
                                      MQT::ProjectWarnings)
message(STATUS "CMAKE_INSTALL_PREFIX is set to ${CMAKE_INSTALL_PREFIX}")
# Install directive for scikit-build-core
install(
  TARGETS pydebug
  DESTINATION .
  COMPONENT mqt-debug_Python)
