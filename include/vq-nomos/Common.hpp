#pragma once

#include <cstdint>
#include <string>

extern "C" {
#include <openssl/evp.h>
}

namespace vqnomos {

std::string Sha256Binary(const std::string& input);

std::string EncodeUint32(uint32_t value);
std::string EncodeUint64(uint64_t value);

std::string BuildAnchorMessage(uint64_t version, const std::string& root_hash);
std::string BuildMerkleAuthMessage(int bucket_index,
                                   const std::string& root_hash);

int ComputeKeywordBucketIndex(const std::string& keyword, int bucket_count);

std::string ExportPublicKeyPem(EVP_PKEY* key_pair);
std::string SignMessage(EVP_PKEY* key_pair, const std::string& message);
bool VerifyMessage(const std::string& public_key_pem,
                   const std::string& message, const std::string& signature);

}  // namespace vqnomos
