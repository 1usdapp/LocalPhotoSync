find_package(GTest REQUIRED)
enable_testing()

# Example smoke test target
add_executable(tests_example tests/test_example.cpp)
target_link_libraries(tests_example PRIVATE GTest::gtest_main)
target_include_directories(tests_example PRIVATE ${CMAKE_SOURCE_DIR}/src)

add_test(NAME tests_example COMMAND tests_example)
