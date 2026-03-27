# Godot AgentBridge — Design Spec
*Date: 2026-03-27*

## Overview

A C++ engine module for Godot 4.7 that embeds an MCP (Model Context Protocol) server directly
into the Godot Editor. Claude Code connects to it as an MCP client, gaining tools to execute
GDScript, control editor playback, manage scenes, and query project state — mirroring the UE
AgentBridge plugin in `D:\Projects\ue-toolkit\AgentHost`.

No Python intermediary. The C++ module IS the MCP server.

---

## Repository Layout

```
D:\Projects\godot-toolkit\
├── AgentBridge\                   ← C++ module source
│   ├── SCsub                      ← SCons build script
│   ├── register_types.h/cpp       ← Module registration (EditorPlugin wired here)
│   ├── agent_bridge_server.h/cpp  ← TCPServer + HTTP/1.1 parser + MCP JSON-RPC 2.0
│   ├── agent_bridge_executor.h/cpp← GDScript dynamic execution + output capture
│   ├── agent_bridge_log_capture.h/cpp ← PrintHandlerList hook (circular buffer)
│   └── editor\
│       └── agent_bridge_plugin.h/cpp ← EditorPlugin subclass; owns server lifetime
├── skills\                        ← Claude Code skill files
│   ├── godot-coder\
│   ├── godot-scene\
│   ├── godot-debugger\
│   ├── godot-architect\
│   ├── godot-ui\
│   ├── godot-shader\
│   ├── godot-signals\
│   └── godot-runner\
├── install.bat / install.sh       ← Symlink AgentBridge\ into godot\modules\agent_bridge\
│                                     Register MCP server in Claude Code settings
├── docs\
└── memory\
```

---

## Target Environment

- **Engine source:** `D:\Projects\godot` (Godot 4.7, latest)
- **Module install path:** `D:\Projects\godot\modules\agent_bridge\`
- **Test host project:** `D:\GodotProjects\demo-host` (Godot 4.7, Forward+, Jolt Physics, D3D12)
- **Port file:** `D:\GodotProjects\demo-host\Saved\AgentBridge.port`
  (written at startup so Claude Code / install script can discover the active port)
- **Default port:** 8765 (scans +1…+100 if occupied, mirrors UE behavior)

---

## Architecture

### Threading Model

```
┌─────────────────────────────────────────────────────┐
│  Background Thread (AgentBridgeServer::_thread_func) │
│  - TCPServer::poll() — accept new connections        │
│  - Read HTTP request bytes from each peer            │
│  - Parse HTTP/1.1 (method, path, headers, body)      │
│  - Push (connection_id, parsed_request) → SafeQueue  │
└──────────────────────┬──────────────────────────────┘
                       │ thread-safe queue
┌──────────────────────▼──────────────────────────────┐
│  Main Thread (AgentBridgePlugin::_process every frame)│
│  - Drain SafeQueue                                   │
│  - Dispatch to MCP handler                           │
│  - Execute GDScript / call EditorInterface           │
│  - Push response → SafeQueue (back to bg thread)     │
└──────────────────────┬──────────────────────────────┘
                       │
┌──────────────────────▼──────────────────────────────┐
│  Background Thread — send HTTP response bytes        │
└─────────────────────────────────────────────────────┘
```

All Godot engine and editor API calls happen **exclusively on the main thread**.
The background thread only does socket I/O and HTTP byte parsing.

### HTTP Layer

Minimal HTTP/1.1 server — enough for MCP Streamable HTTP transport:
- `POST /mcp` — MCP JSON-RPC 2.0 request/response
- `GET  /mcp` — SSE stream for server-initiated notifications (session open/close)
- `GET  /agent/health` — plain JSON health check (for scripts/CI that don't speak MCP)

No keep-alive required for MVP; each connection is closed after response.

### MCP Protocol

Implements MCP 2025-03-26 spec over Streamable HTTP:
- `initialize` / `initialized` handshake
- `tools/list` — returns all tool definitions with JSON Schema parameters
- `tools/call` — dispatches to the appropriate tool handler

Server name: `"godot-bridge"`, version mirrors Godot engine version.

---

## MCP Tools

| Tool | Parameters | Godot API | Notes |
|------|-----------|-----------|-------|
| `health` | — | `Engine`, `EditorInterface` state | Returns editor status, GDScript availability |
| `execute` | `code: str`, `isolated?: bool` | `GDScriptLanguage` dynamic load | Wraps code in `_run()`, captures print output |
| `play_scene` | `scene?: str` | `EditorInterface::play_main_scene()` / `play_current_scene()` / `open_scene_from_path()` + play | If `scene` given, opens then plays |
| `stop_scene` | — | `EditorInterface::stop_playing_scene()` | No-op if not playing |
| `save_scene` | `path?: str` | `EditorInterface::save_scene()` | Saves current or specific scene |
| `save_all` | — | `EditorInterface::save_all_scenes()` | — |
| `open_scene` | `path: str` | `EditorInterface::open_scene_from_path()` | res:// paths |
| `get_log` | `max_lines?: int` | `AgentBridgeLogCapture` buffer | Returns recent print/warn/err output |
| `set_setting` | `key: str`, `value: any` | `ProjectSettings::set_setting()` + save | — |
| `get_setting` | `key: str` | `ProjectSettings::get_setting()` | — |
| `resource_delete` | `path: str` | `DirAccess::remove_absolute()` + FS refresh | Moves to OS trash if available |
| `resource_duplicate` | `src: str`, `dst: str` | `DirAccess::copy()` + FS refresh | — |
| `resource_ensure` | `path: str`, `type: str` | Check + create minimal resource | Creates blank resource of given type if missing |
| `batch` | `calls: array` | sequential dispatch | Each call: `{tool, params}`. Returns array of results. |

---

## GDScript Execution (`execute` tool)

1. Receive `code` string from MCP call.
2. Wrap in a minimal GDScript:
   ```gdscript
   @tool
   extends Node
   func _run():
       <user code here>
   ```
3. Create a `GDScript` resource via `GDScript::new()`, set source via `set_source_code()`, call `reload()`.
4. Instantiate via `new()` — returns a `Node`-derived Object.
5. Call `_run()` via `call("_run")`.
6. `AgentBridgeLogCapture` buffers all `print()` / `push_error()` output during the call.
7. Return `{output: <captured>, error: <any error string>}`.

`isolated: true` creates a fresh script instance with no retained state (default).
`isolated: false` reuses a persistent instance across calls (shared `__main__`-style context).

---

## Log Capture (`AgentBridgeLogCapture`)

- Registers a custom handler via `add_print_handler()` at module startup.
- Circular buffer of N most recent messages (default 2000, configurable).
- Records: `{level: "print"|"warn"|"error"|"rich", message: str, timestamp_ms: int}`.
- Thread-safe (Mutex-protected writes, main-thread reads).
- Removed via `remove_print_handler()` on module shutdown.

---

## Port Discovery

1. Module starts → finds available port starting at 8765.
2. Writes port number to `{project_dir}/Saved/AgentBridge.port`.
3. Install script configures Claude Code MCP with `--url http://localhost:<port>/mcp`.
   If port file exists at install time, uses that; otherwise uses 8765.
4. Port file is deleted on clean editor shutdown.

---

## Install Script (`install.bat` / `install.sh`)

1. Creates junction/symlink: `D:\Projects\godot\modules\agent_bridge` → `D:\Projects\godot-toolkit\AgentBridge`
2. Registers MCP server in Claude Code: `claude mcp add godot-bridge --url http://localhost:8765/mcp`
3. Prints reminder to recompile Godot with `scons platform=windows target=editor`.

---

## Skills (Initial Set)

Each skill lives in `skills/<name>/` as a markdown file following the superpowers skill format.

| Skill | Purpose |
|-------|---------|
| `godot-coder` | Writing idiomatic GDScript 4.x |
| `godot-scene` | Scene tree / node manipulation via `execute` tool |
| `godot-debugger` | Systematic Godot debugging using log + execute |
| `godot-architect` | Project structure, autoloads, resource design |
| `godot-ui` | Control nodes, themes, anchors, CanvasLayer |
| `godot-shader` | Godot Shading Language, VisualShader |
| `godot-signals` | Signal-driven architecture, connect/disconnect patterns |
| `godot-runner` | Running, profiling, and testing scenes via AgentBridge |

---

## Out of Scope (MVP)

- C# / .NET script execution (GDScript only for MVP)
- Export / packaging via MCP tool
- Hot-reload triggering (scripts reload automatically in Godot editor on file change)
- Authentication / API key for the MCP server
- Windows-only for MVP (Linux/macOS later)
