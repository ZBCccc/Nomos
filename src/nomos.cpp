#include "nomos/Nomos.hpp"

#include <iostream>

namespace nomos {

std::string getVersion() {
    return "1.0.0";
}

void printWelcome() {
    std::cout << "Welcome to Nomos!" << std::endl;
    std::cout << "Version: " << getVersion() << std::endl;
}

}  // namespace nomos
