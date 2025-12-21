#include <iostream>

#include "nomos/Client.hpp"
#include "nomos/Nomos.hpp"
#include "nomos/Server.hpp"

int main() {
    nomos::printWelcome();

    nomos::Client client;
    client.Setup();

    nomos::Server server;
    server.run();

    return 0;
}