#pragma once
#include <string>
#include <unordered_map>

class GatekeeperState {
public:
    GatekeeperState();
    ~GatekeeperState();

    bool Get(std::string& out, const std::string& keyword);

    void Put(const std::string& in, const std::string& keyword);

    void Clear();

private:
    std::unordered_map<std::string, std::string> state_map;
};