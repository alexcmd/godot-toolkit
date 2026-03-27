---
name: debugger
description: Debug Godot issues using AgentBridge — use when encountering errors, unexpected behavior, or runtime crashes.
type: process
---

# Godot Debugger

```bash
GDEXEC="bash ${CLAUDE_SKILL_DIR}/../runner/scripts/godot-exec.sh"
```

## Workflow

1. **Get recent log** — always start here:
   ```bash
   $GDEXEC --log 200
   ```

2. **Execute a diagnostic** — narrow down the problem:
   ```bash
   $GDEXEC --execute - <<'GD'
   var root = EditorInterface.get_edited_scene_root()
   if root:
       print("Root: ", root.name, " children: ", root.get_child_count())
   else:
       print("No scene open")
   GD
   ```

3. **Inspect a specific node path:**
   ```bash
   $GDEXEC --execute - <<'GD'
   var root = EditorInterface.get_edited_scene_root()
   var node = root.get_node_or_null("Player/CollisionShape3D")
   print("Found: ", node != null)
   if node:
       print("Type: ", node.get_class())
   GD
   ```

4. **Check a script method exists:**
   ```bash
   $GDEXEC --execute - <<'GD'
   var root = EditorInterface.get_edited_scene_root()
   var player = root.get_node_or_null("Player")
   print("Has take_damage: ", player.has_method("take_damage"))
   GD
   ```

## Rules

- Never change code without first reading the log
- One change per iteration — never shotgun-fix
- Always verify with `$GDEXEC --log` after each fix
- If stuck after 3 iterations, re-read the relevant API docs via godot:docs
