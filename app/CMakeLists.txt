# Copyright (c) 2024 - 2025 Chair for Design Automation, TUM Copyright (c) 2025 Munich Quantum
# Software Company GmbH All rights reserved.
#
# SPDX-License-Identifier: MIT
#
# Licensed under the MIT License

add_executable(mqt_debugger_app testDDSimDebugger.cpp)

# set include directories
target_include_directories(mqt_debugger_app PUBLIC ${PROJECT_SOURCE_DIR}/include
                                                   ${PROJECT_BINARY_DIR}/include)

# link to the MQT::Core libraries
target_link_libraries(mqt_debugger_app PRIVATE ${PROJECT_NAME})
target_link_libraries(mqt_debugger_app PUBLIC MQT::CoreDD)
target_link_libraries(mqt_debugger_app PRIVATE MQT::ProjectWarnings MQT::ProjectOptions)
