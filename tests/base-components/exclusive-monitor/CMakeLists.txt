add_executable(test_exclusive_monitor tests.cc)
target_link_libraries(test_exclusive_monitor gtest gmock exclusive_monitor router ${TARGET_LIBS})
add_test(NAME test_exclusive_monitor COMMAND test_exclusive_monitor)
