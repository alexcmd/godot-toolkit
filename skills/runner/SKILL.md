---
name: runner
description: Run, test, and profile Godot scenes using AgentBridge — use when you need to execute, observe, or benchmark a scene.
type: process
---

# Godot Runner

All AgentBridge communication goes through `godot-exec.sh`. Never use raw curl.

```bash
GDEXEC="bash ${CLAUDE_SKILL_DIR}/scripts/godot-exec.sh"
```

## Pre-flight: ensure editor is running for this project

Run this exact sequence every time before using AgentBridge:

```bash
GDEXEC="bash ${CLAUDE_SKILL_DIR}/scripts/godot-exec.sh"
PROJECT="<project-path>"   # look up from memory — machine-specific
GODOT_EXE="<godot-exe>"    # look up from memory — machine-specific
PORT_FILE="${PROJECT}/.godot/agentbridge.port"

# Step 1 — if already running and responsive, reuse it
if $GDEXEC --health 2>/dev/null; then
    echo "[AgentBridge] Editor running — reusing."
else
    # Step 2 — check for stale port file (editor crashed)
    if [[ -f "$PORT_FILE" ]]; then
        echo "[AgentBridge] Stale port file found — editor crashed. Relaunching..."
        rm -f "$PORT_FILE"
    else
        echo "[AgentBridge] Editor not running. Launching..."
    fi

    # Step 3 — launch editor
    powershell.exe -Command "Start-Process '${GODOT_EXE}' -ArgumentList '--path','${PROJECT}','--editor'"

    # Step 4 — wait for AgentBridge to write the port file and respond
    for i in {1..15}; do
        sleep 2
        $GDEXEC --health 2>/dev/null && { echo "[AgentBridge] Connected."; break; }
        echo "[AgentBridge] Waiting... ($i/15)"
    done
    $GDEXEC --health   # final check — exits 1 if still not up
fi
```

Port file location: `<project>/.godot/agentbridge.port` — written by the editor on startup, deleted on close. If the file exists but health fails, the editor crashed and must be relaunched.

**No need to restart Claude Code** — once health passes, proceed in this same session.

---

## Run a scene and capture output

```bash
GDEXEC="bash ${CLAUDE_SKILL_DIR}/scripts/godot-exec.sh"

$GDEXEC --save-all                        # ensure changes are saved
$GDEXEC --play res://path/to/scene.tscn   # start playing
sleep 3                                   # let scene initialize
$GDEXEC --stop                            # stop
$GDEXEC --log 200                         # inspect output
```

## Execute GDScript and check result

```bash
$GDEXEC --execute 'print(ProjectSettings.get_setting("application/config/name"))'

# Multiline code — use heredoc:
$GDEXEC --execute - <<'GD'
print(OS.get_name())
print(Engine.get_version_info())
GD
```

## Common diagnostic snippets

```bash
# Check scene tree root
$GDEXEC --execute - <<'GD'
var root = EditorInterface.get_edited_scene_root()
print(root.name, " — ", root.get_child_count(), " children")
GD

# List all nodes
$GDEXEC --execute - <<'GD'
func list_nodes(node: Node, indent: int = 0) -> void:
    print(" ".repeat(indent), node.name, " (", node.get_class(), ")")
    for child in node.get_children():
        list_nodes(child, indent + 2)
list_nodes(EditorInterface.get_edited_scene_root())
GD
```

## Profile timing

```bash
$GDEXEC --execute - <<'GD'
var start = Time.get_ticks_usec()
# ... code to profile ...
print("Elapsed: ", Time.get_ticks_usec() - start, " us")
GD
```
