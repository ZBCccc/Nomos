#include "vq-nomos/Common.hpp"

#include <stdexcept>
#include <vector>

extern "C" {
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/sha.h>
}

namespace vqnomos {

std::string Sha256Binary(const std::string& input) {
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256(reinterpret_cast<const unsigned char*>(input.data()), input.size(),
         hash);
  return std::string(reinterpret_cast<const char*>(hash), SHA256_DIGEST_LENGTH);
}

std::string EncodeUint32(uint32_t value) {
  std::string encoded;
  for (int shift = 24; shift >= 0; shift -= 8) {
    encoded.push_back(static_cast<char>((value >> shift) & 0xff));
  }
  return encoded;
}

std::string EncodeUint64(uint64_t value) {
  std::string encoded;
  for (int shift = 56; shift >= 0; shift -= 8) {
    encoded.push_back(static_cast<char>((value >> shift) & 0xff));
  }
  return encoded;
}

std::string BuildAnchorMessage(uint64_t version, const std::string& root_hash) {
  std::string message = EncodeUint64(version);
  message.append(root_hash);
  return message;
}

std::string BuildMerkleAuthMessage(int bucket_index,
                                   const std::string& root_hash) {
  std::string message =
      EncodeUint32(static_cast<uint32_t>(bucket_index < 0 ? 0 : bucket_index));
  message.append(root_hash);
  return message;
}

int ComputeKeywordBucketIndex(const std::string& keyword, int bucket_count) {
  if (bucket_count <= 0) {
    throw std::invalid_argument("bucket_count must be positive");
  }

  const std::string digest = Sha256Binary(keyword);
  uint32_t index = 0;
  for (int i = 0; i < 4; ++i) {
    index = (index << 8) |
            static_cast<unsigned char>(digest[static_cast<size_t>(i)]);
  }
  return static_cast<int>(index % static_cast<uint32_t>(bucket_count));
}

std::string ExportPublicKeyPem(EVP_PKEY* key_pair) {
  BIO* bio = BIO_new(BIO_s_mem());
  if (bio == NULL) {
    throw std::runtime_error("BIO_new failed while exporting public key");
  }

  if (PEM_write_bio_PUBKEY(bio, key_pair) != 1) {
    BIO_free(bio);
    throw std::runtime_error("PEM_write_bio_PUBKEY failed");
  }

  char* data = NULL;
  const long len = BIO_get_mem_data(bio, &data);
  std::string pem(data, data + len);
  BIO_free(bio);
  return pem;
}

std::string SignMessage(EVP_PKEY* key_pair, const std::string& message) {
  EVP_MD_CTX* ctx = EVP_MD_CTX_new();
  if (ctx == NULL) {
    throw std::runtime_error("EVP_MD_CTX_new failed while signing");
  }

  size_t signature_len = 0;
  if (EVP_DigestSignInit(ctx, NULL, NULL, NULL, key_pair) != 1 ||
      EVP_DigestSign(ctx, NULL, &signature_len,
                     reinterpret_cast<const unsigned char*>(message.data()),
                     message.size()) != 1) {
    EVP_MD_CTX_free(ctx);
    throw std::runtime_error("EVP_DigestSignInit failed");
  }

  std::vector<unsigned char> signature(signature_len);
  if (EVP_DigestSign(ctx, signature.data(), &signature_len,
                     reinterpret_cast<const unsigned char*>(message.data()),
                     message.size()) != 1) {
    EVP_MD_CTX_free(ctx);
    throw std::runtime_error("EVP_DigestSign failed");
  }

  EVP_MD_CTX_free(ctx);
  return std::string(reinterpret_cast<const char*>(signature.data()),
                     signature_len);
}

bool VerifyMessage(const std::string& public_key_pem,
                   const std::string& message, const std::string& signature) {
  BIO* bio = BIO_new_mem_buf(public_key_pem.data(),
                             static_cast<int>(public_key_pem.size()));
  if (bio == NULL) {
    throw std::runtime_error("BIO_new_mem_buf failed while verifying");
  }

  EVP_PKEY* public_key = PEM_read_bio_PUBKEY(bio, NULL, NULL, NULL);
  BIO_free(bio);
  if (public_key == NULL) {
    throw std::runtime_error("PEM_read_bio_PUBKEY failed");
  }

  EVP_MD_CTX* ctx = EVP_MD_CTX_new();
  if (ctx == NULL) {
    EVP_PKEY_free(public_key);
    throw std::runtime_error("EVP_MD_CTX_new failed while verifying");
  }

  const int rc = EVP_DigestVerifyInit(ctx, NULL, NULL, NULL, public_key);
  if (rc != 1) {
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(public_key);
    throw std::runtime_error("EVP_DigestVerifyInit failed");
  }

  const int verify_rc = EVP_DigestVerify(
      ctx, reinterpret_cast<const unsigned char*>(signature.data()),
      signature.size(), reinterpret_cast<const unsigned char*>(message.data()),
      message.size());

  EVP_MD_CTX_free(ctx);
  EVP_PKEY_free(public_key);
  return verify_rc == 1;
}

}  // namespace vqnomos
