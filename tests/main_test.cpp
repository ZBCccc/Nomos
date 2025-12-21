#include <gtest/gtest.h>
#include "nomos/Nomos.hpp"

TEST(NomosTest, VersionCheck) {
    // This assumes nomos::getVersion() is available.
    // We need to implement it properly in the source.
    EXPECT_FALSE(nomos::getVersion().empty());
}

TEST(NomosTest, BasicSetup) {
    EXPECT_TRUE(true);
}
