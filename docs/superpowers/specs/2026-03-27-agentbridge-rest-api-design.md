# AgentBridge REST API Design

**Date:** 2026-03-27
**Goal:** Add a generic REST API to AgentBridge, shell scripts that use it directly via HTTP, and Godot editor auto-launch on session start — so the plugin works without an MCP server registered in Claude Code.

---

## Approach

Option A — REST shim inside AgentBridge. Refactor the existing MCP tool dispatch into a reusable `_invoke_tool(name, args)` method. Add REST routes under `/agent/` that call into it. The MCP path is unchanged. Shell scripts (`ab.sh` / `ab.bat`) hit the REST routes with plain `curl`. Session-start detects whether the Godot editor is running and launches it if not.

---

## Section 1: AgentBridge REST API (C++)

### Routes

All new routes are added to `agent_bridge_server.cpp` alongside the existing `/mcp` and `/agent/health` routes.

| Method | Path | Body / Query | Maps to tool |
|---|---|---|---|
| `GET` | `/agent/health` | — | `health` (existing) |
| `POST` | `/agent/execute` | `{"code": "...", "isolated"?: bool}` | `execute` |
| `GET` | `/agent/log` | `?max=100` | `get_log` |
| `POST` | `/agent/save-all` | — | `save_all` |
| `POST` | `/agent/save-scene` | `{"path"?: "res://..."}` | `save_scene` |
| `POST` | `/agent/play` | `{"scene"?: "res://..."}` | `play_scene` |
| `POST` | `/agent/stop` | — | `stop_scene` |
| `POST` | `/agent/open` | `{"path": "res://..."}` | `open_scene` |
| `GET` | `/agent/setting` | `?key=application/name` | `get_setting` |
| `POST` | `/agent/setting` | `{"key": "...", "value": ...}` | `set_setting` |
| `POST` | `/agent/resource-delete` | `{"path": "res://..."}` | `resource_delete` |
| `POST` | `/agent/resource-duplicate` | `{"src": "res://...", "dst": "res://..."}` | `resource_duplicate` |
| `POST` | `/agent/resource-ensure` | `{"path": "res://...", "type"?: "..."}` | `resource_ensure` |
| `POST` | `/agent/batch` | `[{"tool": "...", "args": {...}}, ...]` | `batch` |

### Response Format

Success:
```json
{"ok": true, "result": "output text here"}
```

Error:
```json
{"ok": false, "error": "message here"}
```

HTTP status is always `200 OK` for both success and error responses (the `ok` field carries the result). `400 Bad Request` is returned only for malformed HTTP (missing body when required, unparseable JSON).

### C++ Refactor

Extract tool dispatch from the MCP `tools/call` handler into:

```cpp
String AgentBridgeServer::_invoke_tool(const String &p_name, const Dictionary &p_args, bool &r_ok);
```

This method contains all 14 tool implementations. Both the MCP handler and the new REST handler call `_invoke_tool` — no logic is duplicated.

REST dispatch is added as a new branch in the request handler:

```cpp
if (path.begins_with("/agent/") && path != "/agent/health") {
    _handle_rest(conn);
    return;
}
```

`_handle_rest` maps the path segment after `/agent/` to a tool name, builds the args `Dictionary` from the JSON body or query string, calls `_invoke_tool`, and writes the `{"ok":..., "result"/"error":...}` response.

Path-to-tool name mapping:
- `/agent/execute` → `execute`
- `/agent/log` → `get_log`
- `/agent/save-all` → `save_all`
- `/agent/save-scene` → `save_scene`
- `/agent/play` → `play_scene`
- `/agent/stop` → `stop_scene`
- `/agent/open` → `open_scene`
- `/agent/setting` → `get_setting` (GET) or `set_setting` (POST)
- `/agent/resource-delete` → `resource_delete`
- `/agent/resource-duplicate` → `resource_duplicate`
- `/agent/resource-ensure` → `resource_ensure`
- `/agent/batch` → `batch`

---

## Section 2: `ab` Shell Scripts

### Files

- `hooks/scripts/ab.sh` — bash (Mac/Linux/WSL)
- `hooks/scripts/ab.bat` — Windows cmd

### Usage

```bash
ab <subcommand> [args...]
```

| Subcommand | Args | REST call |
|---|---|---|
| `health` | — | `GET /agent/health` |
| `execute` | `"gdscript code"` | `POST /agent/execute` |
| `log` | `[max_lines]` | `GET /agent/log?max=N` |
| `save-all` | — | `POST /agent/save-all` |
| `save-scene` | `[res://path]` | `POST /agent/save-scene` |
| `play` | `[res://scene]` | `POST /agent/play` |
| `stop` | — | `POST /agent/stop` |
| `open` | `res://path` | `POST /agent/open` |
| `get-setting` | `<key>` | `GET /agent/setting?key=...` |
| `set-setting` | `<key> <value>` | `POST /agent/setting` |
| `resource-delete` | `res://path` | `POST /agent/resource-delete` |
| `resource-duplicate` | `res://src res://dst` | `POST /agent/resource-duplicate` |
| `resource-ensure` | `res://path [type]` | `POST /agent/resource-ensure` |
| `batch` | `'[{...}]'` | `POST /agent/batch` |

### Port Detection

Resolved in this order:
1. `$AGENTBRIDGE_PORT` / `%AGENTBRIDGE_PORT%` environment variable
2. `./Saved/AgentBridge.port` — searched from cwd upward until `project.godot` is found
3. Default: `8765`

### Output

On success: prints the `result` string from the response, exits 0.
On error: prints `ERROR: <message>` to stderr, exits 1.

### Dependencies

`ab.sh` requires `curl` and `jq`. Both are standard on Mac and available on WSL/Linux. The script checks for both at startup and exits with a clear message if missing.

`ab.bat` uses `curl` (built into Windows 10+) and `PowerShell` for JSON parsing (always available).

---

## Section 3: Session-Start Enhancement

### Files Modified

- `hooks/scripts/session-start.sh`
- `hooks/scripts/session-start.bat`

### Logic

```
1. Find project.godot (up to 2 levels deep — existing)
2. If not found: exit (no Godot project, skip everything)
3. Resolve port: read Saved/AgentBridge.port, fallback to 8765
4. Health check: GET http://localhost:{port}/agent/health
5. If health check fails (editor not running):
   a. Find Godot executable (see search order below)
   b. If found: launch `godot --editor <project_dir>` in background
   c. If not found: print warning, continue
6. Output skill context (cat SKILL.md — existing)
```

### Godot Executable Search Order

**bash:**
1. `$GODOT_PATH` env var
2. `godot4` on PATH
3. `godot` on PATH
4. macOS: `/Applications/Godot.app/Contents/MacOS/Godot`
5. macOS: `/Applications/Godot_*.app/Contents/MacOS/Godot` (glob)

**bat:**
1. `%GODOT_PATH%` env var
2. `godot4.exe` on PATH
3. `godot.exe` on PATH
4. `%LOCALAPPDATA%\Programs\Godot\Godot*.exe` (glob, picks first match)
5. `%ProgramFiles%\Godot\Godot*.exe` (glob, picks first match)

### Launch Command

```bash
"$GODOT_EXE" --editor "$PROJECT_DIR" &
```

The process is launched in the background. The session-start script does not wait for the editor to finish loading — it outputs the skill context immediately after launching.

---

## Section 4: `godot:coder` Skill Update

Add a note at the bottom of `skills/coder/SKILL.md` covering the two usage modes:

```markdown
## Usage Modes

**MCP mode** (AgentBridge registered as MCP server):
Use the `execute` MCP tool directly.

**HTTP mode** (no MCP server, using `ab` scripts):
Use the Bash tool to call `ab`:
\`\`\`bash
bash $CLAUDE_PLUGIN_ROOT/hooks/scripts/ab.sh execute "print('hello')"
\`\`\`
```

---

## Files Changed

**Modified (C++):**
- `AgentBridge/agent_bridge_server.h` — add `_invoke_tool` and `_handle_rest` declarations
- `AgentBridge/agent_bridge_server.cpp` — refactor tool dispatch, add REST routing

**Created (scripts):**
- `hooks/scripts/ab.sh`
- `hooks/scripts/ab.bat`

**Modified (scripts):**
- `hooks/scripts/session-start.sh`
- `hooks/scripts/session-start.bat`

**Modified (skill):**
- `skills/coder/SKILL.md`
