# Nomos Current State

## Scope

This repository currently maintains three experiment-facing schemes:

| Scheme | Runtime status | Entry point | Search path |
|---|---|---|---|
| `Nomos` | ✅ Running | `./Nomos nomos-simplified` | `Client::genToken -> Gatekeeper::genToken -> Client::prepareSearch -> Server::search -> Client::decryptResults` |
| `MC-ODXT` | ✅ Running | `./Nomos mc-odxt` | `McOdxtClient::genToken -> McOdxtGatekeeper::genToken -> McOdxtClient::prepareSearch -> McOdxtServer::search -> McOdxtClient::decryptResults` |
| `VQNomos` | ✅ Running | `./Nomos vq-nomos` / `./Nomos verifiable` | `VQNomosClient::genToken -> VQNomosGatekeeper::genToken -> VQNomosClient::prepareSearch -> VQNomosServer::search -> VQNomosClient::decryptAndVerify` |

## Paper Mapping

| Paper / chapter role | Implementation status | Main code |
|---|---|---|
| Nomos baseline | Active experiment implementation | `include/nomos/*`, `src/nomos/*` |
| VQNomos / verifiable extension | Active experiment implementation | `include/vq-nomos/*`, `src/vq-nomos/*` |
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

Main runtime files:

- `include/vq-nomos/Client.hpp`
- `include/vq-nomos/Gatekeeper.hpp`
- `include/vq-nomos/Server.hpp`
- `include/vq-nomos/types.hpp`
- `include/vq-nomos/VQNomosExperiment.hpp`
- `src/vq-nomos/Client.cpp`
- `src/vq-nomos/Gatekeeper.cpp`
- `src/vq-nomos/Server.cpp`
- `src/vq-nomos/Common.cpp`

Current runtime path:

1. `VQNomosClient::genToken(query, updateCnt)`
2. `VQNomosGatekeeper::genToken(tokenRequest)`
3. `VQNomosClient::prepareSearch(token, tokenRequest)`
4. `VQNomosServer::search(request, token)`
5. `VQNomosClient::decryptAndVerify(response, token, tokenRequest)`

Implemented verification layers:

- signed version anchor over the current `QTree` root
- signed Merkle-open root per `(keyword, entry)` relation
- selective opening for the sampled `beta` positions
- QTree positive / negative witnesses (bit value sourced from committed QTree state, not exact XSet)
- client-side semantic recomputation and result-set equality check

> **Note (2026-03-16)**: Relation verdict requires both QTree all-1 witnesses *and* a valid Merkle auth
> opening. A QTree leaf set only by hash collision (no matching XSet entry) yields no Merkle auth;
> the client treats it as a verifiable non-match rather than a protocol error.
> See deviation item 12 in `docs/parameter-deviations.md`.

## Experiment Entry Points

Current CLI entry points:

- `./Nomos nomos-simplified`
- `./Nomos mc-odxt`
- `./Nomos verifiable`
- `./Nomos vq-nomos`
- `./Nomos benchmark`
- `./Nomos chapter4-client-search-fixed-w1`

Main experiment / benchmark drivers:

- `src/nomos/NomosSimplifiedExperiment.cpp`
- `src/mc-odxt/McOdxtExperiment.cpp`
- `src/benchmark/NomosBenchmark.cpp`
- `src/benchmark/ClientSearchFixedW1Experiment.cpp`

## Current Gaps

### P0

1. Keep experiment timing boundaries consistent across schemes.
2. Continue pruning stale historical notes when implementation changes land.
3. Harden VQNomos benchmarks beyond the current prototype-level storage estimates.

### P1

1. Unify experiment output formats and downstream plotting assumptions.
2. Tighten documentation around what is measured vs estimated in Chapter 4.
3. Continue hardening validation around client-side keyword reordering.

## Practical Summary

- `Nomos`, `MC-ODXT`, and `VQNomos` are all real runnable experiment paths.
- `implementation-status.md` is the canonical overview; avoid reviving older
  per-scheme snapshot docs.
