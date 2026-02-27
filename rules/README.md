# Rules Directory

This directory contains behavioral rules and workflows that Claude Code should follow when working on this project. These files are automatically loaded into context.

## Files

- **编码流程.md** - Development workflow using Actor-Critic architecture with Codex integration
  - Phase 0: Pre-implementation debate (blueprint validation)
  - Phase 1: Internal validation (self-review + testing)
  - Phase 2: Final Boss audit (Codex review)
  - Phase 3: Handoff summary + auto-commit

- **论文复现规则.md** - **MANDATORY** Paper reproduction constraints
  - Protocol fidelity requirements
  - Parameter consistency checks
  - Algorithm mapping standards
  - Security guarantee preservation
  - Deviation documentation requirements

## Usage

These rules are MANDATORY and automatically enforced. They define:
- **Paper reproduction constraints** (CRITICAL - must follow paper specifications)
- When to use which AI agents (Opus planner, Sonnet implementer/debugger/reviewer)
- How to validate code before declaring tasks complete
- When to escalate to the user vs. proceeding autonomously
- Commit message formatting and git workflow

## Adding New Rules

When adding new rule files, follow this structure:
- Keep rules concise and actionable
- Use clear section headers
- Define trigger conditions and exit conditions
- Specify escalation paths
