# Nomos

A C++ project framework.

## Project Structure

```
Nomos/
├── CMakeLists.txt      # CMake build configuration
├── README.md           # This file
├── include/            # Header files
│   └── nomos.h
├── src/                # Source files
│   └── main.cpp
└── tests/              # Unit tests
```

## Requirements

- CMake 3.10 or higher
- C++11 compatible compiler (GCC, Clang, MSVC)

## Building the Project

This project uses CMakePresets.json for platform-specific compiler configuration:

- **Windows**: MinGW compiler
- **Linux/macOS**: Clang compiler

### Linux/macOS (with Clang)

```bash
# Create build directory and configure
mkdir build
cd build
cmake --preset default-unix ..

# Build the project
cmake --build .

# Run the executable
./bin/Nomos
```

### Windows (with MinGW)

```bash
# Create build directory and configure
mkdir build
cd build
cmake --preset default-windows ..

# Build the project
cmake --build .

# Run the executable
.\bin\Nomos.exe
```

### Manual Preset Selection

You can also use the presets directly:

```bash
# List available presets
cmake --list-presets

# Configure with specific preset
cmake --preset default-windows ..   # For Windows
cmake --preset default-unix ..      # For Linux/macOS
```

## Development

### Adding New Source Files

1. Add your source files to the `src/` directory
2. Add your header files to the `include/` directory
3. Update `CMakeLists.txt` to include new source files in the `SOURCES` variable

### Compiler Warnings

The project is configured with high warning levels:

- GCC/Clang: `-Wall -Wextra -Wpedantic`
- MSVC: `/W4`

## License

This project is open source.
