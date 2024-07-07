add_executable(test_option test_option.cpp)
target_link_libraries(test_option better_option)
add_test(NAME test_option COMMAND test_option)

add_executable(test_result test_result.cpp)
target_link_libraries(test_result better_option)
add_test(NAME test_result COMMAND test_result)

add_executable(bench bench.cpp)
target_link_libraries(bench better_option)
