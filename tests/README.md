# Tests Directory

This directory is for unit tests.

## Adding Tests

You can add unit tests here using a testing framework like:
- Google Test (gtest)
- Catch2
- Boost.Test

## Example with Google Test

1. Add Google Test to your project
2. Update CMakeLists.txt to include test targets
3. Create test files in this directory

```cpp
// Example test file: test_nomos.cpp
#include <gtest/gtest.h>
#include "nomos.h"

TEST(NomosTest, VersionTest) {
    EXPECT_EQ(nomos::getVersion(), "1.0.0");
}
```
