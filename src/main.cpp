#include <iostream>
#include "nomos.h"

namespace nomos {

std::string getVersion() {
    return "1.0.0";
}

void printWelcome() {
    std::cout << "Welcome to Nomos!" << std::endl;
    std::cout << "Version: " << getVersion() << std::endl;
}

} // namespace nomos

int main() {
    nomos::printWelcome();
    
    std::cout << "\nNomos C++ Project Framework Initialized Successfully!" << std::endl;
    
    return 0;
}
