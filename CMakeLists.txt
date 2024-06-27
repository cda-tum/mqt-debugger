cmake_minimum_required(VERSION 3.26)
project(mqt_debug)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
set(FETCH_PACKAGES "")

set(MQT_CORE_VERSION 2.4.0 CACHE STRING "MQT Core version")
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

add_executable(mqt_debug
        include/common.h
        include/ddsimDebug.hpp
        include/debug.h
        src/ddsimDebug.cpp
        test/testDDSimDebugger.cpp)

# Link against mqt-core library
target_link_libraries(mqt_debug
        nlohmann_json
        boost_multiprecision
        gtest
        MQT::CoreDD MQT::ProjectOptions MQT::ProjectWarnings)
