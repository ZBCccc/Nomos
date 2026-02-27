# Known Issues and Troubleshooting

## Current Known Issues

### Nomos Baseline Scheme
- **Search returns empty results**: Cryptographic operations need refinement
  - Likely issues in token generation or TSet/XSet construction
  - Check `Gatekeeper::GenToken()` and `Server::Search()` implementations

### Verifiable Scheme
- **QTree path verification fails**: Index calculation needs correction
  - Issue in `QTree::verifyPath()` or proof generation methods
  - May be related to bit indexing in the Merkle tree

### MC-ODXT
- **Implementation incomplete**: Currently placeholder only
  - Needs full protocol implementation
  - Requires multi-client coordination logic

### Memory Management
- **RELIC big numbers not fully released**: Potential memory leaks
  - Need to audit all `bn_new()` calls for corresponding `bn_free()`
  - Check elliptic curve point cleanup (`g1_free()`, `g2_free()`, `gt_free()`)

### Error Handling
- **Minimal error handling**: Lacks exception handling and boundary checks
  - No validation of input parameters in most functions
  - Missing bounds checking on array/vector accesses
  - No graceful handling of cryptographic operation failures

## Debugging Tips

### Enable Verbose Logging
Add debug prints in critical paths:
- Token generation in `Gatekeeper::GenToken()`
- Search operations in `Server::Search()`
- Update operations in `Client::Update()`

### Memory Leak Detection
```bash
# Use valgrind (Linux) or leaks (macOS)
valgrind --leak-check=full ./Nomos nomos
# or
leaks --atExit -- ./Nomos nomos
```

### Test Individual Components
Run specific test suites to isolate issues:
```bash
./tests/nomos_test --gtest_filter=GatekeeperTest.*
./tests/nomos_test --gtest_filter=QTreeTest.*
```

## Common Build Issues

### RELIC Not Found
```
CMake Error: Could not find RELIC
```
**Solution**: Install RELIC via Homebrew:
```bash
brew install relic
```

### OpenSSL Version Mismatch
**Solution**: Ensure OpenSSL 3.x is installed:
```bash
brew install openssl@3
```

### C++11 Compatibility
The project uses C++11 standard. Avoid using:
- `std::make_unique` (C++14)
- Structured bindings (C++17)
- `std::optional` (C++17)
