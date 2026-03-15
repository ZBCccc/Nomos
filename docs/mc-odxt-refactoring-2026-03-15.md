# MC-ODXT Token Generation Refactoring

**Date**: 2026-03-15
**Status**: ✅ Complete

## Motivation

The original `McOdxtGatekeeper::genTokenSimplified` implementation incorrectly combined both client-side and gatekeeper-side responsibilities into a single function. This violated the paper's protocol design where:

1. **Client** is responsible for keyword reordering (optimization)
2. **Gatekeeper** is responsible for cryptographic token generation

## Changes Made

### 1. Client-Side (`McOdxtClient::genTokenSimplified`)

**Before**: Simple delegation to gatekeeper
```cpp
SearchToken McOdxtClient::genTokenSimplified(
    const std::vector<std::string>& query_keywords,
    McOdxtGatekeeper& gatekeeper) {
  return gatekeeper.genTokenSimplified(query_keywords);
}
```

**After**: Client handles keyword reordering
```cpp
SearchToken McOdxtClient::genTokenSimplified(
    const std::vector<std::string>& query_keywords,
    McOdxtGatekeeper& gatekeeper) {
  // Step 1: Reorder keywords - put keyword with minimum update count first
  std::vector<std::string> reordered_keywords = query_keywords;

  int min_update_count = gatekeeper.getUpdateCount(reordered_keywords[0]);
  int min_index = 0;
  for (int i = 1; i < n; ++i) {
    const int current_count = gatekeeper.getUpdateCount(reordered_keywords[i]);
    if (current_count < min_update_count) {
      min_update_count = current_count;
      min_index = i;
    }
  }

  if (min_index != 0) {
    std::swap(reordered_keywords[0], reordered_keywords[min_index]);
  }

  // Step 2: Request token generation from gatekeeper
  return gatekeeper.genTokenSimplified(reordered_keywords);
}
```

### 2. Gatekeeper-Side (`McOdxtGatekeeper::genTokenSimplified`)

**Before**: Combined keyword reordering + token generation
- Performed keyword reordering (client responsibility)
- Generated cryptographic tokens (gatekeeper responsibility)

**After**: Pure cryptographic token generation
- Assumes keywords are already reordered by client
- Focuses solely on cryptographic operations:
  1. Compute `strap = H(w1)^Ks`
  2. Compute `bstag_j = H(w1||j||0)^Kt[I(w1)]` for j=1..m
  3. Compute `delta_j = H(w1||j||1)^Kt[I(w1)]` for j=1..m
  4. Compute `bxtrap_i = H(wi)^Kx[I(wi)]` for i=2..n (no RBF)

### 3. Interface Cleanup

**Removed**: `TokenRequest` struct (unused intermediate representation)
```cpp
struct TokenRequest {
  std::string w1;
  std::string hw1;
  std::vector<std::string> hw1_j_0;
  std::vector<std::string> hw1_j_1;
  std::vector<std::string> other_keywords;
  std::vector<std::string> hwi;
};
```

**New signature**: Direct vector of keywords
```cpp
SearchToken genTokenSimplified(
    const std::vector<std::string>& query_keywords);
```

## Key Differences from Nomos

### MC-ODXT Simplifications

1. **No RBF (Random Bloom Filter)**
   - Nomos: `bxtrap_j[t] = xtrap_j^beta[t]` where beta is randomly sampled
   - MC-ODXT: `bxtrap_i = xtrap_i` (direct computation, no exponentiation)

2. **Single xtag per update**
   - Nomos: `ℓ=3` xtags per insertion
   - MC-ODXT: Single xtag per insertion

3. **Simplified cross-keyword matching**
   - Nomos: Uses `k=2` samples from RBF
   - MC-ODXT: Direct matching without sampling

## Verification

All tests pass after refactoring:
```bash
$ ./tests/nomos_test --gtest_filter='McOdxt*'
[==========] Running 8 tests from 1 test suite.
[  PASSED  ] 8 tests.
```

## Alignment with Paper

This refactoring aligns MC-ODXT implementation with the original Nomos paper's protocol design:

- **Client** (Algorithm 4, Client side): Keyword reordering for optimization
- **Gatekeeper** (Algorithm 3): Cryptographic token generation using master keys
- **Server** (Algorithm 4, Server side): Search execution using tokens

## Files Modified

1. `src/mc-odxt/McOdxtClient.cpp` - Added keyword reordering logic
2. `src/mc-odxt/McOdxtGatekeeper.cpp` - Removed keyword reordering, pure crypto
3. `include/mc-odxt/McOdxtGatekeeper.hpp` - Updated signature, removed TokenRequest

## References

- Bag et al. 2024 - "Tokenised Multi-client Provisioning for Dynamic Searchable Encryption"
- Nomos implementation: `src/nomos/Client.cpp`, `src/nomos/Gatekeeper.cpp`
