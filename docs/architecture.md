# Architecture and Engineering Guide

## System Layout

### Experiment Framework

- `core/Experiment.hpp`
  Base interface for `setup() / run() / teardown() / getName()`.
- `core/ExperimentFactory.hpp`
  Registry for CLI-selectable experiments.
- `src/main.cpp`
  Registers all experiment names and dispatches them from the command line.

### Nomos Runtime Roles

- `Gatekeeper`
  Owns master keys, update counters, update-token generation, and the
  gatekeeper-side `genToken`.
- `Client`
  Reorders the query, builds `TokenRequest`, prepares the search request, and
  decrypts results.
- `Server`
  Stores `TSet` / `XSet` and performs candidate enumeration plus cross-tag
  filtering.

### MC-ODXT Runtime Roles

The role split mirrors Nomos:

- `McOdxtGatekeeper`
- `McOdxtClient`
- `McOdxtServer`

The main protocol difference is that MC-ODXT omits RBF expansion and uses
single-position cross-keyword matching.

### Verifiable Components

- `verifiable/QTree.{hpp,cpp}`
  Merkle-hash tree used for XSet-style verification.
- `verifiable/AddressCommitment.{hpp,cpp}`
  Address commitment and subset-check helpers.

These components are implemented and tested, but the full protocol-level
integration is still pending.

### Shared Crypto Layer

`core/Primitive.{hpp,cpp}` provides:

- `Hash_H1 / Hash_H2 / Hash_G1 / Hash_G2`
- `Hash_Zn`
- `F`
- `F_p`

## Build and Run

### Configure

```bash
cd /Users/cyan/code/paper/Nomos
cmake -S . -B build -DBUILD_TESTING=ON
```

### Build

```bash
cmake --build build
```

### Run

```bash
cd /Users/cyan/code/paper/Nomos/build

./Nomos nomos-simplified
./Nomos mc-odxt
./Nomos verifiable
./Nomos benchmark
./Nomos chapter4-client-search-fixed-w1
```

### Test

```bash
cd /Users/cyan/code/paper/Nomos/build

./tests/nomos_test
./tests/nomos_test '--gtest_filter=NomosSimplifiedTest.*'
./tests/nomos_test '--gtest_filter=McOdxtTest.*'
./tests/relic_test_app
```

### Format

```bash
cd /Users/cyan/code/paper/Nomos/build

cmake --build . --target format
cmake --build . --target check-format
```

## Troubleshooting

### RELIC / OpenSSL / GMP Not Found

This project expects Homebrew-style installs on macOS. Re-run CMake after
installing dependencies so the cache picks up the correct paths.

### Search Results Unexpectedly Empty

Check the full runtime chain in order:

1. `Client::genToken` or `McOdxtClient::genToken`
2. `Gatekeeper::genToken` or `McOdxtGatekeeper::genToken`
3. `Client::prepareSearch`
4. `Server::search`

The most common logic issues in the current codebase are:

- using stale build artifacts after an interface change
- passing the wrong token-request object into `prepareSearch`
- benchmarking code measuring only one half of the explicit token path

### After Interface Changes, Rebuild Everything

When `Client / Gatekeeper / TokenRequest` signatures change, use a full rebuild:

```bash
cd /Users/cyan/code/paper/Nomos
cmake --build build --clean-first
```

### Memory / Resource Checks

Useful commands:

```bash
valgrind --leak-check=full ./build/Nomos nomos-simplified
leaks --atExit -- ./build/Nomos nomos-simplified
```

## Dependency Notes

- RELIC: elliptic-curve operations
- OpenSSL: hashing and randomness
- GMP: big integers
- GoogleTest: test runner

The project targets C++11. Avoid introducing C++14+ language features unless
the repository standard changes.
