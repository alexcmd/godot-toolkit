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

## Test Snippets in the Live Editor

```bash
GDEXEC="bash ${CLAUDE_SKILL_DIR}/../runner/scripts/godot-exec.sh"

# Quick one-liner test:
$GDEXEC --execute 'print(Vector3.UP.rotated(Vector3.RIGHT, PI/4))'

# Multiline test:
$GDEXEC --execute - <<'GD'
var v = Vector3(1, 2, 3)
print(v.normalized())
GD

# Check for errors after running:
$GDEXEC --log 50
```

## Docs

When API is uncertain, use godot:docs (Context7) before writing code.
