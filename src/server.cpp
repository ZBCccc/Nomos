#include "nomos/Server.hpp"

#include <iostream>

namespace nomos {

Server::Server() : m_isRunning(false) {}

Server::~Server() {}

void Server::run() {
    m_isRunning = true;
    std::cout << "Server is running..." << std::endl;
}

}  // namespace nomos
