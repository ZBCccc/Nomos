#include "nomos/Server.hpp"

#include <iostream>

using namespace nomos;

Server::Server() {}

Server::~Server() {}

void Server::Update(const Metadata& metadata) {
  TSet[metadata.addr] = metadata.val;
  for (const auto& xtag : metadata.xtag_list) {
    XSet.insert(xtag);
  }
}

std::vector<sEOp> Server::Search(const TrapdoorMetadata& trapdoor_metadata) {
  std::vector<sEOp> sEop_list;
  for (size_t i = 0; i < trapdoor_metadata.stokenList.size(); ++i) {
    const std::string& stoken = trapdoor_metadata.stokenList[i];

    int count = 0;
  }
  return sEop_list;
}