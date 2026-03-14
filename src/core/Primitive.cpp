#include "core/Primitive.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

extern "C" {
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <relic/relic.h>
#include <sys/socket.h>
}

void Hash_H1(ep_t out, const std::string& in) {
  // Hash-to-curve helper for G1.
  unsigned char buf[64];
  SHA256((const unsigned char*)in.c_str(), in.length(), buf);

  ep_map(out, buf, 32);
}

void Hash_H2(ep_t out, const std::string& in) {
  // Alternate hash-to-curve helper for G1 with a different digest domain.
  unsigned char buf[64];
  SHA384((const unsigned char*)in.c_str(), in.length(), buf);

  ep_map(out, buf, 48);
}

void Hash_G1(ep_t out, const std::string& in) {
  // CP-ABE-oriented hash-to-curve helper.
  unsigned char buf[64];
  SHA512((const unsigned char*)in.c_str(), in.length(), buf);

  ep_map(out, buf, 64);
}

void Hash_G2(ep_t out, const std::string& in) {
  // Additional hash-to-curve helper kept for compatibility with existing code.
  unsigned char buf[64];
  SHA224((const unsigned char*)in.c_str(), in.length(), buf);

  ep_map(out, buf, 28);
}

void Hash_G2(ep2_t out, const std::string& in) {
  // Hash-to-curve helper for G2.
  unsigned char buf[64];
  SHA384((const unsigned char*)in.c_str(), in.length(), buf);

  ep2_map(out, buf, 48);
}

void Hash_Zn(bn_t out, const std::string& in) {
  // Hash arbitrary input into the scalar field of the active curve.
  unsigned char buf[64];
  SHA512((const unsigned char*)in.c_str(), in.length(), buf);

  bn_t order;
  bn_new(order);
  ep_curve_get_ord(order);

  bn_read_bin(out, buf, 64);
  bn_mod(out, out, order);

  bn_free(order);
}

std::string SerializeBn(const bn_t in) {
  const int len = bn_size_bin(in);
  std::vector<uint8_t> bytes(static_cast<size_t>(len > 0 ? len : 1), 0);
  if (len > 0) {
    bn_write_bin(bytes.data(), len, in);
  }
  return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

std::string HmacSha256(const std::string& key, const std::string& in) {
  unsigned char mac[EVP_MAX_MD_SIZE];
  unsigned int mac_len = 0;

  HMAC(EVP_sha256(),
       reinterpret_cast<const unsigned char*>(key.data()),
       static_cast<int>(key.size()),
       reinterpret_cast<const unsigned char*>(in.data()),
       in.size(),
       mac,
       &mac_len);

  return std::string(reinterpret_cast<const char*>(mac), mac_len);
}

std::string F(const std::string& key, const std::string& in) {
  return HmacSha256(key, in);
}

std::string F(const bn_t key, const std::string& in) {
  return F(SerializeBn(key), in);
}

void F_p(bn_t out, const std::string& key, const std::string& in) {
  // Two-stage keyed derivation: F(key, input) followed by Hash_Zn.
  Hash_Zn(out, F(key, in));
}

void F_p(bn_t out, const bn_t key, const std::string& in) {
  F_p(out, SerializeBn(key), in);
}
