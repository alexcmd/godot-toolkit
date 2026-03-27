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

## Verify Connections via AgentBridge

```bash
GDEXEC="bash ${CLAUDE_SKILL_DIR}/../runner/scripts/godot-exec.sh"

$GDEXEC --execute - <<'GD'
var node = EditorInterface.get_edited_scene_root().get_node("Player")
var connections = node.get_signal_list()
for sig in connections:
    print(sig.name, ": ", node.get_signal_connection_list(sig.name))
GD
```
