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
