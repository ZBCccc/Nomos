#pragma once

#include <string>

namespace nomos {

class Server {
public:
    Server();
    ~Server();

    void run();

private:
    bool m_isRunning;
};

}  // namespace nomos
