# Godot Plugin Design

**Date:** 2026-03-27
**Goal:** Convert `godot-toolkit` into a proper Claude Code plugin named `godot` with a full suite of domain and process skills, a smart session-start hook, and a Godot-native code reviewer agent.

---

## Architecture

`godot-toolkit/` is the plugin root. Plugin name: `godot`. All skills are namespaced `godot:*` automatically by the plugin system.

The plugin has four component types:
- **Skills** — auto-activated by Claude based on task context
- **Agent** — `code-reviewer`, triggered after completing plan steps
- **Command** — `/godot`, manual entry point reload mid-session
- **Hook** — `SessionStart`, auto-injects `godot:godot` when a Godot project is detected

---

## Directory Layout

```
godot-toolkit/
├── .claude-plugin/
│   └── plugin.json
├── AgentBridge/                          ← unchanged C++ module source
├── commands/
│   └── godot.md                          ← /godot (manual entry reload)
├── agents/
│   └── code-reviewer.md                  ← godot:code-reviewer
├── hooks/
│   ├── hooks.json                        ← SessionStart hook
│   └── scripts/
│       ├── session-start.sh              ← Godot project detection + injection
│       └── session-start.bat             ← Windows equivalent
├── skills/
│   ├── godot/SKILL.md                    ← godot:godot (entry orientation)
│   ├── docs/SKILL.md                     ← godot:docs (Context7 integration)
│   ├── coder/SKILL.md                    ← godot:coder
│   ├── architect/SKILL.md                ← godot:architect
│   ├── debugger/SKILL.md                 ← godot:debugger
│   ├── runner/SKILL.md                   ← godot:runner
│   ├── scene/SKILL.md                    ← godot:scene
│   ├── shader/SKILL.md                   ← godot:shader
│   ├── signals/SKILL.md                  ← godot:signals
│   ├── ui/SKILL.md                       ← godot:ui
│   ├── brainstorming/SKILL.md
│   ├── writing-plans/SKILL.md
│   ├── executing-plans/SKILL.md
│   ├── subagent-driven-development/SKILL.md
│   ├── systematic-debugging/SKILL.md
│   ├── verification-before-completion/SKILL.md
│   ├── requesting-code-review/SKILL.md
│   ├── receiving-code-review/SKILL.md
│   ├── dispatching-parallel-agents/SKILL.md
│   ├── using-git-worktrees/SKILL.md
│   ├── finishing-a-development-branch/SKILL.md
│   └── writing-skills/SKILL.md
├── docs/
│   ├── specs/
│   └── plans/
├── install.sh
└── install.bat
```

---

## Plugin Manifest

`.claude-plugin/plugin.json`:
```json
{
  "name": "godot",
  "version": "1.0.0",
  "description": "Godot 4.7 development skills, agents, and workflow tools for Claude Code."
}
```

---

## Session-Start Hook

### Detection Logic

`hooks/scripts/session-start.sh` checks for `project.godot` up to 2 directory levels deep. If found, outputs the full `godot:godot` skill content to stdout, which the hook system injects as session context.

```bash
#!/usr/bin/env bash
if find . -maxdepth 2 -name "project.godot" 2>/dev/null | grep -q .; then
    cat "$CLAUDE_PLUGIN_ROOT/skills/godot/SKILL.md"
fi
```

`session-start.bat` performs the same check using `dir /s /b /a-d project.godot`.

### Hook Configuration

`hooks/hooks.json`:
```json
{
  "SessionStart": [{
    "hooks": [{
      "type": "command",
      "command": "bash ${CLAUDE_PLUGIN_ROOT}/hooks/scripts/session-start.sh"
    }]
  }]
}
```

---

## Skill Inventory

### Domain Skills

| Skill | Purpose | Key Content |
|---|---|---|
| `godot:godot` | Entry orientation | Lists all skills with one-line "use when" triggers; lists AgentBridge MCP tools |
| `godot:docs` | Context7 API lookup | Resolve Godot library ID → query docs → cite version |
| `godot:coder` | GDScript 4.x authoring | Typed vars, `@export`, `@onready`, signals, `await`; + Context7 footer |
| `godot:architect` | Project structure | Autoloads, scene composition, resource patterns; + Context7 footer |
| `godot:debugger` | Live debugging | `get_log` → `execute` diagnostic → fix loop using AgentBridge |
| `godot:runner` | Scene execution | `save_all` → `play_scene` → wait → `stop_scene` → `get_log` |
| `godot:scene` | Scene tree manipulation | `execute` patterns for add/find/remove nodes |
| `godot:shader` | Shading language | spatial/canvas_item/particle templates; + Context7 footer |
| `godot:signals` | Signal architecture | Declaration, connection, event bus patterns |
| `godot:ui` | Control nodes & themes | Anchors, containers, `Theme` resources; + Context7 footer |

Context7 footer (added to `coder`, `architect`, `shader`, `ui`):
```
## Docs
When API is uncertain, use godot:docs (Context7) before writing code.
```

### Process Skills

All process skills are fully self-contained — no external skill dependencies.

| Skill | Purpose |
|---|---|
| `godot:brainstorming` | Design Godot features before implementation: scene layout, GDScript structure, AgentBridge validation strategy. One question at a time. Produce a written spec before writing code. |
| `godot:writing-plans` | Write step-by-step implementation plans with Godot file maps, AgentBridge checkpoints (`execute`/`get_log` after each step), and GDScript task breakdowns. |
| `godot:executing-plans` | Execute a written plan checkpoint by checkpoint. After each step: `save_all` → `execute` smoke-test → `get_log`. Do not advance until current step is verified. |
| `godot:subagent-driven-development` | Identify independent Godot tasks, dispatch parallel subagents with full context (scene path, node names, AgentBridge availability), coordinate results. |
| `godot:systematic-debugging` | Structured: reproduce → `get_log` → isolate with `execute` → form hypothesis → fix → verify with `get_log`. Do not guess. |
| `godot:verification-before-completion` | Before claiming done: `save_all` → `play_scene` → wait 2s → `stop_scene` → `get_log(max_lines=200)`. Zero errors required. |
| `godot:requesting-code-review` | After completing a plan step, invoke `godot:code-reviewer` agent. Provide: what was built, which plan step, relevant file paths. |
| `godot:receiving-code-review` | When receiving review feedback: verify each claim with `execute` or file inspection before acting. Do not implement suggestions blindly. |
| `godot:dispatching-parallel-agents` | Identify subtasks with no shared state. Launch agents with: task description, scene context, AgentBridge availability, output expectations. |
| `godot:using-git-worktrees` | Create worktrees for Godot feature branches. Note: AgentBridge module symlink must be re-established in each worktree via `install.sh`. |
| `godot:finishing-a-development-branch` | Verify (`godot:verification-before-completion`) → review (`godot:requesting-code-review`) → merge or PR. |
| `godot:writing-skills` | Create new `godot:*` skills. Follow plugin conventions: `skills/<name>/SKILL.md`, frontmatter with `name`/`description`. |

---

## Agent

### `godot:code-reviewer` (`agents/code-reviewer.md`)

Triggered after completing a plan step. Reviews:
- **GDScript idioms**: typed vars, `@export`, `@onready`, signal patterns, `await` usage
- **Scene structure**: node ownership, `set_owner`, scene independence (runnable with F6)
- **AgentBridge usage**: correct tool calls (`execute`, `get_log`, `save_all`), error checking
- **Performance**: no per-frame allocations, proper `Timer` usage, no `OS.get_ticks_msec()`

Always runs `get_log` to check for live errors before issuing verdict.

Output format: **Critical** (must fix) / **Important** (should fix) / **Suggestions** (nice to have).

---

## Command

### `/godot` (`commands/godot.md`)

Manual mid-session reload of `godot:godot` orientation content. Used when starting a new task in an existing session without restarting.

---

## Install Script Changes

Both `install.sh` and `install.bat` gain a third step:

```bash
# install.sh addition
echo ""
echo "[godot plugin] Installing Claude Code plugin..."
if command -v claude &>/dev/null; then
    claude plugin install "$SCRIPT_DIR" --scope user
    echo "[godot plugin] Plugin installed."
else
    echo "WARNING: 'claude' not found. Run: claude plugin install $SCRIPT_DIR --scope user"
fi

echo ""
echo "[godot plugin] Done. Next steps:"
echo "  1. Rebuild Godot: cd D:/Projects/godot && scons platform=windows target=editor -j32"
echo "  2. Open demo-host: D:/GodotProjects/demo-host"
echo "  3. Verify: claude plugin list && claude mcp list"
```

Existing AgentBridge module linking and MCP registration steps are unchanged.

---

## Migration from Current Skills

Existing `skills/godot-*/skill.md` files are:
1. Moved to `skills/<name>/SKILL.md` (directory renamed, file capitalized)
2. Frontmatter `name` field updated (strip `godot-` prefix)
3. Context7 footer added to `coder`, `architect`, `shader`, `ui`
4. Old directories deleted after migration
