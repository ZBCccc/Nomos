include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

add_executable(nomos_test
    main_test.cpp
)

target_link_libraries(nomos_test PRIVATE
    nomos_core
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(nomos_test)
