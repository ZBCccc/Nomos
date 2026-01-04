find_package(GTest REQUIRED)

add_executable(nomos_test
    # main_test.cpp
    cp_abe_test.cpp


)

target_link_libraries(nomos_test PRIVATE
    nomos_core
    GTest::gtest
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(nomos_test)
