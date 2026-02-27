# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Nomos is a C++11 research project implementing three searchable encryption schemes for academic evaluation:
1. **Nomos** - Multi-user multi-keyword dynamic SSE baseline scheme
2. **Verifiable Nomos** - Verifiable scheme using QTree (Merkle Hash Tree) + embedded commitments
3. **MC-ODXT** - Multi-client ODXT adaptation (to be implemented)

The project uses cryptographic primitives from RELIC (elliptic curves), OpenSSL (hashing), and GMP (big integers).

## Build Commands

### Initial Setup
```bash
# Configure with CMake (from project root)
mkdir -p build && cd build
cmake ..

# Or use presets for platform-specific configuration
cmake --preset default-unix ..    # macOS/Linux with Clang
cmake --preset default-windows .. # Windows with MinGW
```

### Building
```bash
# From build directory
cmake --build .

# Or from project root using Makefile
make build
```

### Running Experiments
```bash
# From build directory
./Nomos nomos        # Run Nomos baseline scheme
./Nomos verifiable   # Run verifiable scheme
./Nomos mc-odxt      # Run MC-ODXT (not yet implemented)
```

### Testing
```bash
# Build with tests enabled
cd build
cmake .. -DBUILD_TESTING=ON
make

# Run all tests
./tests/nomos_test

# Run specific test suite
./tests/nomos_test --gtest_filter=CpABETest.*

# Run RELIC test app
./tests/relic_test_app
```

### Code Formatting
```bash
# Format all source files (requires clang-format)
cd build
cmake --build . --target format

# Check formatting without modifying files
cmake --build . --target check-format

# Or from project root
make fmt
```

## Architecture

### Experiment Framework
The project uses a factory pattern for experiments:
- `core/Experiment.hpp`: Abstract base class defining `setup()`, `run()`, `teardown()`, `getName()`
- `core/ExperimentFactory.hpp`: Factory for creating experiments by name
- `main.cpp`: Registers all experiments and dispatches based on command-line argument

### Nomos Scheme Components
Three-party architecture:
- **Gatekeeper** (`nomos/Gatekeeper.{hpp,cpp}`): Key management and token generation
  - `Setup()`: Initialize master keys (Ks, Ky, Km)
  - `GenToken()`: Generate search tokens (stokens for enumeration, xtokens for filtering)
  - Maintains `UpdateCnt` counter and keyword state
- **Server** (`nomos/Server.{hpp,cpp}`): Encrypted index storage and retrieval
  - Stores TSet (encrypted index) and XSet (cross-tags for filtering)
  - `Search()`: Candidate enumeration + cross-filtering
- **Client** (`nomos/Client.{hpp,cpp}`): Query initiation and result decryption
  - `Update()`: Dynamic update protocol (add/delete operations)
  - Computes TSet addresses and encrypted payloads

Key data structures in `nomos/types.hpp`:
- `Metadata`: Contains addr, val, alpha, xtag_list
- `TrapdoorMetadata`: Contains stokenList and xtokenList
- `sEOp`: Search operation metadata

### Verifiable Scheme Components
- **QTree** (`verifiable/QTree.{hpp,cpp}`): Merkle Hash Tree implementation
  - `initialize()`: Set up MHT with capacity 1024
  - `updateBit()`: Single-bit update with automatic path maintenance
  - `generateProof()`, `generatePositiveProof()`, `generateNegativeProof()`: Generate authentication paths
  - `getRootHash()`: Get root commitment R_X^(t)
  - `verifyPath()`: Static verification function
- **AddressCommitment** (`verifiable/AddressCommitment.{hpp,cpp}`): Address commitment mechanism
  - `commit()`: Compute Cm_{w,id} = H_c(xtag_1||...||xtag_ℓ)
  - `verify()`: Verify commitment opening
  - `checkSubsetMembership()`: Subset membership check

### Cryptographic Primitives
`core/Primitive.{hpp,cpp}` implements hash functions mapping to elliptic curve points:
- `Hash_H1()`, `Hash_H2()`: SHA256/SHA384 → curve points
- `Hash_G1()`, `Hash_G2()`: SHA512/SHA224/SHA384 → curve points
- `Hash_Zn()`: SHA512 → modular big integers

### Dependencies
- **RELIC**: Elliptic curve operations (required, installed via Homebrew)
- **OpenSSL**: Hashing and random number generation
- **GMP**: Big integer arithmetic
- **GoogleTest**: Unit testing framework

The CMakeLists.txt handles Homebrew paths for both Intel and Apple Silicon Macs.

## Key Parameters
- ℓ = 3: Number of cross-tags generated per insertion
- k = 2: Number of cross-tags sampled during query
- λ = 256 bits: Security parameter
- C++11 standard (downgraded from C++20 for compatibility)

## Known Issues
- Nomos search returns empty results (cryptographic operations need refinement)
- QTree path verification fails (index calculation needs correction)
- MC-ODXT implementation is incomplete (placeholder only)
- Memory management: RELIC big numbers not fully released (potential leaks)
- Error handling is minimal (lacks exception handling and boundary checks)

## Code Style
- Uses Google C++ style guide (see `.clang-format`)
- Standard: C++11 (note: .clang-format says c++20 but project uses C++11)
- Format code before committing using `make fmt`
