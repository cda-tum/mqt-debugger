cmake_minimum_required(VERSION 3.26)
project(
  mqt_debugger
  LANGUAGES CXX
  DESCRIPTION "MQT Debugger - A debugging tool for Quantum Circuits")

option(BUILD_MQT_DEBUGGER_BINDINGS "Build the MQT Debugger Python bindings" OFF)
option(BUILD_MQT_DEBUGGER_TESTS "Also build tests for the MQT Debugger project" ON)
option(BUILD_MQT_DEBUGGER_APP "Also build the CLI app for the MQT Debugger project" ON)

set(CMAKE_CXX_STANDARD 17)

if(BUILD_MQT_DEBUGGER_BINDINGS)
  # ensure that the BINDINGS option is set
  set(BINDINGS
      ON
      CACHE INTERNAL "Enable settings related to Python bindings")
  # Some common settings for finding Python
  set(Python_FIND_VIRTUALENV
      FIRST
      CACHE STRING "Give precedence to virtualenvs when searching for Python")
  set(Python_FIND_FRAMEWORK
      LAST
      CACHE STRING "Prefer Brew/Conda to Apple framework Python")
  set(Python_FIND_STRATEGY
      LOCATION
      CACHE STRING "Prefer Brew/Conda to Apple framework Python")
  set(Python_ARTIFACTS_INTERACTIVE
      ON
      CACHE BOOL "Prevent multiple searches for Python and instead cache the results.")

  # top-level call to find Python
  find_package(
    Python 3.9 REQUIRED
    COMPONENTS Interpreter Development.Module
    OPTIONAL_COMPONENTS Development.SABIModule)
endif()

include(cmake/ExternalDependencies.cmake)

# add main library code
add_subdirectory(src)

# add test app
if(BUILD_MQT_DEBUGGER_APP)
  add_subdirectory(app)
endif()

# add test code
if(BUILD_MQT_DEBUGGER_TESTS)
  enable_testing()
  include(GoogleTest)
  add_subdirectory(test)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in
               ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake IMMEDIATE @ONLY)
add_custom_target(uninstall-debugger COMMAND ${CMAKE_COMMAND} -P
                                             ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
