package_add_test(
  mqt_debugger_test
  MQT::Debugger
  common_fixtures.cpp
  utils_test.cpp
  test_utility.cpp
  test_simulation.cpp
  test_data_retrieval.cpp
  test_diagnostics.cpp
  test_custom_code.cpp
  test_parsing.cpp
  test_assertion_movement.cpp
  test_assertion_creation.cpp)

# set include directories
target_include_directories(mqt_debugger_test PUBLIC ${PROJECT_SOURCE_DIR}/test/utils)
