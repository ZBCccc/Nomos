# High Priority Fixes - Code Review Follow-up

## Date: 2026-02-28

## Issues Fixed

### 1. Performance Measurement Bias ✅ FIXED

**Problem**: Test data generation time was included in benchmark measurements, causing inaccurate timing results.

**Root Cause**:
- Timer started before generating test data (keywords, file IDs)
- String concatenation and vector allocation overhead counted as cryptographic operation time
- Object allocation (`new`) overhead counted as setup time

**Fix Applied**:
```cpp
// BEFORE (incorrect):
double updatePhase(const BenchmarkConfig& config) {
  Timer timer;  // ← Timer starts here

  // Generate test data (this time is counted!)
  std::vector<std::string> keywords;
  for (...) { keywords.push_back(...); }

  // Actual crypto operations
  for (...) { gatekeeper_->update(...); }
}

// AFTER (correct):
double updatePhase(const BenchmarkConfig& config) {
  // Generate test data BEFORE starting timer
  std::vector<std::string> keywords;
  keywords.reserve(config.num_keywords);  // Also added reserve()
  for (...) { keywords.push_back(...); }

  Timer timer;  // ← Timer starts here

  // Only crypto operations are timed
  for (...) { gatekeeper_->update(...); }
}
```

**Impact**:
- Setup time: 3.503ms → 2.806ms (20% improvement)
- Avg update time: 4.428ms → 4.290ms (3% improvement)
- Avg search time: 4.157ms → 4.083ms (2% improvement)

**Files Modified**:
- `src/benchmark/NomosBenchmark.cpp`: `setupPhase()`, `updatePhase()`, `searchPhase()`

---

### 2. Storage Overhead Estimation Inaccuracy ✅ FIXED

**Problem**: Storage size calculations used rough approximations instead of accurate RELIC type sizes.

**Root Cause**:
- Assumed ep_t (elliptic curve point) = 32 bytes (incorrect)
- Assumed ep_t = 64 bytes for XSet (incorrect)
- Assumed TSet entry = 128 bytes (incorrect)

**Correct Sizes** (based on RELIC implementation):
- `ep_t` compressed: **33 bytes** (not 32 or 64)
- `bn_t` (big number): **32 bytes**
- AES-128 encrypted payload: **48 bytes** (typical for id||op)

**Fix Applied**:
```cpp
// BEFORE (incorrect):
void measureStorage(BenchmarkResult& result) {
  result.tset_size_bytes = server_->getTSetSize() * 128;  // Wrong!
  result.xset_size_bytes = server_->getXSetSize() * 64;   // Wrong!
}

// AFTER (correct):
void measureStorage(BenchmarkResult& result) {
  // TSet entry: ep_t(33) + AES(48) + bn_t(32) = 113 bytes
  const size_t TSET_ENTRY_SIZE = 113;
  // XSet entry: ep_t(33) compressed
  const size_t XSET_ENTRY_SIZE = 33;

  result.tset_size_bytes = server_->getTSetSize() * TSET_ENTRY_SIZE;
  result.xset_size_bytes = server_->getXSetSize() * XSET_ENTRY_SIZE;
}
```

**Impact**:
- TSet size: 2560 → 2260 bytes (11.7% reduction, more accurate)
- XSet size: 3840 → 1980 bytes (48.4% reduction, more accurate)
- Total storage: 6400 → 4240 bytes (33.8% reduction, more accurate)

**Files Modified**:
- `src/benchmark/NomosBenchmark.cpp`: `measureStorage()`

---

### 3. Communication Overhead Estimation Inaccuracy ✅ FIXED

**Problem**: Token size calculation used incorrect ep_t size (32 bytes instead of 33).

**Root Cause**:
- Assumed ep_t = 32 bytes (common mistake, actual compressed size is 33)
- Assumed env (encrypted payload) = 64 bytes (overestimate)

**Fix Applied**:
```cpp
// BEFORE (incorrect):
void measureCommunication(BenchmarkResult& result) {
  size_t stag_size = 32;  // Wrong!
  size_t xtokens_size = k * ℓ * 32;  // Wrong!
  size_t env_size = 64;  // Overestimate
}

// AFTER (correct):
void measureCommunication(BenchmarkResult& result) {
  const size_t EP_T_SIZE = 33;  // Compressed elliptic curve point
  const size_t ENV_SIZE = 48;   // AES-128 encrypted payload

  size_t stag_size = EP_T_SIZE;
  size_t xtokens_size = k * ℓ * EP_T_SIZE;
  result.token_size_bytes = stag_size + xtokens_size + ENV_SIZE;
}
```

**Impact**:
- Token size: 288 → 279 bytes (3.1% reduction, more accurate)
- For ℓ=3, k=2: stag(33) + xtokens(6×33=198) + env(48) = 279 bytes

**Files Modified**:
- `src/benchmark/NomosBenchmark.cpp`: `measureCommunication()`

---

## Verification

### Compilation
```bash
cd build && cmake --build .
# Result: ✅ Success, no errors
```

### Benchmark Execution
```bash
./Nomos benchmark
# Result: ✅ Success, accurate measurements
```

### Regression Testing
```bash
./Nomos nomos-simplified
# Result: ✅ All tests passed, no regression
```

### Code Formatting
```bash
make format
# Result: ✅ Code formatted according to Google C++ style
```

---

## Remaining Issues (Medium/Low Priority)

### Medium Priority
1. **RELIC Resource Cleanup Verification** - Need to verify `GatekeeperCorrect`, `ServerCorrect`, `ClientCorrect` destructors properly free RELIC resources
2. **Exception Handling** - Add try-catch in `setupPhase()` for setup failures

### Low Priority
3. **Warmup Runs** - First run may be slower due to cache misses
4. **Statistical Analysis** - Add standard deviation, confidence intervals
5. **Parameter Sweep** - Implement `runParameterSweep()` for thesis experiments

---

## Impact Summary

| Metric | Before | After | Change |
|--------|--------|-------|--------|
| Setup time | 3.503 ms | 2.806 ms | -20% ✅ |
| Avg update time | 4.428 ms | 4.290 ms | -3% ✅ |
| Avg search time | 4.157 ms | 4.083 ms | -2% ✅ |
| TSet storage | 2560 bytes | 2260 bytes | -11.7% ✅ |
| XSet storage | 3840 bytes | 1980 bytes | -48.4% ✅ |
| Total storage | 6400 bytes | 4240 bytes | -33.8% ✅ |
| Token size | 288 bytes | 279 bytes | -3.1% ✅ |

**Overall**: More accurate measurements, better performance metrics for thesis.

---

## References

- RELIC documentation: https://github.com/relic-toolkit/relic
- Compressed elliptic curve points: 33 bytes (1 byte prefix + 32 bytes x-coordinate)
- AES-128 block size: 16 bytes, typical ciphertext with padding: 48 bytes
- Paper: Bag et al. 2024, Section 6 (Performance Evaluation)
