add_library(
  ${PROJECT_NAME}
  backend/dd/DDSimDebug.cpp common/parsing/AssertionParsing.cpp
  common/parsing/CodePreprocessing.cpp common/parsing/ParsingError.cpp common/parsing/Utils.cpp
  frontend/cli/CliFrontEnd.cpp)

# set include directories
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include
                                                  ${PROJECT_BINARY_DIR}/include)

# link to the MQT::Core libraries
target_link_libraries(${PROJECT_NAME} PUBLIC MQT::CoreDD)
target_link_libraries(${PROJECT_NAME} PRIVATE MQT::ProjectWarnings MQT::ProjectOptions)

# add MQT alias
add_library(MQT::Debug ALIAS ${PROJECT_NAME})

if(BUILD_MQT_DEBUG_BINDINGS)
  add_subdirectory(python)
endif()
