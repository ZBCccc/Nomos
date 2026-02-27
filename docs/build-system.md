# Build System Documentation

## Initial Setup

### Using CMake Directly
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

## Running Experiments

```bash
# From build directory
./Nomos nomos        # Run Nomos baseline scheme
./Nomos verifiable   # Run verifiable scheme
./Nomos mc-odxt      # Run MC-ODXT (not yet implemented)
```

## Testing

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

## Code Formatting

```bash
# Format all source files (requires clang-format)
cd build
cmake --build . --target format

# Check formatting without modifying files
cmake --build . --target check-format

# Or from project root
make fmt
```

## Platform-Specific Notes

### macOS (Homebrew)
The CMakeLists.txt automatically handles Homebrew paths for both Intel and Apple Silicon Macs:
- Intel: `/usr/local/opt/`
- Apple Silicon: `/opt/homebrew/opt/`

Dependencies installed via Homebrew:
- RELIC
- OpenSSL
- GMP
