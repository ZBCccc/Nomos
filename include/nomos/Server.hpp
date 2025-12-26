#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "types.hpp"

namespace nomos {

class Server {
public:
    Server();
    ~Server();

    void Update(const Metadata& metadata);
    std::vector<sEOp> Search(const TrapdoorMetadata& trapdoor_metadata);

private:
    std::unordered_map<std::string, std::string> TSet;
    std::unordered_set<std::string> XSet;
};

}  // namespace nomos
