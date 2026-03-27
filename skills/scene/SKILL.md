---
name: scene
description: Manipulate Godot scene trees and nodes via the AgentBridge execute tool — use when adding, removing, or inspecting nodes programmatically.
type: implementation
---

# Godot Scene Manipulation

```bash
GDEXEC="bash ${CLAUDE_SKILL_DIR}/../runner/scripts/godot-exec.sh"
```

Use `$GDEXEC --execute` to manipulate the scene tree in the live Godot editor.

## Common Patterns

**Get the edited scene root:**
```bash
$GDEXEC --execute 'print(EditorInterface.get_edited_scene_root().name)'
```

**Add a node to the scene:**
```bash
$GDEXEC --execute - <<'GD'
var root = EditorInterface.get_edited_scene_root()
var node = MeshInstance3D.new()
node.name = "MyMesh"
root.add_child(node, true)
node.owner = root  # required for serialization
GD
```

**Find a node by name:**
```bash
$GDEXEC --execute - <<'GD'
var root = EditorInterface.get_edited_scene_root()
var player = root.get_node_or_null("Player")
if player:
    print("Found: ", player.get_class())
GD
```

**Remove a node:**
```bash
$GDEXEC --execute - <<'GD'
var root = EditorInterface.get_edited_scene_root()
var node = root.get_node_or_null("OldNode")
if node:
    node.queue_free()
GD
```

**Set a node property:**
```bash
$GDEXEC --execute - <<'GD'
var root = EditorInterface.get_edited_scene_root()
var mesh: MeshInstance3D = root.get_node("MyMesh")
mesh.position = Vector3(0, 1, 0)
GD
```

**Save after changes:**
```bash
$GDEXEC --save-all
```
Always call `--save-all` after scene manipulation to persist changes.
