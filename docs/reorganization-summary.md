# Claude Settings Reorganization Summary

## Three-Tier Structure

### Tier 1: CLAUDE.md (Always Loaded, <200 lines)
**Location**: `/CLAUDE.md`
**Purpose**: Entry point with pointers to other resources
**Content**:
- Project overview (1 paragraph)
- Quick command reference
- Architecture pointers (file paths only)
- Key parameters
- Pointers to rules/ and docs/

**Line count**: ~80 lines (well under 200 limit)

### Tier 2: rules/ (Auto-loaded, Behavioral Rules)
**Location**: `/rules/`
**Purpose**: Mandatory workflows and behavioral constraints
**Files**:
- `编码流程.md` - Actor-Critic development workflow
- `README.md` - Rules directory guide

**When to add here**:
- Development workflows
- Code review protocols
- Debugging procedures
- Git commit rules
- Error catching patterns

### Tier 3: docs/ (On-Demand, Heavy Documentation)
**Location**: `/docs/`
**Purpose**: Detailed technical documentation loaded only when needed
**Files**:
- `architecture.md` - Component details, data structures
- `build-system.md` - Build instructions, testing, formatting
- `crypto-protocols.md` - Protocol specifications, security assumptions
- `known-issues.md` - Bugs, troubleshooting, debugging tips
- `README.md` - Docs directory guide

**When to add here**:
- Detailed architecture explanations
- Protocol specifications
- API documentation
- Troubleshooting guides
- Performance tuning guides

## Migration Summary

### What was moved from old CLAUDE.md:

**To docs/architecture.md**:
- Experiment framework details
- Nomos component descriptions
- Verifiable scheme component details
- Cryptographic primitives details
- Dependency information

**To docs/build-system.md**:
- Initial setup instructions
- Building procedures
- Running experiments
- Testing procedures
- Code formatting commands
- Platform-specific notes

**To docs/crypto-protocols.md**:
- Protocol parameters (ℓ, k, λ)
- Setup/Update/Search protocol details
- Verification protocol
- Hash function specifications
- Security assumptions

**To docs/known-issues.md**:
- Current bugs and issues
- Debugging tips
- Common build problems

**Kept in CLAUDE.md**:
- High-level project overview
- Quick command reference
- Architecture pointers (paths only)
- Key parameters (summary)
- Pointers to rules/ and docs/

**Already in rules/**:
- `编码流程.md` - Actor-Critic workflow (unchanged)

## Benefits

1. **Faster context loading**: CLAUDE.md is now ~80 lines vs. 143 lines
2. **Better organization**: Clear separation between pointers, rules, and documentation
3. **On-demand loading**: Heavy docs only loaded when needed
4. **Easier maintenance**: Update docs without touching CLAUDE.md
5. **Scalability**: Can add more docs without bloating CLAUDE.md

## Usage Examples

**For quick reference**:
```
Read CLAUDE.md → Get commands and pointers
```

**For implementation work**:
```
Read CLAUDE.md → Follow rules/编码流程.md → Load docs/architecture.md if needed
```

**For debugging**:
```
Read CLAUDE.md → Check docs/known-issues.md → Load docs/architecture.md for details
```

**For protocol questions**:
```
Read CLAUDE.md → Load docs/crypto-protocols.md
```
