#ifndef NOMOS_H
#define NOMOS_H

#include <string>

namespace nomos {

/**
 * @brief Get the version of the Nomos project
 * @return Version string
 */
std::string getVersion();

/**
 * @brief Print a welcome message
 */
void printWelcome();

} // namespace nomos

#endif // NOMOS_H
