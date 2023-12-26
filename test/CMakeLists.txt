enable_testing()
include(GoogleTest)
add_executable(ut_test test.cpp)
target_link_libraries(ut_test PRIVATE gtest gtest_main art_shared)

gtest_discover_tests(ut_test)