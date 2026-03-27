# Godot Plugin Implementation Plan

> **For agentic workers:** Execute tasks in order using checkboxes to track progress. Use `godot:executing-plans` once it exists (Task 7), or execute inline until then.

**Goal:** Convert `godot-toolkit` into a Claude Code plugin named `godot` with 22 skills, 1 agent, 1 command, and a smart `SessionStart` hook that auto-activates in Godot projects.

**Architecture:** Monorepo plugin — `godot-toolkit/` becomes the plugin root. Plugin manifest at `.claude-plugin/plugin.json`. Existing 8 domain skills migrated and renamed. 13 new process skills added. `SessionStart` hook detects `project.godot` and injects the entry skill. Install scripts updated to register plugin at user scope.

**Tech Stack:** Claude Code plugin system (plugin.json, SKILL.md, agents, commands, hooks), Bash (session detection hook), Godot 4.7 (AgentBridge MCP tools: execute, get_log, save_all, play_scene, stop_scene), Context7 MCP.

---

## File Map

**Created:**
- `.claude-plugin/plugin.json` — plugin manifest
- `skills/godot/SKILL.md` — entry orientation skill
- `skills/docs/SKILL.md` — Context7 docs skill
- `skills/brainstorming/SKILL.md`
- `skills/writing-plans/SKILL.md`
- `skills/executing-plans/SKILL.md`
- `skills/subagent-driven-development/SKILL.md`
- `skills/systematic-debugging/SKILL.md`
- `skills/verification-before-completion/SKILL.md`
- `skills/requesting-code-review/SKILL.md`
- `skills/receiving-code-review/SKILL.md`
- `skills/dispatching-parallel-agents/SKILL.md`
- `skills/using-git-worktrees/SKILL.md`
- `skills/finishing-a-development-branch/SKILL.md`
- `skills/writing-skills/SKILL.md`
- `agents/code-reviewer.md`
- `commands/godot.md`
- `hooks/hooks.json`
- `hooks/scripts/session-start.sh`
- `hooks/scripts/session-start.bat`

**Migrated (rename + capitalize):**
- `skills/godot-architect/skill.md` → `skills/architect/SKILL.md`
- `skills/godot-coder/skill.md` → `skills/coder/SKILL.md`
- `skills/godot-debugger/skill.md` → `skills/debugger/SKILL.md`
- `skills/godot-runner/skill.md` → `skills/runner/SKILL.md`
- `skills/godot-scene/skill.md` → `skills/scene/SKILL.md`
- `skills/godot-shader/skill.md` → `skills/shader/SKILL.md`
- `skills/godot-signals/skill.md` → `skills/signals/SKILL.md`
- `skills/godot-ui/skill.md` → `skills/ui/SKILL.md`

**Docs migration:**
- `docs/superpowers/plans/` → `docs/plans/`
- `docs/superpowers/specs/` → `docs/specs/`

**Modified:**
- `install.sh` — add plugin install step
- `install.bat` — add plugin install step

---

## Task 1: Plugin Manifest

**Files:**
- Create: `.claude-plugin/plugin.json`

- [ ] Create the manifest:

```json
{
  "name": "godot",
  "version": "1.0.0",
  "description": "Godot 4.7 development skills, agents, and workflow tools for Claude Code."
}
```

Write to `D:/Projects/godot-toolkit/.claude-plugin/plugin.json`.

- [ ] Verify the file exists:

```bash
cat D:/Projects/godot-toolkit/.claude-plugin/plugin.json
```

Expected: JSON with `"name": "godot"`.

- [ ] Commit:

```bash
cd D:/Projects/godot-toolkit
git add .claude-plugin/plugin.json
git commit -m "add plugin manifest"
```

---

## Task 2: Docs Migration

**Files:**
- Move: `docs/superpowers/plans/2026-03-27-godot-agentbridge.md` → `docs/plans/2026-03-27-godot-agentbridge.md`
- Move: `docs/superpowers/specs/2026-03-27-godot-agentbridge-design.md` → `docs/specs/2026-03-27-godot-agentbridge-design.md`
- Remove: `docs/superpowers/`

- [ ] Move plan file:

```bash
cd D:/Projects/godot-toolkit
git mv docs/superpowers/plans/2026-03-27-godot-agentbridge.md docs/plans/2026-03-27-godot-agentbridge.md
```

- [ ] Move spec file:

```bash
git mv docs/superpowers/specs/2026-03-27-godot-agentbridge-design.md docs/specs/2026-03-27-godot-agentbridge-design.md
```

- [ ] Remove the now-empty superpowers directory:

```bash
git rm -r docs/superpowers/ 2>/dev/null || true
rmdir docs/superpowers/plans docs/superpowers/specs docs/superpowers 2>/dev/null || true
```

- [ ] Verify:

```bash
ls docs/plans/ && ls docs/specs/
```

Expected: both plan files present in their new locations; `docs/superpowers/` gone.

- [ ] Commit:

```bash
git add -A
git commit -m "move docs out of superpowers subdir"
```

---

## Task 3: Migrate Domain Skills

**Files:**
- Create: `skills/architect/SKILL.md`, `skills/coder/SKILL.md`, `skills/debugger/SKILL.md`, `skills/runner/SKILL.md`, `skills/scene/SKILL.md`, `skills/shader/SKILL.md`, `skills/signals/SKILL.md`, `skills/ui/SKILL.md`
- Delete: `skills/godot-architect/`, `skills/godot-coder/`, `skills/godot-debugger/`, `skills/godot-runner/`, `skills/godot-scene/`, `skills/godot-shader/`, `skills/godot-signals/`, `skills/godot-ui/`

For each skill: create new directory, write `SKILL.md` with updated frontmatter (strip `godot-` prefix from `name`), add Context7 footer to `coder`, `architect`, `shader`, `ui`.

- [ ] Migrate `godot-architect` → `architect`:

```bash
mkdir -p D:/Projects/godot-toolkit/skills/architect
```

Write `D:/Projects/godot-toolkit/skills/architect/SKILL.md` with this content (copy from `skills/godot-architect/skill.md`, update frontmatter name to `architect`, add Context7 footer):

```markdown
---
name: architect
description: Design Godot project structure — use when designing autoloads, resource architecture, scene organization, or plugin layout.
type: guidance
---

# Godot Architect

## Project Structure Principles

- **Autoloads (singletons):** Use for global state only — game manager, settings, event bus. Keep them thin.
- **Scenes as components:** Each scene should be independently runnable (F6).
- **Resources for data:** Use `Resource` subclasses for config/data, not plain Dictionaries.
- **Signals over references:** Nodes should communicate up via signals, down via method calls.

## Autoload Pattern

```
res://
├── autoloads/
│   ├── GameManager.gd     (class_name GameManager)
│   ├── EventBus.gd        (class_name EventBus — signals only)
│   └── Settings.gd        (class_name Settings)
├── scenes/
│   ├── ui/
│   ├── gameplay/
│   └── world/
├── resources/
│   └── data/
└── scripts/
```

## Resource Pattern

```gdscript
# data/enemy_data.gd
class_name EnemyData extends Resource

@export var max_health: int = 100
@export var speed: float = 3.0
@export var attack_damage: int = 10
```

## Scene Ownership Rule

Every node added at runtime must have its `owner` set:
```gdscript
var node = MeshInstance3D.new()
root.add_child(node, true)
node.owner = root  # required for serialization
```

## Plugin Layout

```
addons/my_plugin/
├── plugin.cfg
├── plugin.gd        (EditorPlugin subclass)
└── src/
```

## Docs

When API is uncertain, use godot:docs (Context7) before writing code.
```

- [ ] Migrate `godot-coder` → `coder`:

```bash
mkdir -p D:/Projects/godot-toolkit/skills/coder
```

Write `D:/Projects/godot-toolkit/skills/coder/SKILL.md`:

```markdown
---
name: coder
description: Write idiomatic GDScript 4.x for Godot 4.7 — use when writing any GDScript code, autoloads, or class definitions.
type: implementation
---

# Godot Coder

You are writing GDScript for **Godot 4.7**. Follow these rules:

## GDScript 4.x Rules

- Use `class_name MyClass` at the top of reusable classes.
- Use `@export` for inspector-visible variables.
- Use `@onready var node = $NodePath` for node references — never assign in `_init`.
- Use typed variables: `var speed: float = 5.0` not `var speed = 5.0`.
- Use `super()` not `super._ready()` for overrides — use `super._ready()` only when the parent method name differs.
- Use `%NodeName` (scene-unique names) for key child nodes.
- Signal declarations: `signal my_signal(arg: Type)`.
- Connect signals in code: `my_node.my_signal.connect(_on_my_signal)`.
- Avoid `setget` — use `@export` with `set(value)` getter/setter syntax in Godot 4.
- `await` replaces `yield`.
- `PackedScene` instantiation: `scene.instantiate()`.
- No `OS.get_ticks_msec()` for timers — use `Timer` nodes or `get_tree().create_timer()`.

## Tool Usage

Use the `execute` MCP tool to test snippets in the live editor before finalizing code:
```
execute(code='print(Vector3.UP.rotated(Vector3.RIGHT, PI/4))')
```

Use `get_log` to check for errors after running code.

## Docs

When API is uncertain, use godot:docs (Context7) before writing code.
```

- [ ] Migrate `godot-debugger` → `debugger`:

```bash
mkdir -p D:/Projects/godot-toolkit/skills/debugger
```

Write `D:/Projects/godot-toolkit/skills/debugger/SKILL.md`:

```markdown
---
name: debugger
description: Debug Godot issues using AgentBridge — use when encountering errors, unexpected behavior, or runtime crashes.
type: process
---

# Godot Debugger

## Workflow

1. **Get recent log** — always start here:
   ```
   get_log(max_lines=200)
   ```

2. **Execute a diagnostic** — narrow down the problem:
   ```
   execute(code='''
   var root = EditorInterface.get_edited_scene_root()
   if root:
       print("Root: ", root.name, " children: ", root.get_child_count())
   else:
       print("No scene open")
   ''')
   ```

3. **Inspect a specific node path:**
   ```
   execute(code='''
   var root = EditorInterface.get_edited_scene_root()
   var node = root.get_node_or_null("Player/CollisionShape3D")
   print("Found: ", node != null)
   if node:
       print("Type: ", node.get_class())
   ''')
   ```

4. **Check a script method exists:**
   ```
   execute(code='''
   var root = EditorInterface.get_edited_scene_root()
   var player = root.get_node_or_null("Player")
   print("Has take_damage: ", player.has_method("take_damage"))
   ''')
   ```

## Rules

- Never change code without first reading the log
- One change per iteration — never shotgun-fix
- Always verify with `get_log` after each fix
- If stuck after 3 iterations, re-read the relevant API docs via godot:docs
```

- [ ] Migrate `godot-runner` → `runner`:

```bash
mkdir -p D:/Projects/godot-toolkit/skills/runner
```

Write `D:/Projects/godot-toolkit/skills/runner/SKILL.md`:

```markdown
---
name: runner
description: Run, test, and profile Godot scenes using the AgentBridge MCP tools — use when you need to execute, observe, or benchmark a scene.
type: process
---

# Godot Runner

## Run a scene and capture output

```
1. save_all()                          — ensure changes are saved
2. play_scene(scene="res://my.tscn")  — start playing
3. [wait 2–3 seconds for scene to run]
4. stop_scene()                        — stop
5. get_log(max_lines=200)             — inspect output
```

## Execute code and check result

```
execute(code='''
print(ProjectSettings.get_setting("application/config/name"))
print(OS.get_name())
print(Engine.get_version_info())
''')
```

## Common diagnostic executes

```gdscript
# Check scene tree
var root = EditorInterface.get_edited_scene_root()
print(root.name, " — ", root.get_child_count(), " children")

# List all nodes
func list_nodes(node: Node, indent: int = 0) -> void:
    print(" ".repeat(indent), node.name, " (", node.get_class(), ")")
    for child in node.get_children():
        list_nodes(child, indent + 2)
list_nodes(EditorInterface.get_edited_scene_root())
```

## Profile timing

```gdscript
var start = Time.get_ticks_usec()
# ... code to profile ...
print("Elapsed: ", Time.get_ticks_usec() - start, " us")
```
```

- [ ] Migrate `godot-scene` → `scene`:

```bash
mkdir -p D:/Projects/godot-toolkit/skills/scene
```

Write `D:/Projects/godot-toolkit/skills/scene/SKILL.md`:

```markdown
---
name: scene
description: Manipulate Godot scene trees and nodes via the AgentBridge execute tool — use when adding, removing, or inspecting nodes programmatically.
type: implementation
---

# Godot Scene Manipulation

Use the `execute` MCP tool to manipulate the scene tree in the live Godot editor.

## Common Patterns

**Get the edited scene root:**
```gdscript
var root = EditorInterface.get_edited_scene_root()
print(root.name)
```

**Add a node to the scene:**
```gdscript
var root = EditorInterface.get_edited_scene_root()
var node = MeshInstance3D.new()
node.name = "MyMesh"
root.add_child(node, true)
node.owner = root  # required for serialization
```

**Find a node by name:**
```gdscript
var root = EditorInterface.get_edited_scene_root()
var player = root.get_node_or_null("Player")
if player:
    print("Found: ", player.get_class())
```

**Remove a node:**
```gdscript
var root = EditorInterface.get_edited_scene_root()
var node = root.get_node_or_null("OldNode")
if node:
    node.queue_free()
```

**Set a node property:**
```gdscript
var root = EditorInterface.get_edited_scene_root()
var mesh: MeshInstance3D = root.get_node("MyMesh")
mesh.position = Vector3(0, 1, 0)
```

**Save after changes:**
```
save_all()
```
Always call `save_all()` after scene manipulation to persist changes.
```

- [ ] Migrate `godot-shader` → `shader`:

```bash
mkdir -p D:/Projects/godot-toolkit/skills/shader
```

Write `D:/Projects/godot-toolkit/skills/shader/SKILL.md`:

```markdown
---
name: shader
description: Write Godot Shading Language shaders — use when creating spatial, canvas_item, or particle shaders.
type: implementation
---

# Godot Shader

## Shader Types

- `shader_type spatial;` — 3D objects
- `shader_type canvas_item;` — 2D sprites, UI
- `shader_type particles;` — GPU particles

## Spatial Shader Template

```glsl
shader_type spatial;

uniform vec4 albedo : source_color = vec4(1.0);
uniform float roughness : hint_range(0.0, 1.0) = 0.5;

void fragment() {
    ALBEDO = albedo.rgb;
    ROUGHNESS = roughness;
}
```

## Canvas Item Template

```glsl
shader_type canvas_item;

uniform float speed : hint_range(0.0, 10.0) = 1.0;

void fragment() {
    vec2 uv = UV;
    uv.x += TIME * speed * 0.1;
    COLOR = texture(TEXTURE, uv);
}
```

## Particle Shader Template

```glsl
shader_type particles;

void process() {
    VELOCITY.y -= 9.8 * DELTA;
    TRANSFORM.origin += VELOCITY * DELTA;
}
```

## Uniforms

- `hint_range(min, max)` — slider in inspector
- `source_color` — color picker with sRGB
- `hint_default_white` / `hint_default_black` — texture defaults

## Docs

When GLSL built-ins or Godot shader globals are uncertain, use godot:docs (Context7) before writing code.
```

- [ ] Migrate `godot-signals` → `signals`:

```bash
mkdir -p D:/Projects/godot-toolkit/skills/signals
```

Write `D:/Projects/godot-toolkit/skills/signals/SKILL.md`:

```markdown
---
name: signals
description: Design and implement Godot signal-driven architecture — use when connecting nodes, designing event systems, or debugging missing connections.
type: implementation
---

# Godot Signals

## Rules

- Signals flow **up** the tree (child → parent). Method calls flow **down**.
- Never store a reference to a sibling — use signals or an autoload event bus.
- Disconnect in `_exit_tree` if you connected in `_enter_tree` (not needed for `@onready` + `connect` in `_ready`).

## Declaration and Connection

```gdscript
# emitter.gd
signal health_changed(new_health: int)

func take_damage(amount: int) -> void:
    health -= amount
    health_changed.emit(health)
```

```gdscript
# receiver.gd
@onready var emitter: Emitter = $Emitter

func _ready() -> void:
    emitter.health_changed.connect(_on_health_changed)

func _on_health_changed(new_health: int) -> void:
    print("Health now: ", new_health)
```

## Event Bus Pattern

```gdscript
# autoloads/EventBus.gd
class_name EventBus extends Node

signal enemy_died(enemy_id: int)
signal level_completed(level: int)
```

```gdscript
# Any node emitting:
EventBus.enemy_died.emit(enemy.id)

# Any node receiving (in _ready):
EventBus.enemy_died.connect(_on_enemy_died)
```

## Verify connections via execute

```gdscript
var node = EditorInterface.get_edited_scene_root().get_node("Player")
var connections = node.get_signal_list()
for sig in connections:
    print(sig.name, ": ", node.get_signal_connection_list(sig.name))
```
```

- [ ] Migrate `godot-ui` → `ui`:

```bash
mkdir -p D:/Projects/godot-toolkit/skills/ui
```

Write `D:/Projects/godot-toolkit/skills/ui/SKILL.md`:

```markdown
---
name: ui
description: Build Godot UI with Control nodes, themes, and anchors — use when creating menus, HUDs, or any CanvasLayer UI.
type: implementation
---

# Godot UI

## Key Principles

- Use `Control` as base for all UI. `CanvasLayer` for HUDs that overlay the 3D world.
- Set anchors via `set_anchor_and_offset` or the Layout menu in editor.
- Use `Theme` resources to style multiple controls at once — never style individual controls in code.
- `MarginContainer` + `VBoxContainer`/`HBoxContainer` for most layouts.
- `%NodeName` unique names for controls you reference in scripts.

## Common Patterns

**Anchoring a panel to bottom-right:**
```gdscript
var panel: Panel = $Panel
panel.set_anchor_and_offset(SIDE_LEFT, 1.0, -200)
panel.set_anchor_and_offset(SIDE_TOP, 1.0, -100)
panel.set_anchor_and_offset(SIDE_RIGHT, 1.0, 0)
panel.set_anchor_and_offset(SIDE_BOTTOM, 1.0, 0)
```

**Connecting a Button signal:**
```gdscript
%MyButton.pressed.connect(_on_my_button_pressed)
```

**Showing/hiding UI:**
```gdscript
%HUD.visible = true
%PauseMenu.visible = false
```

**Theme application:**
```gdscript
var theme = preload("res://resources/ui_theme.tres")
%MyPanel.theme = theme
```

## CanvasLayer for HUD

```gdscript
# hud.gd
extends CanvasLayer

@onready var health_label: Label = %HealthLabel

func update_health(value: int) -> void:
    health_label.text = "HP: %d" % value
```

## Docs

When Control node properties or theme overrides are uncertain, use godot:docs (Context7) before writing code.
```

- [ ] Delete old skill directories:

```bash
cd D:/Projects/godot-toolkit
git rm -r skills/godot-architect skills/godot-coder skills/godot-debugger skills/godot-runner skills/godot-scene skills/godot-shader skills/godot-signals skills/godot-ui
```

- [ ] Verify new skill layout:

```bash
ls D:/Projects/godot-toolkit/skills/
```

Expected: `architect  coder  debugger  runner  scene  shader  signals  ui`

- [ ] Commit:

```bash
cd D:/Projects/godot-toolkit
git add -A
git commit -m "migrate domain skills to plugin layout"
```

---

## Task 4: godot:godot Entry Skill

**Files:**
- Create: `skills/godot/SKILL.md`

- [ ] Create directory and write file:

```bash
mkdir -p D:/Projects/godot-toolkit/skills/godot
```

Write `D:/Projects/godot-toolkit/skills/godot/SKILL.md`:

```markdown
---
name: godot
description: Godot 4.7 development entry point — lists all available godot:* skills and when to use each. Auto-injected when a project.godot file is detected.
---

# Godot Development Context

You are working on a **Godot 4.7** project with the **AgentBridge** module active.

## AgentBridge MCP Tools

| Tool | Purpose |
|---|---|
| `execute(code)` | Run GDScript in the live editor |
| `get_log(max_lines)` | Read editor output log |
| `save_all()` | Save all open scenes and scripts |
| `play_scene(scene)` | Start playing a scene |
| `stop_scene()` | Stop the playing scene |

## Domain Skills — When to Use Each

| Skill | Use when |
|---|---|
| `godot:coder` | Writing any GDScript — classes, autoloads, methods |
| `godot:architect` | Designing scene hierarchy, autoload structure, resource patterns |
| `godot:debugger` | Encountering errors, crashes, or unexpected behavior |
| `godot:runner` | Running, testing, or profiling scenes |
| `godot:scene` | Adding, removing, or inspecting nodes programmatically |
| `godot:shader` | Writing spatial, canvas_item, or particle shaders |
| `godot:signals` | Designing event systems or connecting signals |
| `godot:ui` | Building Control-based UI, HUDs, menus |
| `godot:docs` | Looking up Godot 4.x API docs via Context7 |

## Process Skills — When to Use Each

| Skill | Use when |
|---|---|
| `godot:brainstorming` | Before implementing any new feature |
| `godot:writing-plans` | After brainstorming, before writing code |
| `godot:executing-plans` | Executing a written implementation plan |
| `godot:subagent-driven-development` | Tasks have independent subtasks for parallel work |
| `godot:systematic-debugging` | Debugging — always use instead of guessing |
| `godot:verification-before-completion` | Before claiming any work is done |
| `godot:requesting-code-review` | After completing a plan step |
| `godot:receiving-code-review` | When processing code review feedback |
| `godot:dispatching-parallel-agents` | Launching parallel subagents for independent work |
| `godot:using-git-worktrees` | Starting feature work needing isolation |
| `godot:finishing-a-development-branch` | When implementation is complete |
| `godot:writing-skills` | Creating new godot:* skills |
```

- [ ] Verify file:

```bash
head -5 D:/Projects/godot-toolkit/skills/godot/SKILL.md
```

Expected: frontmatter with `name: godot`.

- [ ] Commit:

```bash
cd D:/Projects/godot-toolkit
git add skills/godot/SKILL.md
git commit -m "add godot:godot entry skill"
```

---

## Task 5: godot:docs Skill

**Files:**
- Create: `skills/docs/SKILL.md`

- [ ] Create and write:

```bash
mkdir -p D:/Projects/godot-toolkit/skills/docs
```

Write `D:/Projects/godot-toolkit/skills/docs/SKILL.md`:

```markdown
---
name: docs
description: Look up Godot 4.x API documentation via Context7 — use when API behavior is uncertain before writing code.
---

# Godot Docs

Use Context7 to retrieve up-to-date Godot 4.x documentation before implementing unfamiliar APIs.

## Workflow

1. **Resolve the library ID:**

```
mcp__plugin_context7_context7__resolve-library-id(libraryName="godot")
```

2. **Query for the specific API:**

```
mcp__plugin_context7_context7__query-docs(
  context7CompatibleLibraryId="<id from step 1>",
  query="Timer class autostart one_shot",
  tokens=3000
)
```

3. **Cite the version** from the returned docs before using the API.

## Common Queries

| Topic | Query string |
|---|---|
| Node lifecycle | `"_ready _process _physics_process lifecycle order"` |
| Signal patterns | `"signal emit connect callable"` |
| Resource loading | `"load preload ResourceLoader"` |
| Scene instantiation | `"PackedScene instantiate add_child"` |
| Physics | `"CharacterBody3D move_and_slide collision"` |
| Timer | `"Timer autostart one_shot timeout"` |
| Tweens | `"Tween create_tween PropertyTweener"` |
| Input | `"Input is_action_pressed get_axis"` |
| Shader globals | `"shader built-in TIME FRAGCOORD UV"` |
| UI anchors | `"Control anchor offset set_anchor_and_offset"` |

## When to Use

- Before implementing an API you haven't used in Godot 4.x
- When a method name or signature is uncertain
- When debugging and the error references an unknown built-in
- When the coder/shader/ui/architect skill's example doesn't cover your case
```

- [ ] Commit:

```bash
cd D:/Projects/godot-toolkit
git add skills/docs/SKILL.md
git commit -m "add godot:docs skill"
```

---

## Task 6: godot:brainstorming Skill

**Files:**
- Create: `skills/brainstorming/SKILL.md`

- [ ] Create and write:

```bash
mkdir -p D:/Projects/godot-toolkit/skills/brainstorming
```

Write `D:/Projects/godot-toolkit/skills/brainstorming/SKILL.md`:

```markdown
---
name: brainstorming
description: Design Godot features before touching code — use before implementing any new scene, system, or GDScript class.
---

# Godot Brainstorming

Use before implementing any new feature. Produce a written spec before writing code.

## Hard Gate

Do NOT write any GDScript or modify any `.tscn` until the spec is written and approved.

## Checklist

- [ ] Understand what is being built (ask one clarifying question at a time)
- [ ] Identify which scenes are involved
- [ ] Identify which GDScript classes or autoloads are needed
- [ ] Identify AgentBridge checkpoints for live validation
- [ ] Propose 2–3 approaches with trade-offs
- [ ] Present the design and get approval
- [ ] Write spec to `docs/specs/YYYY-MM-DD-<feature>.md`
- [ ] Invoke `godot:writing-plans`

## Questions to Ask (one at a time)

1. What does this feature do for the player or editor?
2. Which existing scenes does it touch?
3. Does it need a new autoload, or can it live in an existing node?
4. How will it be tested via AgentBridge (`execute`/`get_log`/`play_scene`)?

## Approaches

Always propose 2–3 options with:
- How it fits the existing scene hierarchy
- What new nodes or scripts are required
- AgentBridge validation strategy

## Spec Format

Save to `docs/specs/YYYY-MM-DD-<feature>.md`:

```markdown
# <Feature> Design

**Goal:** <one sentence>
**Scenes:** <list of .tscn files involved>
**New scripts:** <list of .gd files to create>
**Signals:** <signals to add or connect>
**AgentBridge validation:** <how to verify it works live>
```

## After Spec Approval

Invoke `godot:writing-plans` to produce the implementation plan.
```

- [ ] Commit:

```bash
cd D:/Projects/godot-toolkit
git add skills/brainstorming/SKILL.md
git commit -m "add godot:brainstorming skill"
```

---

## Task 7: godot:writing-plans Skill

**Files:**
- Create: `skills/writing-plans/SKILL.md`

- [ ] Create and write:

```bash
mkdir -p D:/Projects/godot-toolkit/skills/writing-plans
```

Write `D:/Projects/godot-toolkit/skills/writing-plans/SKILL.md`:

```markdown
---
name: writing-plans
description: Write a step-by-step Godot implementation plan — use after brainstorming produces an approved spec, before writing any code.
---

# Godot Writing Plans

Write an implementation plan from the approved spec. Every task must include exact file paths, complete GDScript, and AgentBridge verification steps.

## Plan Header

Every plan starts with:

```markdown
# <Feature> Implementation Plan

**Goal:** <one sentence>
**Scenes:** <list of .tscn files>
**Scripts:** <list of .gd files>
**AgentBridge:** execute / get_log / save_all / play_scene / stop_scene

---
```

## Task Structure

```markdown
### Task N: <Name>

**Files:**
- Create/Modify: `res://path/to/file.gd`

- [ ] Write the complete GDScript (show full file content, no placeholders)
- [ ] `save_all()`
- [ ] Smoke-test: `execute(code='print(<node>.get_class())')` → `get_log(max_lines=50)`
- [ ] Commit: `git commit -m "feat: <description>"`
```

## Rules

- Every task ends with `save_all()` + `get_log()` verification
- Every task ends with a git commit
- No "TBD", "TODO", or "similar to above" — write the actual code
- One task per script file, one task per scene modification
- Show complete file content, not diffs

## AgentBridge Checkpoint Pattern

After every code task:
```
save_all()
execute(code='''
var root = EditorInterface.get_edited_scene_root()
# verify the thing you just added exists
print(root.get_node_or_null("YourNode") != null)
''')
get_log(max_lines=50)
```

## Save Plan To

`docs/plans/YYYY-MM-DD-<feature>.md`

## After Writing

Invoke `godot:executing-plans` to begin implementation.
```

- [ ] Commit:

```bash
cd D:/Projects/godot-toolkit
git add skills/writing-plans/SKILL.md
git commit -m "add godot:writing-plans skill"
```

---

## Task 8: godot:executing-plans Skill

**Files:**
- Create: `skills/executing-plans/SKILL.md`

- [ ] Create and write:

```bash
mkdir -p D:/Projects/godot-toolkit/skills/executing-plans
```

Write `D:/Projects/godot-toolkit/skills/executing-plans/SKILL.md`:

```markdown
---
name: executing-plans
description: Execute a written Godot implementation plan step by step — use when you have a plan document to implement.
---

# Godot Executing Plans

Execute the plan one checkbox at a time. Do not skip steps. Do not advance until the current step is verified.

## Checkpoint Protocol

After every task:

```
save_all()
execute(code='<smoke test relevant to this task>')
get_log(max_lines=100)
```

If `get_log` shows `ERROR:` or `SCRIPT ERROR:` lines: **STOP**. Fix before continuing. Do not mark the task complete.

## Before Each Commit

Run `godot:verification-before-completion`:
```
save_all()
play_scene(scene="res://path/to/affected_scene.tscn")
# wait 2 seconds
stop_scene()
get_log(max_lines=200)
```
Zero errors required before committing.

## If Blocked

- Log the issue as a comment on the plan checkpoint
- Invoke `godot:systematic-debugging`
- Do not attempt more than 3 different fixes without pausing to reassess the approach

## Tracking

Mark each checkbox only after:
- [ ] Code written
- [ ] `save_all()` called
- [ ] `get_log()` shows zero errors
- [ ] Committed

## After All Tasks Complete

Invoke `godot:finishing-a-development-branch`.
```

- [ ] Commit:

```bash
cd D:/Projects/godot-toolkit
git add skills/executing-plans/SKILL.md
git commit -m "add godot:executing-plans skill"
```

---

## Task 9: godot:subagent-driven-development Skill

**Files:**
- Create: `skills/subagent-driven-development/SKILL.md`

- [ ] Create and write:

```bash
mkdir -p D:/Projects/godot-toolkit/skills/subagent-driven-development
```

Write `D:/Projects/godot-toolkit/skills/subagent-driven-development/SKILL.md`:

```markdown
---
name: subagent-driven-development
description: Dispatch parallel subagents for independent Godot tasks — use when a plan has tasks with no shared files or sequential dependency.
---

# Godot Subagent-Driven Development

Identify which plan tasks can run in parallel (no shared `.gd` files, no shared `.tscn` scenes, no sequential signal dependencies), then dispatch subagents.

## Context to Provide Each Subagent

Every dispatched subagent must receive:
- Scene path(s) they are working on
- Node names they will touch
- Whether AgentBridge is available (yes, via `godot-bridge` MCP)
- Which `godot:*` skills to use
- Expected output (file created, node added, signal connected, etc.)

## Dispatch Template

```
Implement Task N from the plan:

Scene: res://scenes/<scene>.tscn
Script: res://scripts/<script>.gd
Nodes involved: <NodeName1>, <NodeName2>
AgentBridge: available — use execute/get_log/save_all via godot-bridge MCP
Skills to use: godot:coder, godot:verification-before-completion
Success criteria: <exact expected state — e.g., "get_log shows zero errors, node X exists in scene">
```

## Independence Check

Tasks are independent when they touch:
- Different `.gd` files
- Different `.tscn` scenes
- No autoloads being mutated by both

Tasks are NOT independent when:
- One creates a class the other imports
- Both modify the same scene
- One adds a signal the other connects to

## After All Agents Complete

1. `get_log(max_lines=200)` — check for conflicts
2. Open each modified scene, verify node structure with `execute`
3. Invoke `godot:verification-before-completion` on the full project
4. Invoke `godot:requesting-code-review`
```

- [ ] Commit:

```bash
cd D:/Projects/godot-toolkit
git add skills/subagent-driven-development/SKILL.md
git commit -m "add godot:subagent-driven-development skill"
```

---

## Task 10: godot:systematic-debugging Skill

**Files:**
- Create: `skills/systematic-debugging/SKILL.md`

- [ ] Create and write:

```bash
mkdir -p D:/Projects/godot-toolkit/skills/systematic-debugging
```

Write `D:/Projects/godot-toolkit/skills/systematic-debugging/SKILL.md`:

```markdown
---
name: systematic-debugging
description: Debug Godot issues systematically using AgentBridge — use instead of guessing whenever an error, crash, or unexpected behavior occurs.
---

# Godot Systematic Debugging

Never guess. Follow this protocol exactly.

## Step 1: Get the log

```
get_log(max_lines=200)
```

Read every line. Find the **first** error, not just the last. The root cause is usually the first one.

## Step 2: Reproduce with execute

```
execute(code='''
var root = EditorInterface.get_edited_scene_root()
if root == null:
    print("ERROR: no scene open")
    return
print("Root: ", root.name)
# reproduce the minimal condition that triggers the error
''')
get_log(max_lines=50)
```

## Step 3: Isolate

Narrow to the smallest failing unit:
- Comment out code paths
- Test individual methods with `execute`
- Check if the error exists without your changes: `git stash` → `get_log`

## Step 4: Form a hypothesis

Write one sentence: "The error occurs because X."

Do not proceed to Step 5 without a hypothesis.

## Step 5: Fix

Make exactly one change. No shotgun changes.

## Step 6: Verify

```
save_all()
execute(code='<reproduction from Step 2>')
get_log(max_lines=100)
```

Zero errors = fix confirmed. If errors remain, return to Step 1.

## Rules

- Never make more than one change per iteration
- Never skip an error by commenting it out
- After 3 failed iterations: run `godot:docs` on the relevant API before trying again
```

- [ ] Commit:

```bash
cd D:/Projects/godot-toolkit
git add skills/systematic-debugging/SKILL.md
git commit -m "add godot:systematic-debugging skill"
```

---

## Task 11: godot:verification-before-completion Skill

**Files:**
- Create: `skills/verification-before-completion/SKILL.md`

- [ ] Create and write:

```bash
mkdir -p D:/Projects/godot-toolkit/skills/verification-before-completion
```

Write `D:/Projects/godot-toolkit/skills/verification-before-completion/SKILL.md`:

```markdown
---
name: verification-before-completion
description: Verify Godot work is complete and error-free before claiming done or committing — use before every git commit.
---

# Godot Verification Before Completion

Run this before claiming any work is done. Zero errors required.

## Checklist

- [ ] `save_all()` — save all scenes and scripts
- [ ] `play_scene(scene="res://path/to/affected_scene.tscn")` — run the affected scene
- [ ] Wait 2 seconds for the scene to initialize
- [ ] `stop_scene()` — stop playback
- [ ] `get_log(max_lines=200)` — read the full log

## Pass Criteria

- Zero `ERROR:` lines
- Zero `SCRIPT ERROR:` lines
- Zero `SHADER ERROR:` lines
- Expected `print()` output present (if any)

## If Errors Found

Do NOT commit. Invoke `godot:systematic-debugging`.

## After Passing

```bash
git add <changed files>
git commit -m "<type>: <description>"
```
```

- [ ] Commit:

```bash
cd D:/Projects/godot-toolkit
git add skills/verification-before-completion/SKILL.md
git commit -m "add godot:verification-before-completion skill"
```

---

## Task 12: godot:requesting-code-review + godot:receiving-code-review Skills

**Files:**
- Create: `skills/requesting-code-review/SKILL.md`
- Create: `skills/receiving-code-review/SKILL.md`

- [ ] Create requesting-code-review:

```bash
mkdir -p D:/Projects/godot-toolkit/skills/requesting-code-review
```

Write `D:/Projects/godot-toolkit/skills/requesting-code-review/SKILL.md`:

```markdown
---
name: requesting-code-review
description: Request a Godot code review after completing a plan step — use after finishing a task and verifying it before moving to the next task.
---

# Godot Requesting Code Review

After completing a plan task and verifying it (godot:verification-before-completion), invoke the code reviewer agent.

## Checklist

- [ ] Verification passed (zero errors in get_log)
- [ ] Provide the reviewer with: task description, files changed, get_log output
- [ ] Address all Critical issues before continuing to next task
- [ ] Address all Important issues before final commit

## Context to Provide the Reviewer

```
Task N complete: <description>

Files changed:
- res://scripts/<script>.gd (created/modified)
- res://scenes/<scene>.tscn (modified)

get_log output (last 50 lines):
<paste output>

Please review for: GDScript idioms, scene structure, signal patterns, performance.
```

## After Review

- Critical: fix before continuing
- Important: fix before final commit on this branch
- Suggestions: use judgment
```

- [ ] Create receiving-code-review:

```bash
mkdir -p D:/Projects/godot-toolkit/skills/receiving-code-review
```

Write `D:/Projects/godot-toolkit/skills/receiving-code-review/SKILL.md`:

```markdown
---
name: receiving-code-review
description: Process Godot code review feedback — use when the code-reviewer agent has provided feedback, before implementing suggestions.
---

# Godot Receiving Code Review

Do not implement suggestions blindly. Verify each claim before acting.

## Protocol

For each piece of feedback:

1. **Understand the claim** — what does the reviewer say is wrong?
2. **Verify it** — use `execute` to inspect live state, or read the file
3. **Assess** — is the claim correct for Godot 4.7? Is the fix appropriate?
4. **Act** — fix Critical and Important issues; use judgment on Suggestions
5. **Verify the fix** — run `godot:verification-before-completion` after changes

## Godot 4.x Correctness Checklist

Reject feedback that suggests pre-4.x patterns:

| Reviewer says | Correct Godot 4.x pattern |
|---|---|
| Use `setget` | Use `set(value):` property syntax |
| Use `yield` | Use `await` |
| Use `OS.get_ticks_msec()` for timing | Use `Timer` or `get_tree().create_timer()` |
| Use `connect("signal", self, "_handler")` | Use `signal.connect(_handler)` |
| Use `scene.instance()` | Use `scene.instantiate()` |

## If Feedback Seems Wrong

Run `godot:docs` to verify the correct Godot 4.7 API before pushing back on the reviewer.
```

- [ ] Commit:

```bash
cd D:/Projects/godot-toolkit
git add skills/requesting-code-review/SKILL.md skills/receiving-code-review/SKILL.md
git commit -m "add requesting/receiving code review skills"
```

---

## Task 13: godot:dispatching-parallel-agents Skill

**Files:**
- Create: `skills/dispatching-parallel-agents/SKILL.md`

- [ ] Create and write:

```bash
mkdir -p D:/Projects/godot-toolkit/skills/dispatching-parallel-agents
```

Write `D:/Projects/godot-toolkit/skills/dispatching-parallel-agents/SKILL.md`:

```markdown
---
name: dispatching-parallel-agents
description: Launch parallel subagents for independent Godot work — use when 2 or more tasks have no shared files or sequential dependency.
---

# Godot Dispatching Parallel Agents

## Independence Check

Tasks are parallel-safe when they:
- Touch different `.gd` files
- Touch different `.tscn` scenes
- Do not share autoloads being mutated

Tasks require sequential execution when:
- One creates a class the other imports
- Both modify the same scene file
- One adds a signal the other connects to

## Dispatch

Launch all independent agents in one message. Each agent needs:

```
Task: <specific task from plan>
Scene: res://scenes/<scene>.tscn
Script: res://scripts/<script>.gd
Nodes: <NodeName1>, <NodeName2>
AgentBridge: available via godot-bridge MCP (execute, get_log, save_all, play_scene, stop_scene)
Skills: godot:coder, godot:verification-before-completion
Success criteria: <exact expected outcome — e.g., "res://scripts/player.gd exists, get_log shows zero errors">
```

## After All Complete

1. `get_log(max_lines=200)` — detect any conflicts
2. `save_all()` then verify each modified scene with `execute`
3. Invoke `godot:verification-before-completion` on the full project
4. Invoke `godot:requesting-code-review`
```

- [ ] Commit:

```bash
cd D:/Projects/godot-toolkit
git add skills/dispatching-parallel-agents/SKILL.md
git commit -m "add godot:dispatching-parallel-agents skill"
```

---

## Task 14: godot:using-git-worktrees Skill

**Files:**
- Create: `skills/using-git-worktrees/SKILL.md`

- [ ] Create and write:

```bash
mkdir -p D:/Projects/godot-toolkit/skills/using-git-worktrees
```

Write `D:/Projects/godot-toolkit/skills/using-git-worktrees/SKILL.md`:

```markdown
---
name: using-git-worktrees
description: Use git worktrees to isolate Godot feature branches — use when starting feature work that needs isolation from the main branch.
---

# Godot Using Git Worktrees

## Create a Worktree

```bash
cd D:/Projects/godot-toolkit
git worktree add ../godot-toolkit-<feature> -b feature/<feature>
cd ../godot-toolkit-<feature>
```

## Re-establish AgentBridge

The AgentBridge module symlink is NOT copied to the new worktree. Re-link it:

```bash
# From the new worktree directory
bash install.sh
```

This re-runs the junction/symlink for `D:/Projects/godot/modules/agent_bridge` and re-registers the godot-bridge MCP server.

## Work in the Worktree

Implement the feature. Commit freely. The main branch is unaffected.

## Merge Back

```bash
cd D:/Projects/godot-toolkit
git merge feature/<feature> --no-ff -m "feat: <feature>"
git worktree remove ../godot-toolkit-<feature>
git branch -d feature/<feature>
```

## Verify After Merge

```
play_scene(scene="res://path/to/main_scene.tscn")
get_log(max_lines=200)
```

Zero errors required after merge.
```

- [ ] Commit:

```bash
cd D:/Projects/godot-toolkit
git add skills/using-git-worktrees/SKILL.md
git commit -m "add godot:using-git-worktrees skill"
```

---

## Task 15: godot:finishing-a-development-branch Skill

**Files:**
- Create: `skills/finishing-a-development-branch/SKILL.md`

- [ ] Create and write:

```bash
mkdir -p D:/Projects/godot-toolkit/skills/finishing-a-development-branch
```

Write `D:/Projects/godot-toolkit/skills/finishing-a-development-branch/SKILL.md`:

```markdown
---
name: finishing-a-development-branch
description: Complete a Godot development branch — use when implementation is done and needs to be integrated into the main branch.
---

# Godot Finishing a Development Branch

## Checklist

- [ ] **Verify** — `godot:verification-before-completion` passes (zero errors)
- [ ] **Review** — `godot:requesting-code-review` complete, all Critical/Important issues addressed
- [ ] **Clean log** — `get_log(max_lines=200)` shows zero errors
- [ ] **All commits clean** — no uncommitted changes: `git status`
- [ ] **Decide: merge or PR**

## Option A: Direct Merge

```bash
git checkout master
git merge feature/<feature> --no-ff -m "feat: <feature>"
git branch -d feature/<feature>
```

Verify after merge:
```
play_scene(scene="res://path/to/main_scene.tscn")
get_log(max_lines=200)
```

## Option B: Pull Request

```bash
git push origin feature/<feature>
gh pr create --title "<feature>" --body "$(cat docs/specs/<spec-file>.md)"
```

## Cleanup

If a worktree was used:
```bash
git worktree remove ../godot-toolkit-<feature>
```
```

- [ ] Commit:

```bash
cd D:/Projects/godot-toolkit
git add skills/finishing-a-development-branch/SKILL.md
git commit -m "add godot:finishing-a-development-branch skill"
```

---

## Task 16: godot:writing-skills Skill

**Files:**
- Create: `skills/writing-skills/SKILL.md`

- [ ] Create and write:

```bash
mkdir -p D:/Projects/godot-toolkit/skills/writing-skills
```

Write `D:/Projects/godot-toolkit/skills/writing-skills/SKILL.md`:

```markdown
---
name: writing-skills
description: Create new godot:* skills — use when adding a new skill to the godot plugin.
---

# Godot Writing Skills

## File Location

```
D:/Projects/godot-toolkit/skills/<skill-name>/SKILL.md
```

## Required Frontmatter

```markdown
---
name: <skill-name>
description: <one line — "use when X" format>
---
```

## Description Quality

The `description` field is what Claude uses to auto-trigger the skill. It must:
- Use "use when X" format
- Name specific Godot concepts (GDScript, scene, signal, AgentBridge, shader)
- Be specific enough not to false-trigger on non-Godot tasks

Good: `"Write Godot shader code — use when creating spatial, canvas_item, or particle shaders."`
Bad: `"Use when writing code"` (too broad)

## Content Structure

**Domain skills** (coder, shader, ui, etc.):
1. One-sentence purpose
2. Rules or patterns (with GDScript/GLSL code examples)
3. AgentBridge usage examples where relevant
4. Context7 footer if API lookups are likely

**Process skills** (brainstorming, debugging, etc.):
1. One-sentence purpose
2. Hard gate (if applicable)
3. Numbered checklist
4. AgentBridge checkpoint pattern

## Self-Contained Rule

No skill may reference another plugin's skills. All content must be usable without external dependencies.

## After Creating

Restart Claude Code to reload the plugin and pick up the new skill.
Verify with: `claude plugin list` → confirm `godot` appears with updated skill count.
```

- [ ] Commit:

```bash
cd D:/Projects/godot-toolkit
git add skills/writing-skills/SKILL.md
git commit -m "add godot:writing-skills skill"
```

---

## Task 17: code-reviewer Agent

**Files:**
- Create: `agents/code-reviewer.md`

- [ ] Create directory and write file:

```bash
mkdir -p D:/Projects/godot-toolkit/agents
```

Write `D:/Projects/godot-toolkit/agents/code-reviewer.md`:

```markdown
---
name: code-reviewer
description: |
  Godot code reviewer — use after completing a plan step to review GDScript idioms, scene structure, signal patterns, and AgentBridge usage. Examples: <example>Context: A plan step was just completed. user: "I've finished Task 3 — the PlayerController script is done" assistant: "Let me invoke the code-reviewer agent to review the implementation" <commentary>A plan step is complete, use code-reviewer</commentary></example> <example>Context: User finished implementing a scene. user: "The HUD scene is complete with all signals connected" assistant: "Great — invoking the code-reviewer agent before we continue" <commentary>Scene implementation complete, trigger code review</commentary></example>
model: inherit
---

You are a senior Godot 4.7 developer reviewing code for correctness, GDScript idioms, and performance.

## Before Reviewing

Run:
```
get_log(max_lines=100)
```
Note any live errors related to the code under review.

## Review Checklist

**GDScript Idioms**
- Typed variables: `var speed: float` not `var speed`
- `@export` for inspector-exposed properties
- `@onready var node = $Path` for node references (never in `_init`)
- `%NodeName` for key node references
- `signal my_signal(arg: Type)` declaration
- `node.signal.connect(_handler)` connection (not old string-based)
- `await` not `yield`
- `set(value):` property syntax, not `setget`
- `super()` for overrides
- `scene.instantiate()` not `scene.instance()`

**Scene Structure**
- Nodes added at runtime have `node.owner = root` set
- Scenes are independently runnable (F6 works without dependencies)
- No direct cross-scene node references — signals or autoloads only

**AgentBridge Usage**
- `save_all()` called before `play_scene()`
- `get_log()` checked after every `execute()`
- Errors in log addressed, not ignored

**Performance**
- No allocations (`new()`, array literals, dict literals) inside `_process()` or `_physics_process()`
- `Timer` node or `get_tree().create_timer()` used, not `OS.get_ticks_msec()`
- No `find_node()` or `get_node()` in hot paths — use `@onready` or `%NodeName`

## Output Format

Start by acknowledging what was done well.

**Critical** (must fix before continuing):
- [issue] — [why it's wrong in Godot 4.7] — [exact fix]

**Important** (fix before final commit):
- [issue] — [why] — [fix]

**Suggestions** (optional):
- [issue] — [why] — [alternative]

If no issues in a category, omit that section.
```

- [ ] Verify:

```bash
head -5 D:/Projects/godot-toolkit/agents/code-reviewer.md
```

Expected: frontmatter with `name: code-reviewer`.

- [ ] Commit:

```bash
cd D:/Projects/godot-toolkit
git add agents/code-reviewer.md
git commit -m "add godot:code-reviewer agent"
```

---

## Task 18: /godot Command

**Files:**
- Create: `commands/godot.md`

- [ ] Create directory and write file:

```bash
mkdir -p D:/Projects/godot-toolkit/commands
```

Write `D:/Projects/godot-toolkit/commands/godot.md`:

```markdown
---
name: godot
description: Load the Godot development context — use at the start of a session or when switching to Godot work mid-session.
---

# Godot Development Context

You are working on a **Godot 4.7** project with the **AgentBridge** module active.

## AgentBridge MCP Tools

| Tool | Purpose |
|---|---|
| `execute(code)` | Run GDScript in the live editor |
| `get_log(max_lines)` | Read editor output log |
| `save_all()` | Save all open scenes and scripts |
| `play_scene(scene)` | Start playing a scene |
| `stop_scene()` | Stop the playing scene |

## Domain Skills — When to Use Each

| Skill | Use when |
|---|---|
| `godot:coder` | Writing any GDScript — classes, autoloads, methods |
| `godot:architect` | Designing scene hierarchy, autoload structure, resource patterns |
| `godot:debugger` | Encountering errors, crashes, or unexpected behavior |
| `godot:runner` | Running, testing, or profiling scenes |
| `godot:scene` | Adding, removing, or inspecting nodes programmatically |
| `godot:shader` | Writing spatial, canvas_item, or particle shaders |
| `godot:signals` | Designing event systems or connecting signals |
| `godot:ui` | Building Control-based UI, HUDs, menus |
| `godot:docs` | Looking up Godot 4.x API docs via Context7 |

## Process Skills — When to Use Each

| Skill | Use when |
|---|---|
| `godot:brainstorming` | Before implementing any new feature |
| `godot:writing-plans` | After brainstorming, before writing code |
| `godot:executing-plans` | Executing a written implementation plan |
| `godot:subagent-driven-development` | Independent subtasks suitable for parallel agents |
| `godot:systematic-debugging` | Any debugging — always use instead of guessing |
| `godot:verification-before-completion` | Before claiming any work is done |
| `godot:requesting-code-review` | After completing a plan step |
| `godot:receiving-code-review` | Processing code review feedback |
| `godot:dispatching-parallel-agents` | Launching parallel agents for independent work |
| `godot:using-git-worktrees` | Starting feature work that needs branch isolation |
| `godot:finishing-a-development-branch` | When implementation is complete and ready to integrate |
| `godot:writing-skills` | Creating new godot:* skills |
```

- [ ] Commit:

```bash
cd D:/Projects/godot-toolkit
git add commands/godot.md
git commit -m "add /godot command"
```

---

## Task 19: SessionStart Hook

**Files:**
- Create: `hooks/hooks.json`
- Create: `hooks/scripts/session-start.sh`
- Create: `hooks/scripts/session-start.bat`

- [ ] Create directories:

```bash
mkdir -p D:/Projects/godot-toolkit/hooks/scripts
```

- [ ] Write `hooks/hooks.json`:

```json
{
  "SessionStart": [
    {
      "hooks": [
        {
          "type": "command",
          "command": "bash ${CLAUDE_PLUGIN_ROOT}/hooks/scripts/session-start.sh"
        }
      ]
    }
  ]
}
```

- [ ] Write `hooks/scripts/session-start.sh`:

```bash
#!/usr/bin/env bash
# Detect if the current working directory contains a Godot project.
# Searches up to 2 levels deep for project.godot.
# If found, outputs the godot:godot skill content as session context.

if find . -maxdepth 2 -name "project.godot" 2>/dev/null | grep -q .; then
    cat "$CLAUDE_PLUGIN_ROOT/skills/godot/SKILL.md"
fi
```

- [ ] Write `hooks/scripts/session-start.bat`:

```bat
@echo off
dir /s /b /a-d project.godot 2>nul | findstr /i "project.godot" >nul
if %ERRORLEVEL% EQU 0 (
    type "%CLAUDE_PLUGIN_ROOT%\skills\godot\SKILL.md"
)
```

- [ ] Make the shell script executable:

```bash
chmod +x D:/Projects/godot-toolkit/hooks/scripts/session-start.sh
```

- [ ] Verify hook script works (from a Godot project directory, e.g. D:/GodotProjects/demo-host):

```bash
cd D:/GodotProjects/demo-host
CLAUDE_PLUGIN_ROOT=D:/Projects/godot-toolkit bash D:/Projects/godot-toolkit/hooks/scripts/session-start.sh | head -5
```

Expected: first 5 lines of `skills/godot/SKILL.md` (the `---` frontmatter).

- [ ] Commit:

```bash
cd D:/Projects/godot-toolkit
git add hooks/
git commit -m "add SessionStart hook with Godot project detection"
```

---

## Task 20: Update Install Scripts

**Files:**
- Modify: `install.sh`
- Modify: `install.bat`

- [ ] Read current `install.sh` to confirm exact content before editing.

- [ ] Append plugin install step to `install.sh` — replace the final `echo` block with:

```bash
echo ""
echo "[godot plugin] Installing Claude Code plugin..."
if command -v claude &>/dev/null; then
    claude plugin install "$SCRIPT_DIR" --scope user
    echo "[godot plugin] Plugin installed (user scope)."
else
    echo "WARNING: 'claude' not found. Run manually:"
    echo "  claude plugin install $SCRIPT_DIR --scope user"
fi

echo ""
echo "[AgentBridge + godot plugin] Done. Next steps:"
echo "  1. Rebuild Godot: cd D:/Projects/godot && scons platform=windows target=editor -j32"
echo "  2. Open demo-host: D:/GodotProjects/demo-host"
echo "  3. Verify: claude plugin list && claude mcp list"
```

- [ ] Append plugin install step to `install.bat` — add before `endlocal`:

```bat
echo.
echo [godot plugin] Installing Claude Code plugin...
claude plugin install "%~dp0" --scope user
if %ERRORLEVEL% NEQ 0 (
    echo WARNING: Plugin install failed. Run manually:
    echo   claude plugin install %~dp0 --scope user
) else (
    echo [godot plugin] Plugin installed (user scope).
)

echo.
echo [AgentBridge + godot plugin] Done. Next steps:
echo   1. Rebuild Godot: cd D:\Projects\godot ^&^& scons platform=windows target=editor -j32
echo   2. Open demo-host: D:\GodotProjects\demo-host
echo   3. Verify: claude plugin list ^&^& claude mcp list
```

- [ ] Verify both files have the new plugin install step:

```bash
grep "plugin install" D:/Projects/godot-toolkit/install.sh D:/Projects/godot-toolkit/install.bat
```

Expected: both files show the `claude plugin install` line.

- [ ] Commit:

```bash
cd D:/Projects/godot-toolkit
git add install.sh install.bat
git commit -m "update install scripts to register godot plugin"
```

---

## Self-Review

**Spec coverage check:**

| Spec requirement | Task |
|---|---|
| `.claude-plugin/plugin.json` with `name: "godot"` | Task 1 |
| `docs/superpowers/` removed | Task 2 |
| 8 domain skills migrated + renamed | Task 3 |
| Context7 footer in coder/architect/shader/ui | Task 3 |
| `godot:godot` entry skill | Task 4 |
| `godot:docs` skill | Task 5 |
| `godot:brainstorming` | Task 6 |
| `godot:writing-plans` | Task 7 |
| `godot:executing-plans` | Task 8 |
| `godot:subagent-driven-development` | Task 9 |
| `godot:systematic-debugging` | Task 10 |
| `godot:verification-before-completion` | Task 11 |
| `godot:requesting-code-review` + `godot:receiving-code-review` | Task 12 |
| `godot:dispatching-parallel-agents` | Task 13 |
| `godot:using-git-worktrees` | Task 14 |
| `godot:finishing-a-development-branch` | Task 15 |
| `godot:writing-skills` | Task 16 |
| `godot:code-reviewer` agent | Task 17 |
| `/godot` command | Task 18 |
| `SessionStart` hook with project detection | Task 19 |
| `install.sh` / `install.bat` updated with `-j32` | Task 20 |

All spec requirements covered. No gaps.
