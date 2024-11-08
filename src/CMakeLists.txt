add_library(
  ${PROJECT_NAME}
  backend/dd/DDSimDebug.cpp
  backend/dd/DDSimDiagnostics.cpp
  common/ComplexMathematics.cpp
  common/ComplexMathematics.cpp
  common/parsing/AssertionParsing.cpp
  common/parsing/AssertionTools.cpp
  common/parsing/CodePreprocessing.cpp
  common/parsing/ParsingError.cpp
  common/parsing/Utils.cpp
  frontend/cli/CliFrontEnd.cpp)

# set include directories
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include
                                                  ${PROJECT_BINARY_DIR}/include)

# link to the MQT::Core libraries
target_link_libraries(${PROJECT_NAME} PUBLIC MQT::CoreDD MQT::CoreIR MQT::CoreCircuitOptimizer)
target_link_libraries(${PROJECT_NAME} PRIVATE MQT::ProjectWarnings MQT::ProjectOptions)
target_link_libraries(${PROJECT_NAME} PRIVATE Eigen3::Eigen)

# add MQT alias
add_library(MQT::Debugger ALIAS ${PROJECT_NAME})

if(BUILD_MQT_DEBUGGER_BINDINGS)
  add_subdirectory(python)
endif()
