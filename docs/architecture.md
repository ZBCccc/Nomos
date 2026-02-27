# Architecture Documentation

## Experiment Framework

The project uses a factory pattern for experiments:
- `core/Experiment.hpp`: Abstract base class defining `setup()`, `run()`, `teardown()`, `getName()`
- `core/ExperimentFactory.hpp`: Factory for creating experiments by name
- `main.cpp`: Registers all experiments and dispatches based on command-line argument

## Nomos Scheme Components

Three-party architecture:

### Gatekeeper (`nomos/Gatekeeper.{hpp,cpp}`)
Key management and token generation:
- `Setup()`: Initialize master keys (Ks, Ky, Km)
- `GenToken()`: Generate search tokens (stokens for enumeration, xtokens for filtering)
- Maintains `UpdateCnt` counter and keyword state

### Server (`nomos/Server.{hpp,cpp}`)
Encrypted index storage and retrieval:
- Stores TSet (encrypted index) and XSet (cross-tags for filtering)
- `Search()`: Candidate enumeration + cross-filtering

### Client (`nomos/Client.{hpp,cpp}`)
Query initiation and result decryption:
- `Update()`: Dynamic update protocol (add/delete operations)
- Computes TSet addresses and encrypted payloads

### Key Data Structures (`nomos/types.hpp`)
- `Metadata`: Contains addr, val, alpha, xtag_list
- `TrapdoorMetadata`: Contains stokenList and xtokenList
- `sEOp`: Search operation metadata

## Verifiable Scheme Components

### QTree (`verifiable/QTree.{hpp,cpp}`)
Merkle Hash Tree implementation:
- `initialize()`: Set up MHT with capacity 1024
- `updateBit()`: Single-bit update with automatic path maintenance
- `generateProof()`, `generatePositiveProof()`, `generateNegativeProof()`: Generate authentication paths
- `getRootHash()`: Get root commitment R_X^(t)
- `verifyPath()`: Static verification function

### AddressCommitment (`verifiable/AddressCommitment.{hpp,cpp}`)
Address commitment mechanism:
- `commit()`: Compute Cm_{w,id} = H_c(xtag_1||...||xtag_ℓ)
- `verify()`: Verify commitment opening
- `checkSubsetMembership()`: Subset membership check

## Cryptographic Primitives

`core/Primitive.{hpp,cpp}` implements hash functions mapping to elliptic curve points:
- `Hash_H1()`, `Hash_H2()`: SHA256/SHA384 → curve points
- `Hash_G1()`, `Hash_G2()`: SHA512/SHA224/SHA384 → curve points
- `Hash_Zn()`: SHA512 → modular big integers

## Dependencies

- **RELIC**: Elliptic curve operations (required, installed via Homebrew)
- **OpenSSL**: Hashing and random number generation
- **GMP**: Big integer arithmetic
- **GoogleTest**: Unit testing framework

The CMakeLists.txt handles Homebrew paths for both Intel and Apple Silicon Macs.
