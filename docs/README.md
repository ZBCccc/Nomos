# Documentation Directory

This directory is intentionally small. The goal is to keep a few documents as
the canonical sources of truth instead of spreading overlapping project notes
across many files.

## Current Set

- `implementation-status.md`
  Current scheme status, active code paths, experiment entry points, and major
  technical gaps.
- `architecture.md`
  System structure plus the practical engineering guide for build, test, run,
  and troubleshooting.
- `crypto-protocols.md`
  Cryptographic primitives and scheme-level protocol notes.
- `experiment-validation.md`
  Validation notes for the experimental results and measurement pipeline.
- `parameter-deviations.md`
  Required record of paper-vs-implementation parameter deviations.
- `paper-sources.md`
  Primary paper list and reproduction source map.
- `historical-design-archive.md`
  Archived design context that is no longer the active implementation guide.

## Consolidation Notes

The following topics were merged into the current set and no longer live in
separate standalone docs:

- current implementation boundaries
- scheme comparison and chapter mapping
- MC-ODXT token-generation refactoring notes
- build-system instructions
- troubleshooting / known-issues notes

## Usage

Use these files on-demand:

- Read `implementation-status.md` for the current runtime reality.
- Read `architecture.md` for how the codebase is organized and how to build it.
- Read `crypto-protocols.md` when checking protocol or primitive details.
- Read `historical-design-archive.md` only when you need superseded context.
