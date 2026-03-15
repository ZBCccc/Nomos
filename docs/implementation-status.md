# Nomos Current State

## Scope

This repository currently maintains three experiment-facing schemes:

| Scheme | Runtime status | Entry point | Search path |
|---|---|---|---|
| `Nomos` | ✅ Running | `./Nomos nomos-simplified` | `Client::genToken -> Gatekeeper::genToken -> Client::prepareSearch -> Server::search -> Client::decryptResults` |
| `MC-ODXT` | ✅ Running | `./Nomos mc-odxt` | `McOdxtClient::genToken -> McOdxtGatekeeper::genToken -> McOdxtClient::prepareSearch -> McOdxtServer::search -> McOdxtClient::decryptResults` |
| `VQNomos` | ⚠️ Partial | `./Nomos verifiable` | Component-level verification pieces exist, but the full protocol path is not yet wired end-to-end |

## Paper Mapping

| Paper / chapter role | Implementation status | Main code |
|---|---|---|
| Nomos baseline | Active experiment implementation | `include/nomos/*`, `src/nomos/*` |
| VQNomos / verifiable extension | Components implemented, full integration pending | `include/verifiable/*`, `src/verifiable/*` |
| MC-ODXT comparison scheme | Active comparison prototype | `include/mc-odxt/*`, `src/mc-odxt/*` |

## Active Code Paths

### Nomos

Main runtime files:

- `include/nomos/Client.hpp`
- `include/nomos/Gatekeeper.hpp`
- `include/nomos/Server.hpp`
- `include/nomos/types.hpp`
- `src/nomos/Client.cpp`
- `src/nomos/Gatekeeper.cpp`
- `src/nomos/Server.cpp`

Current experimental token flow:

1. `Client::genToken(query, updateCnt)`
2. `Gatekeeper::genToken(tokenRequest)`
3. `Client::prepareSearch(token, tokenRequest)`
4. `Server::search(request)`
5. `Client::decryptResults(results, token)`

Important properties:

- The query is reordered on the client so the least-frequently updated term is
  placed at `query_keywords[0]`.
- `TokenRequest` is the explicit hand-off object between client and gatekeeper.
- The simplified runtime path omits the paper's OPRF blinding, but preserves the
  hash / PRF / elliptic-curve derivation structure.
- Nomos still keeps RBF sampling when deriving `bxtrap`.

### MC-ODXT

Main runtime files:

- `include/mc-odxt/McOdxtClient.hpp`
- `include/mc-odxt/McOdxtGatekeeper.hpp`
- `include/mc-odxt/McOdxtServer.hpp`
- `include/mc-odxt/McOdxtTypes.hpp`
- `src/mc-odxt/McOdxtClient.cpp`
- `src/mc-odxt/McOdxtGatekeeper.cpp`
- `src/mc-odxt/McOdxtServer.cpp`

MC-ODXT now follows the same explicit two-step token-generation interface as
Nomos:

1. `McOdxtClient::genToken(query, updateCnt)`
2. `McOdxtGatekeeper::genToken(tokenRequest)`
3. `McOdxtClient::prepareSearch(token, tokenRequest)`

Protocol differences from Nomos:

- no RBF expansion
- one xtag per update
- direct cross-keyword matching rather than sampled RBF positions

### VQNomos

Implemented pieces:

- `QTree`
- `AddressCommitment`
- experiment-side overhead estimation used by Chapter 4 benchmarks

Not yet completed:

- a fully wired `Nomos Update/Search -> Verifiable Update/Search` path
- a concrete `VerifiableScheme.cpp` protocol implementation matching the
  baseline runtime structure

## Experiment Entry Points

Current CLI entry points:

- `./Nomos nomos-simplified`
- `./Nomos mc-odxt`
- `./Nomos verifiable`
- `./Nomos benchmark`
- `./Nomos comparative-benchmark`
- `./Nomos chapter4-client-search-fixed-w1`

Main experiment / benchmark drivers:

- `src/nomos/NomosSimplifiedExperiment.cpp`
- `src/mc-odxt/McOdxtExperiment.cpp`
- `src/benchmark/NomosBenchmark.cpp`
- `src/benchmark/ComparativeBenchmark.cpp`
- `src/benchmark/ClientSearchFixedW1Experiment.cpp`

## Current Gaps

### P0

1. Finish the end-to-end verifiable Nomos protocol path.
2. Keep experiment timing boundaries consistent across schemes.
3. Continue pruning stale historical notes when implementation changes land.

### P1

1. Unify experiment output formats and downstream plotting assumptions.
2. Tighten documentation around what is measured vs estimated in Chapter 4.
3. Continue hardening validation around client-side keyword reordering.

## Practical Summary

- `Nomos` and `MC-ODXT` are both real runnable experiment paths.
- `VQNomos` is still component-complete but protocol-incomplete.
- `implementation-status.md` is the canonical overview; avoid reviving older
  per-scheme snapshot docs.
