#include "mc-odxt/McOdxtExperiment.hpp"
#include "mc-odxt/McOdxtTypes.hpp"

#include <iostream>
#include <vector>
#include <chrono>
#include <memory>

namespace mcodxt {

McOdxtExperiment::McOdxtExperiment() {}

McOdxtExperiment::~McOdxtExperiment() {}

int McOdxtExperiment::setup() {
  std::cout << "[MC-ODXT] Setting up multi-user ODXT experiment..." << std::endl;

  // 初始化组件 (使用 new for C++11)
  m_gatekeeper.reset(new McOdxtGatekeeper());
  m_server.reset(new McOdxtServer());
  m_client.reset(new McOdxtClient("user1"));
  m_data_owner.reset(new McOdxtDataOwner("owner1"));

  // 设置 Gatekeeper
  m_gatekeeper->setup(10);

  // 注册 owner 和 client (MC-ODXT: Gatekeeper manages all keys)
  m_gatekeeper->registerDataOwner("owner1");
  m_gatekeeper->registerSearchUser("user1");

  // 授权 client 访问 owner 的数据
  m_gatekeeper->grantAuthorization("owner1", "user1", {});

  // Client setup
  m_client->setup();

  std::cout << "[MC-ODXT] Setup complete!" << std::endl;
  return 0;
}

void McOdxtExperiment::run() {
  std::cout << "[MC-ODXT] Running experiment..." << std::endl;

  // 运行基准测试
  runBenchmark();

  std::cout << "[MC-ODXT] Experiment complete!" << std::endl;
}

void McOdxtExperiment::runBenchmark() {
  std::cout << "\n=== MC-ODXT Benchmark ===" << std::endl;

  // Simplified benchmark - single configuration
  size_t num_users = 5;
  size_t num_docs = 100;

  std::cout << "\n--- Configuration: " << num_users << " users, " << num_docs << " documents ---" << std::endl;

  // 1. Register additional users
  std::cout << "  [1/5] Registering users..." << std::endl;
  auto t1 = std::chrono::high_resolution_clock::now();
  for (size_t i = 2; i <= num_users; i++) {  // user1 already registered
    m_gatekeeper->registerSearchUser("user" + std::to_string(i));
    m_gatekeeper->grantAuthorization("owner1", "user" + std::to_string(i), {});
  }
  auto t2 = std::chrono::high_resolution_clock::now();
  double user_reg_time = std::chrono::duration<double, std::milli>(t2 - t1).count();

  // 2. Update test
  std::cout << "  [2/5] Running updates..." << std::endl;
  t1 = std::chrono::high_resolution_clock::now();
  for (size_t i = 0; i < num_docs; i++) {
    try {
      auto meta = m_data_owner->update(
        mcodxt::OpType::ADD,
        "doc" + std::to_string(i),
        "keyword" + std::to_string(i % 10),
        *m_gatekeeper
      );
      m_server->update(meta);
    } catch (const std::exception& e) {
      std::cerr << "Update error: " << e.what() << std::endl;
      break;
    }
  }
  t2 = std::chrono::high_resolution_clock::now();
  double update_time = std::chrono::duration<double, std::milli>(t2 - t1).count();

  // 3. Search test
  std::cout << "  [3/5] Running searches..." << std::endl;
  std::unordered_map<std::string, int> updateCnt;
  for (int i = 0; i < 10; i++) {
    updateCnt["keyword" + std::to_string(i)] = num_docs / 10;
  }

  t1 = std::chrono::high_resolution_clock::now();
  int num_searches = 10;
  int successful_searches = 0;
  for (int i = 0; i < num_searches; i++) {
    try {
      auto token = m_client->genToken({"keyword0", "keyword1"}, "owner1", *m_gatekeeper);
      auto req = m_client->prepareSearch(token, {"keyword0", "keyword1"}, updateCnt);
      auto results = m_server->search(req);
      successful_searches++;
    } catch (const std::exception& e) {
      std::cerr << "Search error: " << e.what() << std::endl;
      break;
    }
  }
  t2 = std::chrono::high_resolution_clock::now();
  double search_time = std::chrono::duration<double, std::milli>(t2 - t1).count();

  std::cout << "  [4/5] Completed " << successful_searches << "/" << num_searches << " searches" << std::endl;

  // Output results
  std::cout << "\n=== Results ===" << std::endl;
  std::cout << "User Registration (" << (num_users - 1) << " users): " << user_reg_time << " ms" << std::endl;
  std::cout << "Updates (" << num_docs << " docs): " << update_time << " ms" << std::endl;
  std::cout << "  - Avg per update: " << (update_time / num_docs) << " ms" << std::endl;
  std::cout << "Searches (" << num_searches << " queries): " << search_time << " ms" << std::endl;
  std::cout << "  - Avg per search: " << (search_time / num_searches) << " ms" << std::endl;
  std::cout << "TSet size: " << m_server->getTSetSize() << std::endl;
  std::cout << "XSet size: " << m_server->getXSetSize() << std::endl;
  std::cout << "\n[MC-ODXT] Benchmark completed successfully!" << std::endl;
}

void McOdxtExperiment::teardown() {
  std::cout << "[MC-ODXT] Tearing down..." << std::endl;
}

std::string McOdxtExperiment::getName() const { return "mc-odxt"; }

}  // namespace mcodxt
