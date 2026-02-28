# CLAUDE.md

This file provides guidance to Claude Code when working with this repository.

## ⚠️ CRITICAL: Paper Reproduction Project

**This is an academic paper reproduction project. All implementations must strictly follow the paper specifications.**

**Primary Paper**: Bag et al. 2024 - "Tokenised Multi-client Provisioning for Dynamic Searchable Encryption with Forward and Backward Privacy" (ACM ASIA CCS 2024)

**Paper Location**: `/Users/cyan/code/paper/ref-thesis/Bag 等 - 2024 - Tokenised Multi-client Provisioning for Dynamic Searchable Encryption with Forward and Backward Priv.pdf`

**Mandatory Rule**: See `rules/论文复现规则.md` for strict reproduction constraints.

## Project Overview

Nomos is a C++11 research project implementing three searchable encryption schemes:
1. **Nomos** - Multi-user multi-keyword dynamic SSE baseline
2. **Verifiable Nomos** - Verifiable scheme using QTree + embedded commitments
3. **MC-ODXT** - Multi-client ODXT adaptation (to be implemented)

Uses: RELIC (elliptic curves), OpenSSL (hashing), GMP (big integers).

## Quick Commands

```bash
# Build
mkdir -p build && cd build && cmake .. && cmake --build .

# Run experiments
./Nomos nomos        # Baseline scheme
./Nomos verifiable   # Verifiable scheme

# Test
cmake .. -DBUILD_TESTING=ON && make && ./tests/nomos_test

# Format
make fmt
```

## Architecture Pointers

- **Experiment Framework**: `core/Experiment.hpp` (base), `core/ExperimentFactory.hpp` (factory), `main.cpp` (dispatcher)
- **Nomos Components**: `nomos/{GatekeeperCorrect,ServerCorrect,ClientCorrect}.{hpp,cpp}`, `nomos/types_correct.hpp`
- **Verifiable Components**: `verifiable/{QTree,AddressCommitment}.{hpp,cpp}`
- **Crypto Primitives**: `core/Primitive.{hpp,cpp}`

## Key Parameters

- ℓ = 3 (cross-tags per insertion)
- k = 2 (cross-tags sampled during query)
- λ = 256 bits (security parameter)
- C++11 standard

## Behavioral Rules

See `rules/` directory for:
- `编码流程.md` - Development workflow and Actor-Critic protocol
- `论文复现规则.md` - **MANDATORY** Paper reproduction constraints

## Heavy Documentation

See `docs/` directory for detailed documentation (load on-demand):
- `implementation-status.md` - **Current implementation status and progress**
- `paper-sources.md` - **Paper references and reproduction status**
- `scheme-comparison.md` - Scheme comparison and paper chapter mapping
- `parameter-deviations.md` - Documented deviations from paper
- `architecture.md` - Architecture deep-dives
- `crypto-protocols.md` - Cryptographic protocol specifications
- `build-system.md` - Build system details
- `known-issues.md` - Known issues and troubleshooting
- `quick-guide.md` - Quick start guide

## Code Style

- Google C++ style guide (`.clang-format`)
- C++11 standard
- Format before committing: `make fmt`
