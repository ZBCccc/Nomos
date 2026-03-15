#pragma once

#include <gmpxx.h>

#include <string>
#include <vector>

extern "C" {
#include <relic/relic.h>
};

// Naming convention:
// - Hash_*: unkeyed hash / hash-to-curve / hash-to-field helpers.
// - F: string-valued PRF implemented as HMAC-SHA256.
// - F_p: scalar-valued PRF implemented as HMAC-SHA256 followed by Hash_Zn.
// - OPRF: historical protocol path, not used in the current experimental build.

// Unkeyed hash-to-curve helpers used by Nomos and related experiments.
void Hash_H1(ep_t out, const std::string& in);
void Hash_H2(ep_t out, const std::string& in);
void Hash_G1(ep_t out, const std::string& in);
void Hash_G2(ep_t out, const std::string& in);
void Hash_G2(ep2_t out, const std::string& in);

// Unkeyed hash-to-field helper.
void Hash_Zn(bn_t out, const std::string& in);

// Serialize a scalar so it can be reused as a PRF key or transcript component.
std::string SerializeBn(const bn_t in);

// Serialize/Deserialize elliptic curve points safely.
std::string SerializePoint(const ep_t point);
void DeserializePoint(ep_t point, const std::string& data);

// Low-level HMAC helper shared by F and F_p.
std::string HmacSha256(const std::string& key, const std::string& in);

// String-valued PRF used for F in the paper.
std::string F(const std::string& key, const std::string& in);
std::string F(const bn_t key, const std::string& in);

// Scalar-valued PRF used for F_p in the paper.
void F_p(bn_t out, const std::string& key, const std::string& in);
void F_p(bn_t out, const bn_t key, const std::string& in);
