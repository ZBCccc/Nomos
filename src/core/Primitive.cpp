#include "core/Primitive.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string>

extern "C" {
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <relic/relic.h>
#include <sys/socket.h>
}

void Hash_H1(ep_t out, const std::string& in) {
  unsigned char buf[64];
  SHA256((const unsigned char*)in.c_str(), in.length(), buf);

  ep_map(out, buf, 32);
}

void Hash_H2(ep_t out, const std::string& in) {
  unsigned char buf[64];
  SHA384((const unsigned char*)in.c_str(), in.length(), buf);

  ep_map(out, buf, 48);
}

void Hash_G1(ep_t out, const std::string& in) {
  unsigned char buf[64];
  SHA512((const unsigned char*)in.c_str(), in.length(), buf);

  ep_map(out, buf, 64);
}

void Hash_G2(ep_t out, const std::string& in) {
  unsigned char buf[64];
  SHA224((const unsigned char*)in.c_str(), in.length(), buf);

  ep_map(out, buf, 28);
}
