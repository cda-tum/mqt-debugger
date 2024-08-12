package_add_test(
  mqt_debug_test
  MQT::Debug
  utils_test.cpp
  test_utility.cpp
  test_simulation.cpp
  test_data_retrieval.cpp
  test_diagnostics.cpp
  test_custom_code.cpp
  test_parsing.cpp)

# set include directories
target_include_directories(mqt_debug_test PUBLIC ${PROJECT_SOURCE_DIR}/test/utils)
