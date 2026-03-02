#include <gtest/gtest.h>
#include "mc-odxt/McOdxtTypes.hpp"

extern "C" {
#include <relic/relic.h>
}

using namespace mcodxt;

class McOdxtTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Only initialize RELIC if not already initialized
        if (core_get() == NULL) {
            if (core_init() != RLC_OK) {
                FAIL() << "Failed to initialize RELIC";
            }
            if (pc_param_set_any() != RLC_OK) {
                core_clean();
                FAIL() << "Failed to set pairing parameters";
            }
        }
    }

    void TearDown() override {
        // Don't clean up RELIC - it's shared across tests
    }
};

// Test basic setup and registration
TEST_F(McOdxtTest, BasicSetup) {
    McOdxtGatekeeper gatekeeper;
    ASSERT_EQ(gatekeeper.setup(10), 0);

    // Register owner and user
    ASSERT_EQ(gatekeeper.registerDataOwner("owner1"), 0);
    ASSERT_EQ(gatekeeper.registerSearchUser("user1"), 0);

    // Grant authorization
    std::vector<std::string> keywords = {"crypto", "security"};
    ASSERT_EQ(gatekeeper.grantAuthorization("owner1", "user1", keywords), 0);

    // Check authorization
    ASSERT_TRUE(gatekeeper.isAuthorized("owner1", "user1"));
}

// Test update and search end-to-end
TEST_F(McOdxtTest, UpdateAndSearch) {
    // Setup
    McOdxtGatekeeper gatekeeper;
    gatekeeper.setup(10);

    McOdxtServer server;
    McOdxtClient client("user1");
    client.setup();

    // Register owner and user
    gatekeeper.registerDataOwner("owner1");
    gatekeeper.registerSearchUser("user1");

    // Grant authorization
    std::vector<std::string> keywords = {"crypto", "security"};
    gatekeeper.grantAuthorization("owner1", "user1", keywords);

    // Owner updates: add doc1 with keywords {crypto, security}
    // Note: Owner doesn't need setup() - Gatekeeper manages all keys
    McOdxtDataOwner owner("owner1");

    try {
        auto meta1 = owner.update(OpType::ADD, "doc1", "crypto", gatekeeper);
        std::cout << "meta1 xtags size: " << meta1.xtags.size() << std::endl;
        server.update(meta1);

        auto meta2 = owner.update(OpType::ADD, "doc1", "security", gatekeeper);
        std::cout << "meta2 xtags size: " << meta2.xtags.size() << std::endl;
        server.update(meta2);
    } catch (const std::exception& e) {
        std::cout << "Exception during update: " << e.what() << std::endl;
        FAIL() << "Update failed: " << e.what();
    }

    // Verify TSet and XSet have entries
    EXPECT_GT(server.getTSetSize(), 0);
    EXPECT_GT(server.getXSetSize(), 0);

    // Client searches: query {crypto, security}
    auto token = client.genToken(keywords, "owner1", gatekeeper);
    auto updateCnt = gatekeeper.getUpdateCounts("owner1");
    auto req = client.prepareSearch(token, keywords, updateCnt);
    auto results = server.search(req);

    // Decrypt results
    auto decrypted = client.decryptResults(results, token, gatekeeper);

    // Verify: result should contain doc1
    // Note: This is a simplified test - the search protocol may need refinement
    std::cout << "Search returned " << results.size() << " results" << std::endl;
    std::cout << "Decrypted " << decrypted.size() << " documents" << std::endl;
    
    ASSERT_GE(decrypted.size(), 1);
    EXPECT_EQ(decrypted[0], "doc1");
}

// Test authorization expiry
TEST_F(McOdxtTest, AuthorizationExpiry) {
    Authorization auth;
    auth.owner_id = "owner1";
    auth.user_id = "user1";
    auth.expiry = 0;  // Never expires

    EXPECT_TRUE(auth.isValid());

    // Set expiry to past time
    auth.expiry = 1;  // Unix epoch + 1 second (definitely expired)
    EXPECT_FALSE(auth.isValid());

    // Set expiry to future time (1 year from now)
    auto now = std::chrono::system_clock::now();
    auto future = now + std::chrono::hours(24 * 365);
    auth.expiry = std::chrono::system_clock::to_time_t(future);
    EXPECT_TRUE(auth.isValid());
}
