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
- C++17 compatible compiler (GCC, Clang, MSVC)

## Building the Project

### Linux/macOS

```bash
# Create build directory
mkdir build
cd build

# Configure the project
cmake ..

# Build the project
cmake --build .

# Run the executable
./bin/Nomos
```

### Windows

```bash
# Create build directory
mkdir build
cd build

# Configure the project
cmake ..

# Build the project
cmake --build . --config Release

# Run the executable
.\bin\Release\Nomos.exe
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
