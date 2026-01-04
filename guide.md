# Running Tests for Nomos

## Prerequisites
Ensure passing the following dependencies:
- **CMake** (3.14+)
- **GoogleTest** (Installed via Homebrew: `brew install googletest`)
- **OpenSSL**, **GMP**, **RELIC**

## Building

1. Create a build directory and configure the project:
   ```bash
   mkdir -p build && cd build
   cmake .. -DBUILD_TESTING=ON
   ```

2. Compile the project:
   ```bash
   make
   ```

## Running Tests

Run the test executable generated in `build/tests/`:

```bash
./tests/nomos_test
```

### Filtering Tests
To run specific tests (e.g., only CP-ABE tests):

```bash
./tests/nomos_test --gtest_filter=CpABETest.*
```
