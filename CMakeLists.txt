cmake_minimum_required(VERSION 3.26)
project(mqt_debug)

option(BUILD_MQT_DEBUG_BINDINGS "Build the MQT DEBUG Python bindings" ON)
set(BUILD_MQT_DEBUG_BINDINGS ON)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)
set(FETCH_PACKAGES "")

# Fetch pybind11
FetchContent_Declare(
  pybind11
  GIT_REPOSITORY https://github.com/pybind/pybind11.git
  GIT_TAG v2.10.4 # Specify the version you need
)
list(APPEND FETCH_PACKAGES pybind11)

set(MQT_CORE_VERSION
    2.4.0
    CACHE STRING "MQT Core version")
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
  FetchContent_Declare(
    mqt-core
    GIT_REPOSITORY https://github.com/cda-tum/mqt-core.git
    GIT_TAG v${MQT_CORE_VERSION}
    FIND_PACKAGE_ARGS ${MQT_CORE_VERSION})
  list(APPEND FETCH_PACKAGES mqt-core)
else()
  find_package(mqt-core ${MQT_CORE_VERSION} QUIET)
  if(NOT mqt-core_FOUND)
    FetchContent_Declare(
      mqt-core
      GIT_REPOSITORY https://github.com/cda-tum/mqt-core.git
      GIT_TAG v${MQT_CORE_VERSION})
    list(APPEND FETCH_PACKAGES mqt-core)
  endif()
endif()

# Make all declared dependencies available.
FetchContent_MakeAvailable(${FETCH_PACKAGES})

include_directories(include)

# Create a Python module
if(BUILD_MQT_DEBUG_BINDINGS)
  pybind11_add_module(mqt_debug_bindings src/python/bindings.cpp
                      src/python/dd/DDSimDebugBindings.cpp)

  # Link against pybind11 and other necessary libraries
  target_link_libraries(
    mqt_debug_bindings
    PRIVATE pybind11::module
            nlohmann_json
            boost_multiprecision
            gtest
            MQT::CoreDD
            MQT::ProjectOptions
            MQT::ProjectWarnings)
endif()

# For other parts of your project, you might still need to build an executable
if(NOT BUILD_MQT_DEBUG_BINDINGS)
  add_executable(
    mqt_debug
    include/backend/dd/DDSimDebug.hpp
    include/backend/debug.h
    include/common.h
    include/common/parsing/AssertionParsing.hpp
    include/common/parsing/CodePreprocessing.hpp
    include/common/parsing/ParsingError.hpp
    include/common/parsing/Utils.hpp
    include/frontend/cli/CliFrontEnd.hpp
    include/python/dd/DDSimDebugBindings.hpp
    include/python/InterfaceBindings.hpp
    src/backend/dd/DDSimDebug.cpp
    src/common/parsing/AssertionParsing.cpp
    src/common/parsing/CodePreprocessing.cpp
    src/common/parsing/ParsingError.cpp
    src/common/parsing/Utils.cpp
    src/frontend/cli/CliFrontEnd.cpp
    src/python/bindings.cpp
    src/python/dd/DDSimDebugBindings.cpp
    src/python/InterfaceBindings.cpp
    test/testDDSimDebugger.cpp)

  # Link against mqt-core library
  target_link_libraries(
    mqt_debug
    nlohmann_json
    boost_multiprecision
    gtest
    MQT::CoreDD
    MQT::ProjectOptions
    MQT::ProjectWarnings)
endif()
