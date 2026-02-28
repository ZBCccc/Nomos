#pragma once

#include <gmpxx.h>

#include <memory>
#include <string>
#include <vector>

extern "C" {
#include <relic/relic.h>
}

namespace nomos {

class Client {
public:
    Client();
    ~Client();

    int Setup();
};

}  // namespace nomos