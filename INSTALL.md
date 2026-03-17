# Installation Guide - Nomos Project

This guide provides step-by-step instructions for installing all dependencies and building the Nomos project from scratch on a new server.

## Table of Contents

- [System Requirements](#system-requirements)
- [Platform-Specific Installation](#platform-specific-installation)
  - [macOS](#macos)
  - [Ubuntu/Debian Linux](#ubuntudebian-linux)
  - [CentOS/RHEL/Rocky Linux](#centosrhelrocky-linux)
- [Building the Project](#building-the-project)
- [Running Tests](#running-tests)
- [Troubleshooting](#troubleshooting)

---

## System Requirements

- **C++ Compiler**: GCC 4.8+ or Clang 3.4+ (C++11 support required)
- **CMake**: Version 3.14 or higher
- **Memory**: At least 4GB RAM recommended
- **Disk Space**: ~2GB for dependencies and build artifacts

---

## Platform-Specific Installation

### macOS

#### Prerequisites

Install Homebrew if not already installed:
```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

#### Install Dependencies

```bash
# Update Homebrew
brew update

# Install build tools
brew install cmake

# Install required libraries
brew install openssl@3
brew install gmp
brew install relic

# Verify installations
cmake --version        # Should be >= 3.14
openssl version        # Should show OpenSSL 3.x
```

#### Environment Setup

For Apple Silicon (M1/M2/M3):
```bash
export HOMEBREW_PREFIX="/opt/homebrew"
export PATH="$HOMEBREW_PREFIX/bin:$PATH"
```

For Intel Macs:
```bash
export HOMEBREW_PREFIX="/usr/local"
export PATH="$HOMEBREW_PREFIX/bin:$PATH"
```

Add to `~/.zshrc` or `~/.bash_profile` for persistence.

---

### Ubuntu/Debian Linux

#### Update System

```bash
sudo apt update
sudo apt upgrade -y
```

#### Install Build Tools

```bash
sudo apt install -y build-essential cmake git
sudo apt install -y pkg-config
```

#### Install OpenSSL

```bash
sudo apt install -y libssl-dev
```

#### Install GMP

```bash
sudo apt install -y libgmp-dev
```

#### Install RELIC

RELIC is not available in standard repositories, so build from source.

**Option 1: Clone from GitHub (if accessible)**

```bash
# Install RELIC dependencies
sudo apt install -y libgmp-dev

# Clone RELIC repository
cd /tmp
git clone https://github.com/relic-toolkit/relic.git
cd relic

# Configure and build
mkdir build && cd build
cmake -DALLOC=AUTO -DWSIZE=64 -DRAND=UDEV -DSHLIB=ON \
      -DSTLIB=ON -DSTBIN=OFF -DTIMER=CYCLE -DCHECK=on \
      -DVERBS=on -DARITH=x64-asm-254 -DFP_PRIME=254 \
      -DFP_METHD="INTEG;INTEG;INTEG;MONTY;LOWER;SLIDE" \
      -DCOMP="-O3 -funroll-loops -fomit-frame-pointer -finline-small-functions -march=native -mtune=native" \
      -DFP_PMERS=off -DFP_QNRES=on -DFPX_METHD="INTEG;INTEG;LAZYR" \
      -DEP_PLAIN=off -DEP_SUPER=off -DEP_ENDOM=off -DEP_MIXED=on \
      -DEP_PRECO=on -DPP_METHD="LAZYR;OATEP" ..

make -j$(nproc)
sudo make install

# Update library cache
sudo ldconfig

# Verify installation
ls /usr/local/lib/librelic* || ls /usr/lib/librelic*
```

##### Option 3: Download Release Tarball (no git required)

```bash
# Install RELIC dependencies
sudo apt install -y libgmp-dev wget

# Download latest release tarball
cd /tmp
wget https://github.com/relic-toolkit/relic/archive/refs/tags/0.6.0.tar.gz
tar -xzf 0.6.0.tar.gz
cd relic-0.6.0

# Build and install
mkdir build && cd build
cmake -DALLOC=AUTO -DWSIZE=64 -DRAND=UDEV -DSHLIB=ON \
      -DSTLIB=ON -DSTBIN=OFF -DTIMER=CYCLE -DCHECK=on \
      -DVERBS=on -DARITH=x64-asm-254 -DFP_PRIME=254 \
      -DFP_METHD="INTEG;INTEG;INTEG;MONTY;LOWER;SLIDE" \
      -DCOMP="-O3 -funroll-loops -fomit-frame-pointer -finline-small-functions -march=native -mtune=native" \
      -DFP_PMERS=off -DFP_QNRES=on -DFPX_METHD="INTEG;INTEG;LAZYR" \
      -DEP_PLAIN=off -DEP_SUPER=off -DEP_ENDOM=off -DEP_MIXED=on \
      -DEP_PRECO=on -DPP_METHD="LAZYR;OATEP" ..

make -j$(nproc)
sudo make install
sudo ldconfig
```

##### Option 4: Transfer Pre-built RELIC from Another Machine

If you have RELIC installed on another machine with the same OS/architecture:

```bash
# On the machine with RELIC installed:
cd /tmp
tar -czf relic-prebuilt.tar.gz \
    /usr/local/include/relic* \
    /usr/local/lib/librelic* \
    /usr/local/lib/pkgconfig/relic.pc

# Transfer to target server (via scp, USB, etc.)
scp relic-prebuilt.tar.gz user@target-server:/tmp/

# On target server:
cd /
sudo tar -xzf /tmp/relic-prebuilt.tar.gz
sudo ldconfig

# Verify
ls /usr/local/lib/librelic*
```

#### Verify Installations

```bash
cmake --version        # Should be >= 3.14
gcc --version          # Should support C++11
openssl version        # Should show OpenSSL 1.1.1+ or 3.x
```

---

### CentOS/RHEL/Rocky Linux

#### Enable EPEL Repository (for CentOS 7/8)

```bash
sudo yum install -y epel-release
sudo yum update -y
```

#### Install Build Tools

```bash
# For CentOS 7
sudo yum install -y centos-release-scl
sudo yum install -y devtoolset-8-gcc devtoolset-8-gcc-c++
scl enable devtoolset-8 bash  # Enable GCC 8

# For CentOS 8/Rocky Linux
sudo dnf install -y gcc gcc-c++ make
```

#### Install CMake

```bash
# CentOS 7 (CMake 3.14+ not in repos, install from source)
cd /tmp
wget https://github.com/Kitware/CMake/releases/download/v3.25.0/cmake-3.25.0-linux-x86_64.sh
sudo sh cmake-3.25.0-linux-x86_64.sh --prefix=/usr/local --skip-license

# CentOS 8/Rocky Linux
sudo dnf install -y cmake
```

#### Install Dependencies

```bash
# OpenSSL
sudo yum install -y openssl-devel

# GMP
sudo yum install -y gmp-devel

# Git
sudo yum install -y git
```

#### Install RELIC (from source)

##### Option 1: Clone from GitHub (if accessible)

```bash
cd /tmp
git clone https://github.com/relic-toolkit/relic.git
cd relic
mkdir build && cd build

cmake -DALLOC=AUTO -DWSIZE=64 -DRAND=UDEV -DSHLIB=ON \
      -DSTLIB=ON -DSTBIN=OFF -DTIMER=CYCLE -DCHECK=on \
      -DVERBS=on -DARITH=x64-asm-254 -DFP_PRIME=254 \
      -DFP_METHD="INTEG;INTEG;INTEG;MONTY;LOWER;SLIDE" \
      -DCOMP="-O3 -funroll-loops -fomit-frame-pointer -finline-small-functions -march=native -mtune=native" \
      -DFP_PMERS=off -DFP_QNRES=on -DFPX_METHD="INTEG;INTEG;LAZYR" \
      -DEP_PLAIN=off -DEP_SUPER=off -DEP_ENDOM=off -DEP_MIXED=on \
      -DEP_PRECO=on -DPP_METHD="LAZYR;OATEP" ..

make -j$(nproc)
sudo make install
sudo ldconfig
```

##### Option 2: Clone from Gitee Mirror

```bash
cd /tmp
git clone https://gitee.com/mirrors/relic.git
cd relic
# Then follow the same build steps as Option 1
```

##### Option 3: Download Release Tarball

```bash
cd /tmp
wget https://github.com/relic-toolkit/relic/archive/refs/tags/0.6.0.tar.gz
tar -xzf 0.6.0.tar.gz
cd relic-0.6.0
# Then follow the same build steps as Option 1
```

**Note**: See Ubuntu section above for more alternative methods (proxy, pre-built transfer, etc.)

---

## Building the Project

### Clone the Repository

```bash
# Via SSH (requires SSH key setup)
git clone git@gitee.com:zhang-baichuanqaq/Nomos.git
cd Nomos

# Or via HTTPS
git clone https://gitee.com/zhang-baichuanqaq/Nomos.git
cd Nomos
```

### Build Steps

```bash
# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake ..

# Build the project (use all available cores)
cmake --build . -j$(nproc)

# Or use make directly
make -j$(nproc)
```

### Build Output

After successful build, you should see:
- `./Nomos` - Main executable
- `./tests/nomos_test` - Test executable (if BUILD_TESTING=ON)

---

## Running Tests

### Build with Testing Enabled

```bash
cd build
cmake .. -DBUILD_TESTING=ON
make -j$(nproc)
```

### Run All Tests

```bash
./tests/nomos_test
```

### Run Specific Test Suite

```bash
./tests/nomos_test --gtest_filter=NomosSimplified.*
```

### Expected Output

All tests should pass:
```
[==========] Running X tests from Y test suites.
...
[  PASSED  ] X tests.
```

---

## Running Experiments

### Basic Experiments

```bash
cd build

# Nomos baseline
./Nomos nomos-simplified

# Verifiable scheme
./Nomos verifiable

# MC-ODXT
./Nomos mc-odxt
```

### Benchmark Experiments

```bash
# Chapter 4 experiments
./Nomos chapter4-client-search-fixed-w1 --dataset all --output-dir ../results/ch4
```

---

## Troubleshooting

### RELIC Not Found

**Error**: `RELIC not found`

**Solution**:
```bash
# Check if RELIC is installed
ls /usr/local/lib/librelic* || ls /opt/homebrew/lib/librelic*

# If not found, specify custom path
cmake -DRELIC_INCLUDE_DIR=/path/to/relic/include \
      -DRELIC_LIBRARY=/path/to/relic/lib/librelic.so ..
```

### GMP Not Found

**Error**: `GMP not found`

**Solution**:
```bash
# Ubuntu/Debian
sudo apt install -y libgmp-dev

# CentOS/RHEL
sudo yum install -y gmp-devel

# macOS
brew install gmp
```

### OpenSSL Version Issues

**Error**: `OpenSSL version too old`

**Solution**:
```bash
# Ubuntu/Debian
sudo apt install -y libssl-dev

# macOS
brew install openssl@3
export PKG_CONFIG_PATH="/opt/homebrew/opt/openssl@3/lib/pkgconfig"
```

### CMake Version Too Old

**Error**: `CMake 3.14 or higher is required`

**Solution**:
```bash
# Ubuntu/Debian (add Kitware APT repository)
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main'
sudo apt update
sudo apt install -y cmake

# Or install from source (all platforms)
cd /tmp
wget https://github.com/Kitware/CMake/releases/download/v3.25.0/cmake-3.25.0-linux-x86_64.sh
sudo sh cmake-3.25.0-linux-x86_64.sh --prefix=/usr/local --skip-license
```

### Linker Errors on macOS

**Error**: `dyld: Library not loaded: @rpath/librelic.dylib`

**Solution**:
```bash
# Add RELIC library path to DYLD_LIBRARY_PATH
export DYLD_LIBRARY_PATH="/opt/homebrew/lib:$DYLD_LIBRARY_PATH"

# Or rebuild with proper RPATH
cd build
rm -rf *
cmake .. -DCMAKE_INSTALL_RPATH="/opt/homebrew/lib"
make -j$(nproc)
```

### C++11 Compilation Errors

**Error**: `error: 'auto' type specifier is a C++11 extension`

**Solution**:
```bash
# Ensure GCC 4.8+ or Clang 3.4+
gcc --version

# On CentOS 7, enable devtoolset
sudo yum install -y centos-release-scl devtoolset-8
scl enable devtoolset-8 bash
```

---

## Verification Checklist

After installation, verify everything works:

- [ ] CMake version >= 3.14
- [ ] C++ compiler supports C++11
- [ ] OpenSSL installed and found by CMake
- [ ] GMP installed and found by CMake
- [ ] RELIC installed and found by CMake
- [ ] Project builds without errors
- [ ] All tests pass (if BUILD_TESTING=ON)
- [ ] `./Nomos nomos-simplified` runs successfully

---

## Additional Resources

- **Project Documentation**: See `CLAUDE.md` for architecture details
- **Experiment Guide**: See `实验运行指南-ClientSearchFixedW1.md` for experiment instructions
- **Paper Reference**: Bag et al. 2024 (ACM ASIA CCS 2024)

---

**Last Updated**: 2026-03-17
**Tested Platforms**: macOS (Intel/Apple Silicon), Ubuntu 20.04/22.04, CentOS 7/8, Rocky Linux 8
