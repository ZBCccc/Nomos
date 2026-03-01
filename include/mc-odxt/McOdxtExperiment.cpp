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
  
  // 设置
  m_gatekeeper->setup(10);
  m_data_owner->setup();
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
  
  // 配置参数
  std::vector<size_t> user_counts = {1, 5, 10, 50, 100};
  std::vector<size_t> doc_counts = {100, 1000, 10000};
  
  for (size_t num_users : user_counts) {
    for (size_t num_docs : doc_counts) {
      std::cout << "\n--- Users: " << num_users << ", Docs: " << num_docs << " ---" << std::endl;
      
      // 1. 密钥生成时间
      auto t1 = std::chrono::high_resolution_clock::now();
      m_gatekeeper->setup(10);
      auto t2 = std::chrono::high_resolution_clock::now();
      double setup_time = std::chrono::duration<double, std::milli>(t2 - t1).count();
      
      // 2. 注册用户
      t1 = std::chrono::high_resolution_clock::now();
      for (size_t i = 0; i < num_users; i++) {
        m_gatekeeper->registerSearchUser("user" + std::to_string(i));
      }
      t2 = std::chrono::high_resolution_clock::now();
      double user_reg_time = std::chrono::duration<double, std::milli>(t2 - t1).count();
      
      // 3. 授权
      t1 = std::chrono::high_resolution_clock::now();
      for (size_t i = 0; i < num_users; i++) {
        m_gatekeeper->grantAuthorization("owner1", "user" + std::to_string(i), {});
      }
      t2 = std::chrono::high_resolution_clock::now();
      double auth_time = std::chrono::duration<double, std::milli>(t2 - t1).count();
      
      // 4. 更新测试
      t1 = std::chrono::high_resolution_clock::now();
      size_t num_updates = std::min(num_docs, (size_t)100);
      for (size_t i = 0; i < num_updates; i++) {
        auto token = m_data_owner->update(
          mcodxt::OpType::ADD,
          "doc" + std::to_string(i),
          "keyword" + std::to_string(i % 10),
          *m_gatekeeper
        );
        m_server->update(token);
      }
      t2 = std::chrono::high_resolution_clock::now();
      double update_time = std::chrono::duration<double, std::milli>(t2 - t1).count();
      
      // 5. 搜索测试
      std::unordered_map<std::string, int> updateCnt;
      updateCnt["keyword0"] = 10;
      updateCnt["keyword1"] = 10;
      
      t1 = std::chrono::high_resolution_clock::now();
      for (int i = 0; i < 10; i++) {
        auto token = m_client->genToken({"keyword0", "keyword1"}, "owner1", *m_gatekeeper);
        auto req = m_client->prepareSearch(token, {"keyword0", "keyword1"}, updateCnt);
        auto results = m_server->search(req);
      }
      t2 = std::chrono::high_resolution_clock::now();
      double search_time = std::chrono::duration<double, std::milli>(t2 - t1).count();
      
      // 输出结果
      std::cout << "Setup: " << setup_time << " ms" << std::endl;
      std::cout << "User Register (" << num_users << "): " << user_reg_time << " ms" << std::endl;
      std::cout << "Authorization: " << auth_time << " ms" << std::endl;
      std::cout << "Update (" << num_updates << "): " << update_time << " ms" << std::endl;
      std::cout << "Search (10 queries): " << search_time << " ms" << std::endl;
    }
  }
}

void McOdxtExperiment::teardown() {
  std::cout << "[MC-ODXT] Tearing down..." << std::endl;
}

std::string McOdxtExperiment::getName() const { return "mc-odxt"; }

}  // namespace mcodxt
