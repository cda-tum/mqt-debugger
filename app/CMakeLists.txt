add_executable(mqt_debug_app testDDSimDebugger.cpp)

# set include directories
target_include_directories(mqt_debug_app PUBLIC ${PROJECT_SOURCE_DIR}/include
                                                ${PROJECT_BINARY_DIR}/include)

# link to the MQT::Core libraries
target_link_libraries(mqt_debug_app PRIVATE ${PROJECT_NAME})
target_link_libraries(mqt_debug_app PUBLIC MQT::CoreDD)
target_link_libraries(mqt_debug_app PRIVATE MQT::ProjectWarnings MQT::ProjectOptions)
