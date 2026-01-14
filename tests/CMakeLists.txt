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

# --- Relic Test App ---
add_executable(relic_test_app relic_test.cpp)
target_link_libraries(relic_test_app PRIVATE nomos_core)
