# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## ⚠️ CRITICAL: Paper Reproduction Project

**This is an academic paper reproduction project. All implementations must strictly follow the paper specifications.**

**Primary Paper**: Bag et al. 2024 - "Tokenised Multi-client Provisioning for Dynamic Searchable Encryption with Forward and Backward Privacy" (ACM ASIA CCS 2024)

**Paper Location**: `/Users/cyan/code/paper/ref-thesis/Bag 等 - 2024 - Tokenised Multi-client Provisioning for Dynamic Searchable Encryption with Forward and Backward Priv.pdf`

**Mandatory Rules**:
- See `rules/论文复现规则.md` for strict reproduction constraints
- See `rules/编码流程.md` for Actor-Critic development workflow (Phase 0-2 review protocol)

## Project Overview

Nomos is a C++11 research project implementing three searchable encryption schemes:
1. **Nomos** - Multi-user multi-keyword dynamic SSE baseline
2. **Verifiable Nomos** - Verifiable scheme using QTree + embedded commitments
3. **MC-ODXT** - Multi-client ODXT adaptation (to be implemented)

Uses: RELIC (elliptic curves), OpenSSL (hashing), GMP (big integers).

## Quick Commands

```bash
# Build (from project root)
mkdir -p build && cd build && cmake .. && cmake --build .

# Run experiments (from build directory)
./Nomos nomos-simplified  # Nomos baseline (default)
./Nomos verifiable        # Verifiable scheme with QTree
./Nomos mc-odxt           # Multi-client ODXT (in progress)
./Nomos benchmark         # Benchmark framework

# Test (from build directory)
cd build
cmake .. -DBUILD_TESTING=ON && make
./tests/nomos_test        # Run all tests

# Format code (from build directory)
make format               # Format all sources
make check-format         # Check formatting without modifying

# Single test (from build directory)
./tests/nomos_test --gtest_filter=TestName.*
```

## Architecture Pointers

**Experiment Framework** (Factory Pattern):
- `core/Experiment.hpp` - Base class with setup()/run()/teardown()
- `core/ExperimentFactory.hpp` - Factory registry
- `main.cpp` - Registers experiments and dispatches by CLI arg

**Nomos Baseline** (Simplified Experimental Path):
- `nomos/Gatekeeper.{hpp,cpp}` - Key management and simplified token generation
  - `genTokenSimplified()` - Direct token computation used by all experiments
  - `getKm()` - Legacy compatibility hook for existing setup code
- `nomos/Server.{hpp,cpp}` - TSet/XSet storage and simplified search
  - `setup(Km)` - Compatibility no-op
  - `search()` - Candidate enumeration + cross-filtering
- `nomos/Client.{hpp,cpp}` - Search preparation and result decryption
  - `genTokenSimplified()` - Delegates token generation to Gatekeeper
- `nomos/types.hpp` - Data structures centered on the simplified search token
- `nomos/NomosSimplifiedExperiment.{hpp,cpp}` - Integration tests
- `tests/nomos_simplified_test.cpp` - Simplified search-path tests

**Verifiable Scheme**:
- `verifiable/QTree.{hpp,cpp}` - Merkle Hash Tree (1024 capacity)
- `verifiable/AddressCommitment.{hpp,cpp}` - Commitment scheme
- `verifiable/VerifiableExperiment.{hpp,cpp}` - Integration tests

**MC-ODXT** (Multi-Client, In Progress):
- `mc-odxt/McOdxtTypes.hpp` - Data structures
- `mc-odxt/McOdxtProtocol.cpp` - Core protocol (stub)
- `mc-odxt/McOdxtExperiment.{hpp,cpp}` - Experiment harness
- See `docs/MC-ODXT-Design.md` for design spec

**Crypto Primitives**:
- `core/Primitive.{hpp,cpp}` - Hash functions (H1, H2, G1, G2, Zn)

**Benchmark Framework**:
- `benchmark/BenchmarkFramework.{hpp,cpp}` - Timing utilities
- `benchmark/NomosBenchmark.{hpp,cpp}` - Nomos-specific benchmarks
- `benchmark/BenchmarkExperiment.{hpp,cpp}` - Experiment runner

## Key Parameters

- **ℓ = 3** - Cross-tags per insertion (xtag count)
- **k = 2** - Cross-tags sampled during query
- **λ = 128 bits** - Security parameter (paper Section 3.1)
- **Curve**: BN254 or BLS12-381 (RELIC pairing operations)
- **Hash / PRF**: Hash-to-curve via SHA-2 digests, `F` via `HMAC-SHA256`, `F_p` via `HMAC-SHA256 -> Hash_Zn`
- **C++11 standard** - Strict requirement for compatibility

## Implementation Status

**Current State** (updated 2026-03-14):
- ✅ **Nomos Baseline** - 100% complete, simplified experimental path only
- ✅ **Verifiable Scheme** - 90% complete, QTree path verification needs fix
- ✅ **MC-ODXT** - runnable prototype integrated into experiments
- ✅ **Benchmark Framework** - active Chapter 4 experiment entry exists

**Key Technical Decisions**:
- **Nomos token generation**: simplified direct computation only
  - Client delegates `genToken` to Gatekeeper
  - Server no longer performs env decryption or gamma/rho unblinding
- **RELIC array handling**: Serialization for storage, manual memory for fixed arrays
- **Bloom Filter XSet**: Space-efficient alternative to map-based XSet (MC-ODXT)

## Dependencies

- **RELIC** - Elliptic curve operations (install: `brew install relic`)
- **OpenSSL** - Hashing and random number generation
- **GMP** - Big integer arithmetic (install: `brew install gmp`)
- **GoogleTest** - Unit testing (auto-fetched by CMake if BUILD_TESTING=ON)

CMakeLists.txt handles Homebrew paths for both Intel (`/usr/local`) and Apple Silicon (`/opt/homebrew`).

## Development Workflow

**MANDATORY**: Follow Actor-Critic protocol in `rules/编码流程.md`:
- **Phase 0** (Pre-Implementation): Codex reviews architecture blueprint before coding
- **Phase 1** (Internal Validation): Self-review, tests must pass before Phase 2
- **Phase 2** (Codex Audit): Final review with high reasoning effort
- **Auto-commit**: Only on success (✅), never on escalation (🛑)

**Paper Reproduction Rules** (`rules/论文复现规则.md`):
- All crypto protocols must match paper algorithms exactly
- Parameters must match paper settings (λ=128, ℓ=3, k=2)
- Every core function needs `// Paper: Algorithm X (Section Y.Z)` comment
- Variable names should match paper notation (Ks, Ky, Km, stag, xtag)
- Any deviation must be documented in `docs/parameter-deviations.md`

## Documentation Structure

Load on-demand from `docs/` and root:
- `OPRF实现总结.md` - Historical OPRF implementation summary
- `任务进度-2026-03-08.md` - **Latest task progress report**
- `docs/OPRF-Implementation.md` - Historical OPRF protocol documentation
- `docs/implementation-status.md` - Current progress and test results
- `docs/paper-sources.md` - Paper references and reproduction status
- `docs/MC-ODXT-Design.md` - Multi-client ODXT design spec
- `docs/Benchmark-Plan.md` - Performance testing plan
- `docs/scheme-comparison.md` - Scheme comparison and paper chapter mapping
- `docs/parameter-deviations.md` - Documented deviations from paper
- `docs/architecture.md` - Architecture deep-dives
- `docs/crypto-protocols.md` - Cryptographic protocol specifications
- `docs/build-system.md` - Build system details
- `docs/known-issues.md` - Known issues and troubleshooting

## Code Style & Conventions

- **Style Guide**: Google C++ Style (`.clang-format`)
- **Standard**: C++11 (strict requirement)
- **Format before commit**: `make format` (from build directory)
- **Check format**: `make check-format` (CI-friendly dry-run)

**RELIC Type Handling**:
```cpp
// bn_t and ep_t are array types (bn_st[1], ep_st[1])
// Cannot use std::vector directly

// Option 1: Serialize for storage
std::vector<std::string> bstag;  // Serialized elliptic curve points

// Option 2: Manual memory for fixed arrays
bn_t* m_Kt = new bn_t[d];
for (int i = 0; i < d; ++i) {
    bn_new(m_Kt[i]);
    bn_rand_mod(m_Kt[i], ord);
}
// Don't forget: bn_free() and delete[]

// Option 3: Temporary arrays
bn_t* e = new bn_t[m];
// ... use ...
for (int j = 0; j < m; ++j) bn_free(e[j]);
delete[] e;
```

**Serialization Helpers** (see `nomos/Gatekeeper.cpp`):
- `serializePoint(ep_t)` → `std::string`
- `deserializePoint(ep_t, std::string)`

## Simplified Search Path

**Single Path**:

```
Client::genTokenSimplified(query, gatekeeper)
  -> Gatekeeper::genTokenSimplified(query)
  -> Client::prepareSearch(token, query, updateCnt)
  -> Server::search(request)
  -> Client::decryptResults(results, token)
```

**Key Points**:
- Current code is intended for experiment measurement only
- Token generation is direct; there is no query blinding step
- `Server::setup(gatekeeper.getKm())` is retained only for compatibility
- Tests: `tests/nomos_simplified_test.cpp` plus primitive coverage (`./tests/nomos_test`)

**Memory Management Critical**:
- `bn_t` and `ep_t` are array types, cannot use `std::vector<bn_t>`
- Must use manual allocation: `bn_t* array = new bn_t[n]`
- Always pair `bn_new()` with `bn_free()`, and `new[]` with `delete[]`
- Common bug: remember `bn_t`/`ep_t` are array-backed types and need explicit `bn_free()` / `ep_free()`
