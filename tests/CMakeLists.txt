find_package(GTest REQUIRED)

add_executable(nomos_test
    # main_test.cpp
    mc_odxt_test.cpp
    nomos_simplified_test.cpp
    primitive_test.cpp
    qtree_test.cpp
    search_fixed_w1_smoke_test.cpp
    three_scheme_correctness_test.cpp
    vqnomos_test.cpp
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
