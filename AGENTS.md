# AGENTS.md — Nomos Coding Agent Guide

This file is authoritative guidance for AI coding agents working in this repository.
See also: `CLAUDE.md` (architecture overview), `rules/论文复现规则.md` (paper rules), `rules/编码流程.md` (workflow).

---

## ⚠️ Critical: Paper Reproduction Project

This is an academic paper reproduction of **Bag et al. 2024** (ACM ASIA CCS 2024).
**Every cryptographic protocol implementation must strictly follow the paper.**

- Paper PDF: `/Users/cyan/code/paper/ref-thesis/Bag 等 - 2024 - ...Priv.pdf`
- All deviations must be documented in `docs/parameter-deviations.md`
- Security parameters: **λ=128 bits, ℓ=3, k=2**; curve: BN254 or BLS12-381

---

## Build Commands

```bash
# Configure (from project root)
mkdir -p build && cd build && cmake ..

# Build (from build/)
cmake --build .
# OR
make -j$(nproc)

# Build with tests enabled (default ON; explicitly set if needed)
cmake .. -DBUILD_TESTING=ON && make

# Format all sources (from build/)
make format

# Check formatting without modifying (CI-safe)
make check-format
```

---

## Run Commands

```bash
# From build/ directory:
./Nomos nomos-simplified                    # Nomos baseline (default)
./Nomos verifiable                          # Verifiable scheme with QTree
./Nomos mc-odxt                             # Multi-client ODXT
./Nomos benchmark                           # Benchmark framework
./Nomos chapter4-client-search-fixed-w1 --dataset all --output-dir <path>
```

---

## Test Commands

```bash
# From build/ directory:

# Run all tests
./tests/nomos_test

# Run a single test suite
./tests/nomos_test --gtest_filter=NomosSimplifiedTest.*
./tests/nomos_test --gtest_filter=PrimitiveTest.*
./tests/nomos_test --gtest_filter=McOdxtTest.*
./tests/nomos_test --gtest_filter=CpABETest.*

# Run a single test by name
./tests/nomos_test --gtest_filter=NomosSimplifiedTest.MultiKeywordSearchReturnsIntersection
./tests/nomos_test --gtest_filter=PrimitiveTest.FStringPrfMatchesHmacSha256

# RELIC integration app (not GoogleTest)
./tests/relic_test_app
```

### All Test Suites and Cases

| Suite | Tests |
|---|---|
| `NomosTest` | `VersionCheck`, `BasicSetup` |
| `NomosSimplifiedTest` | `MultiKeywordSearchReturnsIntersection`, `SingleKeywordSearchReturnsAllMatchingDocuments`, `PrepareSearchUsesTokenSnapshotAfterInterleavedUpdate` |
| `PrimitiveTest` | `FStringPrfMatchesHmacSha256`, `FpMatchesExplicitHmacPipeline`, `FStringBnKeyMatchesSerializedKeyVersion`, `FpBnKeyMatchesSerializedKeyVersion` |
| `McOdxtTest` | `SingleKeywordSearch`, `MultiKeywordIntersection`, `MatchesNomosSimplifiedResults`, `PrepareSearchUsesTokenSnapshotAfterInterleavedUpdate` |
| `CpABETest` | `SubsetMatch`, `ExactMatch`, `SupersetFail`, `DisjointFail` |

---

## Code Style

**Style base**: Google C++ Style (`.clang-format` with `BasedOnStyle: Google`, `Standard: c++11`).

### File Layout
- Headers: `include/<module>/ClassName.hpp`
- Sources: `src/<module>/ClassName.cpp`
- Tests: `tests/<module>_test.cpp`
- Modules: `nomos/`, `core/`, `verifiable/`, `mc-odxt/`, `benchmark/`

### Header Guards
Use `#pragma once` (not `#ifndef` guards).

### Includes
Order: own module header first, then STL headers (alphabetical), then third-party (`relic`, `openssl`, `gmp`).
Wrap C library headers in `extern "C" { ... }`:
```cpp
#include "nomos/Gatekeeper.hpp"

#include <stdexcept>
#include <string>
#include <vector>

extern "C" {
#include <relic/relic.h>
#include <openssl/evp.h>
}
```

### Namespaces
- Module code lives in `namespace nomos { ... }` (or `namespace mc_odxt`, etc.)
- Close with `}  // namespace nomos`
- No `using namespace` in headers

### Naming
| Entity | Convention | Example |
|---|---|---|
| Classes/Structs | `PascalCase` | `Gatekeeper`, `SearchToken` |
| Methods | `camelCase` | `genToken()`, `getUpdateCount()` |
| Private members | `m_` prefix | `m_Ks`, `m_updateCnt`, `m_d` |
| Free functions | `PascalCase` or paper symbol | `Hash_H1()`, `SerializeBn()`, `F()`, `F_p()` |
| Constants/Enums | `UPPER_SNAKE` | `OP_ADD`, `OP_DEL` |
| Local variables | `snake_case` | `ell`, `stag`, `xtag` |
| Paper symbols | Match paper notation | `Ks`, `Ky`, `Km`, `stag`, `xtag`, `bstag` |

### Comments
- Doxygen `/** @brief ... */` for public methods in headers
- Every core function **must** include a paper reference comment:
  ```cpp
  // Paper: Algorithm 3 - Search (Section 4.3)
  ```
- Inline comments use `//` with a space; end-of-line comments use `//` two spaces from code
- Mark deviations: `// DEVIATION: [reason]`

---

## RELIC Type Handling (Critical)

`bn_t` and `ep_t` are array types (`bn_st[1]`, `ep_st[1]`) — **cannot be stored directly in `std::vector`**.

```cpp
// Storage: serialize to std::string
std::vector<std::string> bstag;  // serialized ep_t points

// Fixed arrays: manual allocation only
bn_t* m_Kt = new bn_t[d];
for (int i = 0; i < d; ++i) { bn_new(m_Kt[i]); }
// Cleanup: always pair with bn_free() + delete[]
for (int i = 0; i < d; ++i) { bn_free(m_Kt[i]); }
delete[] m_Kt;

// In constructors: always null-initialize
bn_null(m_Ks);
ep_null(addr);

// Copy semantics: use memcpy for RELIC types in move constructors
std::memcpy(dst, src, sizeof(ep_t));
ep_null(src);  // null the source after move
```

Structs holding RELIC types **must** declare copy constructor/assignment `= delete` and implement move semantics manually (see `types.hpp`).

---

## Error Handling

- Use `std::runtime_error` or `std::invalid_argument` for unrecoverable errors
- Check all OpenSSL/RELIC return codes; throw on failure:
  ```cpp
  if (RAND_bytes(&sample, sizeof(sample)) != 1) {
    throw std::runtime_error("RAND_bytes failed");
  }
  ```
- No empty `catch` blocks
- No logging of sensitive data (keys, plaintext keywords, document IDs)

---

## Paper Reproduction Rules (Summary)

Full rules: `rules/论文复现规则.md`

- ✅ Every core function must cite its paper algorithm: `// Paper: Algorithm X (Section Y.Z)`
- ✅ Variable names must match paper notation (`Ks`, `Ky`, `Km`, `stag`, `xtag`, `bstap`, `strap`)
- ✅ Parameters: λ=128 bits, ℓ=3, k=2, SHA-256+, BN254/BLS12-381
- ❌ Do not use undocumented cryptographic primitives
- ❌ Do not modify protocol step order or merge steps
- ❌ Do not weaken security guarantees (forward privacy, Type-II backward privacy, query privacy)
- Any deviation → document in `docs/parameter-deviations.md` with format:
  ```
  **Paper**: <original> | **Impl**: <actual> | **Reason**: <why> | **Impact**: <analysis>
  ```

---

## Development Workflow (Actor-Critic Protocol)

Full workflow: `rules/编码流程.md`

- **Phase 0** (pre-implementation): Review architecture blueprint vs. paper before writing any code. Required for new features, architecture changes, or complex fixes. Skip only for ≤5-line trivial changes not touching critical paths.
- **Phase 1** (internal validation): Self-review + run tests. All tests must be green before Phase 2. Fix failures via targeted debugger delegation, not by deleting tests.
- **Phase 2** (Codex audit): Final review of diff against locked blueprint.
- **Phase 3**: Handoff summary. Auto-commit only on ✅ success; never on 🛑 escalation.

**Commit message format**: `fix:` / `feat:` / `refactor:` + concise body. Append:
`Co-Authored-By: Claude Sonnet 4.6 <noreply@anthropic.com>`

---

## Key Architecture Pointers

- **Experiment factory**: `core/Experiment.hpp` + `core/ExperimentFactory.hpp` + `src/main.cpp`
- **Simplified search path**: `Client::genToken` → `Gatekeeper::genToken` → `Client::prepareSearch` → `Server::search` → `Client::decryptResults`
- **Crypto primitives**: `core/Primitive.hpp` — `Hash_H1/H2/G1/G2`, `F` (HMAC-SHA256), `F_p` (HMAC-SHA256 → Hash_Zn)
- **Server::setup(Km)** is a compatibility no-op; the active token path is `Client::genToken` + `Gatekeeper::genToken`
- **QTree** (verifiable scheme): 1024-capacity Merkle Hash Tree in `verifiable/QTree.cpp`
