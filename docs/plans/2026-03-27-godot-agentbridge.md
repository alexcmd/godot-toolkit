# Godot AgentBridge Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a C++ Godot 4.7 engine module that embeds an MCP server directly in the editor, letting Claude Code execute GDScript and control the editor via MCP tool calls.

**Architecture:** C++ module in `godot-toolkit/AgentBridge/` (symlinked into `godot/modules/agent_bridge/`). The `AgentBridgePlugin` (EditorPlugin subclass) owns a polling HTTP server that speaks MCP JSON-RPC 2.0 over `POST /mcp`. All Godot API calls happen on the main thread via `_notification(NOTIFICATION_PROCESS)`.

**Tech Stack:** Godot 4.7 C++ (`TCPServer`, `StreamPeerTCP`, `GDScript`, `EditorInterface`, `JSON`), MCP 2025-03-26 Streamable HTTP transport.

**Key paths:**
- Module source: `D:\Projects\godot-toolkit\AgentBridge\`
- Module install: `D:\Projects\godot\modules\agent_bridge\` (junction/symlink)
- Engine source: `D:\Projects\godot\`
- Test project: `D:\GodotProjects\demo-host\`
- Port file: `D:\GodotProjects\demo-host\Saved\AgentBridge.port`

---

## File Map

```
godot-toolkit/AgentBridge/
├── config.py                          ← Module build config
├── SCsub                              ← SCons sources
├── register_types.h                   ← Module init declarations
├── register_types.cpp                 ← Registers EditorPlugin at LEVEL_EDITOR
├── agent_bridge_log_capture.h/cpp     ← PrintHandlerList hook, circular buffer
├── agent_bridge_executor.h/cpp        ← GDScript dynamic load+run+capture
├── agent_bridge_server.h/cpp          ← TCPServer, HTTP/1.1 parser, MCP JSON-RPC
└── editor/
    ├── agent_bridge_plugin.h/cpp      ← EditorPlugin: owns server, polls each frame

godot-toolkit/skills/
└── godot-{coder,scene,debugger,architect,ui,shader,signals,runner}/skill.md

godot-toolkit/
├── install.bat
└── install.sh
```

---

### Task 1: Module Scaffold

**Files:**
- Create: `godot-toolkit/AgentBridge/config.py`
- Create: `godot-toolkit/AgentBridge/SCsub`
- Create: `godot-toolkit/AgentBridge/register_types.h`
- Create: `godot-toolkit/AgentBridge/register_types.cpp`
- Create: `godot-toolkit/AgentBridge/editor/` (empty dir placeholder)

- [ ] **Step 1: Create directory structure**

```
mkdir -p D:/Projects/godot-toolkit/AgentBridge/editor
```

- [ ] **Step 2: Write `config.py`**

```python
# godot-toolkit/AgentBridge/config.py
def can_build(env, platform):
    return True

def configure(env):
    pass
```

- [ ] **Step 3: Write `SCsub`**

```python
# godot-toolkit/AgentBridge/SCsub
#!/usr/bin/env python
from misc.utility.scons_hints import *

Import("env")
Import("env_modules")

env_ab = env_modules.Clone()

if env["tools"]:
    env_ab.add_source_files(env.modules_sources, "*.cpp")
    env_ab.add_source_files(env.modules_sources, "editor/*.cpp")
```

- [ ] **Step 4: Write `register_types.h`**

```cpp
// godot-toolkit/AgentBridge/register_types.h
#pragma once
#include "modules/register_module_types.h"

void initialize_agent_bridge_module(ModuleInitializationLevel p_level);
void uninitialize_agent_bridge_module(ModuleInitializationLevel p_level);
```

- [ ] **Step 5: Write `register_types.cpp` (minimal — no plugin yet)**

```cpp
// godot-toolkit/AgentBridge/register_types.cpp
#include "register_types.h"

void initialize_agent_bridge_module(ModuleInitializationLevel p_level) {
    // EditorPlugin wired in Task 10
}

void uninitialize_agent_bridge_module(ModuleInitializationLevel p_level) {
}
```

- [ ] **Step 6: Create junction into Godot modules**

Run as Administrator in PowerShell:
```powershell
New-Item -ItemType Junction -Path "D:\Projects\godot\modules\agent_bridge" -Target "D:\Projects\godot-toolkit\AgentBridge"
```

Or on Linux/Mac:
```bash
ln -s D:/Projects/godot-toolkit/AgentBridge D:/Projects/godot/modules/agent_bridge
```

- [ ] **Step 7: Verify module is detected**

```bash
cd D:/Projects/godot
python SCons/scons.py platform=windows target=editor --dry-run 2>&1 | grep agent_bridge
```

Expected: `agent_bridge` appears in the module list (no errors).

- [ ] **Step 8: Compile minimal scaffold**

```bash
cd D:/Projects/godot
scons platform=windows target=editor -j8
```

Expected: Compiles cleanly. `bin/godot.windows.editor.x86_64.exe` produced.

- [ ] **Step 9: Commit**

```bash
cd D:/Projects/godot-toolkit
git init  # if not already a repo
git add AgentBridge/config.py AgentBridge/SCsub AgentBridge/register_types.h AgentBridge/register_types.cpp
git commit -m "feat: add agent_bridge module scaffold"
```

---

### Task 2: Log Capture

**Files:**
- Create: `godot-toolkit/AgentBridge/agent_bridge_log_capture.h`
- Create: `godot-toolkit/AgentBridge/agent_bridge_log_capture.cpp`

- [ ] **Step 1: Write `agent_bridge_log_capture.h`**

```cpp
// godot-toolkit/AgentBridge/agent_bridge_log_capture.h
#pragma once

#include "core/os/mutex.h"
#include "core/string/print_string.h"
#include "core/string/ustring.h"
#include "core/templates/vector.h"

struct AgentLogEntry {
    String message;
    bool is_error = false;
    bool is_rich = false;
};

class AgentBridgeLogCapture {
    static void _print_handler(void *p_userdata, const String &p_string, bool p_error, bool p_rich);

    PrintHandlerList handler;
    mutable Mutex mutex;
    Vector<AgentLogEntry> buffer; // circular, size = capacity
    int capacity = 0;
    int write_pos = 0;
    int count = 0;

public:
    explicit AgentBridgeLogCapture(int p_capacity = 2000);
    ~AgentBridgeLogCapture();

    // Returns up to p_max most recent entries (oldest first).
    Vector<AgentLogEntry> get_recent(int p_max = 200) const;
    void clear();
};
```

- [ ] **Step 2: Write `agent_bridge_log_capture.cpp`**

```cpp
// godot-toolkit/AgentBridge/agent_bridge_log_capture.cpp
#include "agent_bridge_log_capture.h"
#include "core/os/mutex.h"

void AgentBridgeLogCapture::_print_handler(void *p_userdata, const String &p_string, bool p_error, bool p_rich) {
    AgentBridgeLogCapture *self = static_cast<AgentBridgeLogCapture *>(p_userdata);
    MutexLock lock(self->mutex);

    AgentLogEntry entry;
    entry.message = p_string;
    entry.is_error = p_error;
    entry.is_rich = p_rich;

    if (self->count < self->capacity) {
        self->buffer.resize(self->count + 1);
        self->buffer.write[self->count] = entry;
        self->count++;
        self->write_pos = self->count % self->capacity;
    } else {
        self->buffer.write[self->write_pos] = entry;
        self->write_pos = (self->write_pos + 1) % self->capacity;
    }
}

AgentBridgeLogCapture::AgentBridgeLogCapture(int p_capacity) :
        capacity(p_capacity) {
    buffer.resize(0);
    handler.printfunc = _print_handler;
    handler.userdata = this;
    add_print_handler(&handler);
}

AgentBridgeLogCapture::~AgentBridgeLogCapture() {
    remove_print_handler(&handler);
}

Vector<AgentLogEntry> AgentBridgeLogCapture::get_recent(int p_max) const {
    MutexLock lock(mutex);
    Vector<AgentLogEntry> result;
    int n = MIN(count, p_max);
    result.resize(n);
    // oldest entry starts at write_pos when buffer is full, else at 0
    int start = (count < capacity) ? 0 : write_pos;
    // Return the last n entries
    int skip = count - n;
    for (int i = 0; i < n; i++) {
        int idx = (start + skip + i) % capacity;
        result.write[i] = buffer[idx];
    }
    return result;
}

void AgentBridgeLogCapture::clear() {
    MutexLock lock(mutex);
    count = 0;
    write_pos = 0;
    buffer.resize(0);
}
```

- [ ] **Step 3: Compile**

```bash
cd D:/Projects/godot && scons platform=windows target=editor -j8
```

Expected: Compiles cleanly.

- [ ] **Step 4: Commit**

```bash
cd D:/Projects/godot-toolkit
git add AgentBridge/agent_bridge_log_capture.h AgentBridge/agent_bridge_log_capture.cpp
git commit -m "feat: add log capture (PrintHandlerList circular buffer)"
```

---

### Task 3: GDScript Executor

**Files:**
- Create: `godot-toolkit/AgentBridge/agent_bridge_executor.h`
- Create: `godot-toolkit/AgentBridge/agent_bridge_executor.cpp`

- [ ] **Step 1: Write `agent_bridge_executor.h`**

```cpp
// godot-toolkit/AgentBridge/agent_bridge_executor.h
#pragma once

#include "agent_bridge_log_capture.h"
#include "core/string/ustring.h"

struct AgentExecResult {
    String output;  // captured print lines, newline-separated
    String error;   // empty if success
    bool ok = false;
};

class AgentBridgeExecutor {
    AgentBridgeLogCapture *log_capture = nullptr; // not owned

    static String _wrap_code(const String &p_code);

public:
    explicit AgentBridgeExecutor(AgentBridgeLogCapture *p_log);

    // Compiles and runs p_code as GDScript in a fresh RefCounted instance.
    // Captures print output via log_capture snapshot before/after.
    AgentExecResult execute(const String &p_code);
};
```

- [ ] **Step 2: Write `agent_bridge_executor.cpp`**

```cpp
// godot-toolkit/AgentBridge/agent_bridge_executor.cpp
#include "agent_bridge_executor.h"

#include "core/object/class_db.h"
#include "core/object/ref_counted.h"
#include "modules/gdscript/gdscript.h"

String AgentBridgeExecutor::_wrap_code(const String &p_code) {
    // Indent each line of user code with a tab so it sits inside _run().
    PackedStringArray lines = p_code.split("\n");
    String indented;
    for (int i = 0; i < lines.size(); i++) {
        indented += "\t" + lines[i] + "\n";
    }
    return String("@tool\nextends RefCounted\nfunc _run():\n") + indented;
}

AgentBridgeExecutor::AgentBridgeExecutor(AgentBridgeLogCapture *p_log) :
        log_capture(p_log) {}

AgentExecResult AgentBridgeExecutor::execute(const String &p_code) {
    AgentExecResult res;

    // Snapshot log count before execution so we only return new entries.
    Vector<AgentLogEntry> before = log_capture->get_recent(2000);
    int pre_count = before.size();

    // Build and load the wrapped GDScript.
    Ref<GDScript> script;
    script.instantiate();
    script->set_source_code(_wrap_code(p_code));
    // Use a virtual path so ResourceLoader doesn't try to touch disk.
    script->set_path("res://.__agentbridge_exec__.gd", true);

    Error err = script->reload(false);
    if (err != OK) {
        res.error = "GDScript parse/compile error (Error code " + itos(err) + ")";
        res.ok = false;
        return res;
    }

    if (!script->can_instantiate()) {
        res.error = "Script cannot be instantiated.";
        res.ok = false;
        return res;
    }

    Object *obj = script->instantiate();
    if (!obj) {
        res.error = "Failed to create script instance.";
        res.ok = false;
        return res;
    }

    // Wrap in Ref<RefCounted> so memory is managed automatically.
    Ref<RefCounted> guard(Object::cast_to<RefCounted>(obj));

    // Call _run().
    Callable::CallError ce;
    Variant result;
    obj->callp(StringName("_run"), nullptr, 0, result, ce);

    if (ce.error != Callable::CallError::CALL_OK) {
        res.error = "Runtime error in _run() (CallError " + itos(ce.error) + ")";
        res.ok = false;
        // guard goes out of scope → ref count drops
        return res;
    }

    // Collect new log entries produced during execution.
    Vector<AgentLogEntry> after = log_capture->get_recent(2000);
    String output;
    for (int i = pre_count; i < after.size(); i++) {
        output += after[i].message + "\n";
    }

    res.output = output;
    res.ok = true;
    return res;
}
```

- [ ] **Step 3: Compile**

```bash
cd D:/Projects/godot && scons platform=windows target=editor -j8
```

Expected: Compiles cleanly.

- [ ] **Step 4: Commit**

```bash
cd D:/Projects/godot-toolkit
git add AgentBridge/agent_bridge_executor.h AgentBridge/agent_bridge_executor.cpp
git commit -m "feat: add GDScript executor (dynamic load + run + output capture)"
```

---

### Task 4: HTTP Server + MCP Dispatcher

**Files:**
- Create: `godot-toolkit/AgentBridge/agent_bridge_server.h`
- Create: `godot-toolkit/AgentBridge/agent_bridge_server.cpp`

This task implements the TCP server with HTTP/1.1 parsing and MCP JSON-RPC 2.0 routing. Tool *implementations* are added in Tasks 6–9.

- [ ] **Step 1: Write `agent_bridge_server.h`**

```cpp
// godot-toolkit/AgentBridge/agent_bridge_server.h
#pragma once

#include "agent_bridge_executor.h"
#include "agent_bridge_log_capture.h"
#include "core/io/tcp_server.h"
#include "core/io/stream_peer_tcp.h"
#include "core/string/ustring.h"
#include "core/templates/hash_map.h"
#include "core/templates/vector.h"
#include "core/variant/dictionary.h"
#include "core/variant/variant.h"

struct AgentConnection {
    Ref<StreamPeerTCP> peer;
    PackedByteArray recv_buf;
    bool headers_done = false;
    int content_length = -1;
    String method;
    String path;
    int header_end = 0; // byte offset of body start in recv_buf
};

class AgentBridgeServer {
    Ref<TCPServer> tcp_server;
    Vector<AgentConnection> connections;
    uint16_t port = 0;

    AgentBridgeLogCapture *log_capture = nullptr; // not owned
    AgentBridgeExecutor *executor = nullptr;       // not owned

    // ── HTTP helpers ──────────────────────────────────────────────────────
    static bool _parse_headers(AgentConnection &c);
    static String _build_http_response(int status_code, const String &body,
            const String &content_type = "application/json");

    // ── MCP dispatch ──────────────────────────────────────────────────────
    String _handle_request(const String &method, const String &path, const String &body);
    String _handle_mcp(const Dictionary &rpc);
    String _make_rpc_result(const Variant &id, const Variant &result);
    String _make_rpc_error(const Variant &id, int code, const String &message);
    String _make_tool_result(const String &text, bool is_error = false);

    // ── MCP method handlers ───────────────────────────────────────────────
    Variant _method_initialize(const Variant &id, const Dictionary &params);
    Variant _method_tools_list(const Variant &id, const Dictionary &params);
    Variant _method_tools_call(const Variant &id, const Dictionary &params);

    // ── Tool handlers (implementations added in Tasks 6–9) ────────────────
    String _tool_health(const Dictionary &args);
    String _tool_execute(const Dictionary &args);
    String _tool_get_log(const Dictionary &args);
    String _tool_play_scene(const Dictionary &args);
    String _tool_stop_scene(const Dictionary &args);
    String _tool_save_scene(const Dictionary &args);
    String _tool_save_all(const Dictionary &args);
    String _tool_open_scene(const Dictionary &args);
    String _tool_set_setting(const Dictionary &args);
    String _tool_get_setting(const Dictionary &args);
    String _tool_resource_delete(const Dictionary &args);
    String _tool_resource_duplicate(const Dictionary &args);
    String _tool_resource_ensure(const Dictionary &args);
    String _tool_batch(const Dictionary &args);

    // ── Tool schema helpers ───────────────────────────────────────────────
    static Array _make_tools_array();

public:
    AgentBridgeServer(AgentBridgeLogCapture *p_log, AgentBridgeExecutor *p_exec);
    ~AgentBridgeServer();

    Error start(uint16_t p_port_hint = 8765);
    void stop();
    void poll(); // call every frame from EditorPlugin

    uint16_t get_port() const { return port; }
    bool is_running() const;
};
```

- [ ] **Step 2: Write `agent_bridge_server.cpp` — TCP accept + HTTP parsing**

```cpp
// godot-toolkit/AgentBridge/agent_bridge_server.cpp
#include "agent_bridge_server.h"

#include "core/io/ip_address.h"
#include "core/io/json.h"
#include "core/os/os.h"
#include "core/string/ustring.h"
#include "core/variant/array.h"
#include "core/variant/dictionary.h"

// ── Startup / Shutdown ──────────────────────────────────────────────────────

AgentBridgeServer::AgentBridgeServer(AgentBridgeLogCapture *p_log, AgentBridgeExecutor *p_exec) :
        log_capture(p_log), executor(p_exec) {
    tcp_server.instantiate();
}

AgentBridgeServer::~AgentBridgeServer() {
    stop();
}

Error AgentBridgeServer::start(uint16_t p_port_hint) {
    for (uint16_t try_port = p_port_hint; try_port <= p_port_hint + 100; try_port++) {
        Error err = tcp_server->listen(try_port, IPAddress("127.0.0.1"));
        if (err == OK) {
            port = try_port;
            return OK;
        }
    }
    return ERR_CANT_CREATE;
}

void AgentBridgeServer::stop() {
    if (tcp_server.is_valid() && tcp_server->is_listening()) {
        tcp_server->stop();
    }
    connections.clear();
    port = 0;
}

bool AgentBridgeServer::is_running() const {
    return tcp_server.is_valid() && tcp_server->is_listening();
}

// ── Per-frame poll ──────────────────────────────────────────────────────────

void AgentBridgeServer::poll() {
    if (!is_running()) {
        return;
    }

    // Accept new connections.
    while (tcp_server->is_connection_available()) {
        Ref<StreamPeerTCP> peer = tcp_server->take_connection();
        if (peer.is_valid()) {
            AgentConnection conn;
            conn.peer = peer;
            connections.push_back(conn);
        }
    }

    // Service existing connections.
    for (int i = connections.size() - 1; i >= 0; i--) {
        AgentConnection &c = connections.write[i];

        if (c.peer->get_status() != StreamPeerTCP::STATUS_CONNECTED) {
            connections.remove_at(i);
            continue;
        }

        // Read available bytes.
        int available = c.peer->get_available_bytes();
        if (available > 0) {
            int old_size = c.recv_buf.size();
            c.recv_buf.resize(old_size + available);
            int received = 0;
            c.peer->get_partial_data(c.recv_buf.ptrw() + old_size, available, received);
            c.recv_buf.resize(old_size + received);
        }

        // Try to parse headers if not done yet.
        if (!c.headers_done) {
            if (!_parse_headers(c)) {
                continue; // need more data
            }
        }

        // Check if full body has arrived.
        int body_start = c.header_end;
        int body_needed = MAX(0, c.content_length);
        if (c.recv_buf.size() < body_start + body_needed) {
            continue; // need more data
        }

        // Extract body string.
        String body;
        if (body_needed > 0) {
            body = String::utf8(
                    (const char *)(c.recv_buf.ptr() + body_start),
                    body_needed);
        }

        // Handle the request.
        String http_response = _handle_request(c.method, c.path, body);

        // Send response.
        PackedByteArray bytes = http_response.to_utf8_buffer();
        c.peer->put_data(bytes.ptr(), bytes.size());
        c.peer->disconnect_from_host();
        connections.remove_at(i);
    }
}

// ── HTTP header parser ──────────────────────────────────────────────────────

bool AgentBridgeServer::_parse_headers(AgentConnection &c) {
    // Look for \r\n\r\n in recv_buf.
    const uint8_t *data = c.recv_buf.ptr();
    int size = c.recv_buf.size();
    int header_end = -1;
    for (int i = 0; i <= size - 4; i++) {
        if (data[i] == '\r' && data[i + 1] == '\n' &&
                data[i + 2] == '\r' && data[i + 3] == '\n') {
            header_end = i + 4;
            break;
        }
    }
    if (header_end < 0) {
        return false; // incomplete headers
    }

    c.header_end = header_end;
    c.headers_done = true;

    // Parse request line + headers from raw bytes.
    String headers_str = String::utf8((const char *)data, header_end);
    PackedStringArray lines = headers_str.split("\r\n");
    if (lines.is_empty()) {
        return false;
    }

    // Request line: "POST /mcp HTTP/1.1"
    PackedStringArray req_parts = lines[0].split(" ");
    if (req_parts.size() >= 2) {
        c.method = req_parts[0].to_upper();
        c.path = req_parts[1];
    }

    // Headers
    c.content_length = 0;
    for (int i = 1; i < lines.size(); i++) {
        String line = lines[i].strip_edges();
        if (line.is_empty()) {
            continue;
        }
        int colon = line.find(":");
        if (colon < 0) {
            continue;
        }
        String hname = line.substr(0, colon).strip_edges().to_lower();
        String hval = line.substr(colon + 1).strip_edges();
        if (hname == "content-length") {
            c.content_length = hval.to_int();
        }
    }

    return true;
}

// ── HTTP response builder ───────────────────────────────────────────────────

String AgentBridgeServer::_build_http_response(int status_code, const String &body,
        const String &content_type) {
    String status_text = (status_code == 200) ? "OK" : "Bad Request";
    PackedByteArray body_bytes = body.to_utf8_buffer();
    return String("HTTP/1.1 ") + itos(status_code) + " " + status_text + "\r\n" +
            "Content-Type: " + content_type + "\r\n" +
            "Content-Length: " + itos(body_bytes.size()) + "\r\n" +
            "Access-Control-Allow-Origin: *\r\n" +
            "Connection: close\r\n" +
            "\r\n" + body;
}

// ── Request router ──────────────────────────────────────────────────────────

String AgentBridgeServer::_handle_request(const String &method, const String &path, const String &body) {
    // Plain health endpoint (non-MCP, for curl/CI scripts).
    if (path == "/agent/health") {
        Dictionary d;
        d["status"] = "ok";
        d["version"] = "4.7";
        d["port"] = (int)port;
        return _build_http_response(200, JSON::stringify(d));
    }

    // MCP endpoint.
    if (path == "/mcp" && method == "POST") {
        Variant parsed = JSON::parse_string(body);
        if (parsed.get_type() != Variant::DICTIONARY) {
            return _build_http_response(400, R"({"error":"invalid JSON"})");
        }
        String mcp_resp = _handle_mcp(parsed.operator Dictionary());
        return _build_http_response(200, mcp_resp);
    }

    // OPTIONS preflight (CORS).
    if (method == "OPTIONS") {
        return _build_http_response(200, "");
    }

    return _build_http_response(400, R"({"error":"unknown path"})");
}

// ── MCP JSON-RPC 2.0 dispatcher ─────────────────────────────────────────────

String AgentBridgeServer::_handle_mcp(const Dictionary &rpc) {
    Variant id = rpc.get("id", Variant());
    String rpc_method = rpc.get("method", "");
    Dictionary params = rpc.get("params", Dictionary());

    // Notifications have no id and need no response.
    if (rpc_method == "notifications/initialized") {
        return ""; // caller should not send this response
    }

    if (rpc_method == "initialize") {
        Variant result = _method_initialize(id, params);
        return JSON::stringify(_make_rpc_result(id, result));
    }
    if (rpc_method == "tools/list") {
        Variant result = _method_tools_list(id, params);
        return JSON::stringify(_make_rpc_result(id, result));
    }
    if (rpc_method == "tools/call") {
        Variant result = _method_tools_call(id, params);
        return JSON::stringify(_make_rpc_result(id, result));
    }

    // Unknown method.
    return JSON::stringify(_make_rpc_error(id, -32601, "Method not found: " + rpc_method));
}

// ── MCP helpers ─────────────────────────────────────────────────────────────

Variant AgentBridgeServer::_make_rpc_result(const Variant &id, const Variant &result) {
    Dictionary d;
    d["jsonrpc"] = "2.0";
    d["id"] = id;
    d["result"] = result;
    return d;
}

Variant AgentBridgeServer::_make_rpc_error(const Variant &id, int code, const String &message) {
    Dictionary err;
    err["code"] = code;
    err["message"] = message;
    Dictionary d;
    d["jsonrpc"] = "2.0";
    d["id"] = id;
    d["error"] = err;
    return d;
}

String AgentBridgeServer::_make_tool_result(const String &text, bool is_error) {
    Dictionary content_item;
    content_item["type"] = "text";
    content_item["text"] = text;
    Array content;
    content.push_back(content_item);
    Dictionary result;
    result["content"] = content;
    result["isError"] = is_error;
    return JSON::stringify(result);
}

// ── MCP method: initialize ───────────────────────────────────────────────────

Variant AgentBridgeServer::_method_initialize(const Variant &id, const Dictionary &params) {
    Dictionary tools_cap;
    tools_cap["listChanged"] = false;
    Dictionary caps;
    caps["tools"] = tools_cap;
    Dictionary server_info;
    server_info["name"] = "godot-bridge";
    server_info["version"] = "4.7";
    Dictionary result;
    result["protocolVersion"] = "2025-03-26";
    result["capabilities"] = caps;
    result["serverInfo"] = server_info;
    return result;
}

// ── MCP method: tools/list ───────────────────────────────────────────────────

static Dictionary _make_schema(const Dictionary &props, const Array &required = Array()) {
    Dictionary schema;
    schema["type"] = "object";
    schema["properties"] = props;
    if (!required.is_empty()) {
        schema["required"] = required;
    }
    return schema;
}

static Dictionary _str_prop(const String &desc) {
    Dictionary p;
    p["type"] = "string";
    p["description"] = desc;
    return p;
}

static Dictionary _int_prop(const String &desc) {
    Dictionary p;
    p["type"] = "integer";
    p["description"] = desc;
    return p;
}

static Dictionary _bool_prop(const String &desc) {
    Dictionary p;
    p["type"] = "boolean";
    p["description"] = desc;
    return p;
}

static Dictionary _tool(const String &name, const String &desc, const Dictionary &schema) {
    Dictionary t;
    t["name"] = name;
    t["description"] = desc;
    t["inputSchema"] = schema;
    return t;
}

Array AgentBridgeServer::_make_tools_array() {
    Array tools;

    // health
    tools.push_back(_tool("health", "Check Godot editor status.", _make_schema(Dictionary())));

    // execute
    {
        Dictionary props;
        props["code"] = _str_prop("GDScript code to execute in the editor context.");
        props["isolated"] = _bool_prop("If true (default), run in a fresh instance each call.");
        Array req;
        req.push_back("code");
        tools.push_back(_tool("execute", "Execute GDScript code in the Godot editor and return print output.", _make_schema(props, req)));
    }

    // get_log
    {
        Dictionary props;
        props["max_lines"] = _int_prop("Maximum number of recent log lines to return (default 100).");
        tools.push_back(_tool("get_log", "Return recent editor log output (print, warn, error).", _make_schema(props)));
    }

    // play_scene
    {
        Dictionary props;
        props["scene"] = _str_prop("Optional res:// path. If omitted plays the main scene.");
        tools.push_back(_tool("play_scene", "Start playing a scene in the Godot editor (like pressing F5).", _make_schema(props)));
    }

    // stop_scene
    tools.push_back(_tool("stop_scene", "Stop the currently playing scene.", _make_schema(Dictionary())));

    // save_scene
    {
        Dictionary props;
        props["path"] = _str_prop("Optional res:// path. If omitted saves the current scene.");
        tools.push_back(_tool("save_scene", "Save a scene.", _make_schema(props)));
    }

    // save_all
    tools.push_back(_tool("save_all", "Save all open scenes.", _make_schema(Dictionary())));

    // open_scene
    {
        Dictionary props;
        props["path"] = _str_prop("res:// path to the .tscn file.");
        Array req;
        req.push_back("path");
        tools.push_back(_tool("open_scene", "Open a scene in the editor.", _make_schema(props, req)));
    }

    // get_setting
    {
        Dictionary props;
        props["key"] = _str_prop("ProjectSettings key, e.g. \"application/config/name\".");
        Array req;
        req.push_back("key");
        tools.push_back(_tool("get_setting", "Get a ProjectSettings value.", _make_schema(props, req)));
    }

    // set_setting
    {
        Dictionary props;
        props["key"] = _str_prop("ProjectSettings key.");
        props["value"] = Dictionary(); // any type — no schema constraint
        Array req;
        req.push_back("key");
        req.push_back("value");
        tools.push_back(_tool("set_setting", "Set a ProjectSettings value and save the project.", _make_schema(props, req)));
    }

    // resource_delete
    {
        Dictionary props;
        props["path"] = _str_prop("res:// path to the resource file to delete.");
        Array req;
        req.push_back("path");
        tools.push_back(_tool("resource_delete", "Delete a resource file and refresh the filesystem.", _make_schema(props, req)));
    }

    // resource_duplicate
    {
        Dictionary props;
        props["src"] = _str_prop("Source res:// path.");
        props["dst"] = _str_prop("Destination res:// path.");
        Array req;
        req.push_back("src");
        req.push_back("dst");
        tools.push_back(_tool("resource_duplicate", "Copy a resource file to a new path.", _make_schema(props, req)));
    }

    // resource_ensure
    {
        Dictionary props;
        props["path"] = _str_prop("res:// path.");
        props["type"] = _str_prop("Godot class name, e.g. \"GDScript\", \"Resource\".");
        Array req;
        req.push_back("path");
        req.push_back("type");
        tools.push_back(_tool("resource_ensure", "Ensure a resource file exists; creates a blank one if missing.", _make_schema(props, req)));
    }

    // batch
    {
        Dictionary props;
        props["calls"] = Dictionary(); // array of {tool, arguments}
        Array req;
        req.push_back("calls");
        tools.push_back(_tool("batch", "Execute multiple tool calls sequentially and return an array of results.", _make_schema(props, req)));
    }

    return tools;
}

Variant AgentBridgeServer::_method_tools_list(const Variant &id, const Dictionary &params) {
    Dictionary result;
    result["tools"] = _make_tools_array();
    return result;
}

// ── MCP method: tools/call ───────────────────────────────────────────────────

Variant AgentBridgeServer::_method_tools_call(const Variant &id, const Dictionary &params) {
    String tool_name = params.get("name", "");
    Dictionary args = params.get("arguments", Dictionary());

    String tool_result_json;

    if (tool_name == "health") {
        tool_result_json = _tool_health(args);
    } else if (tool_name == "execute") {
        tool_result_json = _tool_execute(args);
    } else if (tool_name == "get_log") {
        tool_result_json = _tool_get_log(args);
    } else if (tool_name == "play_scene") {
        tool_result_json = _tool_play_scene(args);
    } else if (tool_name == "stop_scene") {
        tool_result_json = _tool_stop_scene(args);
    } else if (tool_name == "save_scene") {
        tool_result_json = _tool_save_scene(args);
    } else if (tool_name == "save_all") {
        tool_result_json = _tool_save_all(args);
    } else if (tool_name == "open_scene") {
        tool_result_json = _tool_open_scene(args);
    } else if (tool_name == "get_setting") {
        tool_result_json = _tool_get_setting(args);
    } else if (tool_name == "set_setting") {
        tool_result_json = _tool_set_setting(args);
    } else if (tool_name == "resource_delete") {
        tool_result_json = _tool_resource_delete(args);
    } else if (tool_name == "resource_duplicate") {
        tool_result_json = _tool_resource_duplicate(args);
    } else if (tool_name == "resource_ensure") {
        tool_result_json = _tool_resource_ensure(args);
    } else if (tool_name == "batch") {
        tool_result_json = _tool_batch(args);
    } else {
        tool_result_json = _make_tool_result("Unknown tool: " + tool_name, true);
    }

    // tool_result_json is already a serialized JSON object for the MCP result.
    return JSON::parse_string(tool_result_json);
}

// ── Tool stubs (filled in Tasks 6–9) ─────────────────────────────────────────

String AgentBridgeServer::_tool_health(const Dictionary &args) {
    return _make_tool_result("TODO: implement in Task 6");
}
String AgentBridgeServer::_tool_execute(const Dictionary &args) {
    return _make_tool_result("TODO: implement in Task 6");
}
String AgentBridgeServer::_tool_get_log(const Dictionary &args) {
    return _make_tool_result("TODO: implement in Task 6");
}
String AgentBridgeServer::_tool_play_scene(const Dictionary &args) {
    return _make_tool_result("TODO: implement in Task 7");
}
String AgentBridgeServer::_tool_stop_scene(const Dictionary &args) {
    return _make_tool_result("TODO: implement in Task 7");
}
String AgentBridgeServer::_tool_save_scene(const Dictionary &args) {
    return _make_tool_result("TODO: implement in Task 7");
}
String AgentBridgeServer::_tool_save_all(const Dictionary &args) {
    return _make_tool_result("TODO: implement in Task 7");
}
String AgentBridgeServer::_tool_open_scene(const Dictionary &args) {
    return _make_tool_result("TODO: implement in Task 7");
}
String AgentBridgeServer::_tool_set_setting(const Dictionary &args) {
    return _make_tool_result("TODO: implement in Task 8");
}
String AgentBridgeServer::_tool_get_setting(const Dictionary &args) {
    return _make_tool_result("TODO: implement in Task 8");
}
String AgentBridgeServer::_tool_resource_delete(const Dictionary &args) {
    return _make_tool_result("TODO: implement in Task 8");
}
String AgentBridgeServer::_tool_resource_duplicate(const Dictionary &args) {
    return _make_tool_result("TODO: implement in Task 8");
}
String AgentBridgeServer::_tool_resource_ensure(const Dictionary &args) {
    return _make_tool_result("TODO: implement in Task 8");
}
String AgentBridgeServer::_tool_batch(const Dictionary &args) {
    return _make_tool_result("TODO: implement in Task 9");
}
```

- [ ] **Step 3: Compile**

```bash
cd D:/Projects/godot && scons platform=windows target=editor -j8
```

Expected: Compiles cleanly.

- [ ] **Step 4: Commit**

```bash
cd D:/Projects/godot-toolkit
git add AgentBridge/agent_bridge_server.h AgentBridge/agent_bridge_server.cpp
git commit -m "feat: add HTTP server + MCP JSON-RPC dispatcher (tool stubs)"
```

---

### Task 5: EditorPlugin (Wire Everything Together)

**Files:**
- Create: `godot-toolkit/AgentBridge/editor/agent_bridge_plugin.h`
- Create: `godot-toolkit/AgentBridge/editor/agent_bridge_plugin.cpp`
- Modify: `godot-toolkit/AgentBridge/register_types.cpp`

- [ ] **Step 1: Write `editor/agent_bridge_plugin.h`**

```cpp
// godot-toolkit/AgentBridge/editor/agent_bridge_plugin.h
#pragma once

#ifdef TOOLS_ENABLED

#include "editor/plugins/editor_plugin.h"
#include "../agent_bridge_executor.h"
#include "../agent_bridge_log_capture.h"
#include "../agent_bridge_server.h"

class AgentBridgePlugin : public EditorPlugin {
    GDCLASS(AgentBridgePlugin, EditorPlugin);

    AgentBridgeLogCapture *log_capture = nullptr;
    AgentBridgeExecutor *executor = nullptr;
    AgentBridgeServer *server = nullptr;

    void _write_port_file(uint16_t p_port);
    void _delete_port_file();
    static String _get_port_file_path();

protected:
    void _notification(int p_what);

public:
    AgentBridgePlugin();
    ~AgentBridgePlugin();
};

#endif // TOOLS_ENABLED
```

- [ ] **Step 2: Write `editor/agent_bridge_plugin.cpp`**

```cpp
// godot-toolkit/AgentBridge/editor/agent_bridge_plugin.cpp
#ifdef TOOLS_ENABLED

#include "agent_bridge_plugin.h"

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/os/os.h"

AgentBridgePlugin::AgentBridgePlugin() {}

AgentBridgePlugin::~AgentBridgePlugin() {
    // _notification(EXIT_TREE) handles cleanup; this is just a safety guard.
    if (server) {
        memdelete(server);
        server = nullptr;
    }
    if (executor) {
        memdelete(executor);
        executor = nullptr;
    }
    if (log_capture) {
        memdelete(log_capture);
        log_capture = nullptr;
    }
}

String AgentBridgePlugin::_get_port_file_path() {
    // Write port file next to the project's Saved directory.
    String project_dir = ProjectSettings::get_singleton()->get_resource_path();
    return project_dir + "/Saved/AgentBridge.port";
}

void AgentBridgePlugin::_write_port_file(uint16_t p_port) {
    String path = _get_port_file_path();
    // Ensure Saved/ dir exists.
    DirAccess::make_dir_absolute(path.get_base_dir());
    Ref<FileAccess> f = FileAccess::open(path, FileAccess::WRITE);
    if (f.is_valid()) {
        f->store_string(itos(p_port));
    }
}

void AgentBridgePlugin::_delete_port_file() {
    String path = _get_port_file_path();
    if (FileAccess::exists(path)) {
        DirAccess::remove_absolute(path);
    }
}

void AgentBridgePlugin::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_ENTER_TREE: {
            log_capture = memnew(AgentBridgeLogCapture(2000));
            executor = memnew(AgentBridgeExecutor(log_capture));
            server = memnew(AgentBridgeServer(log_capture, executor));

            Error err = server->start(8765);
            if (err == OK) {
                _write_port_file(server->get_port());
                print_line(vformat("[AgentBridge] MCP server listening on port %d", server->get_port()));
            } else {
                print_error("[AgentBridge] Failed to start MCP server — no available port in range 8765-8865.");
            }
            set_process(true);
        } break;

        case NOTIFICATION_PROCESS: {
            if (server) {
                server->poll();
            }
        } break;

        case NOTIFICATION_EXIT_TREE: {
            set_process(false);
            _delete_port_file();
            if (server) {
                server->stop();
                memdelete(server);
                server = nullptr;
            }
            if (executor) {
                memdelete(executor);
                executor = nullptr;
            }
            if (log_capture) {
                memdelete(log_capture);
                log_capture = nullptr;
            }
        } break;
    }
}

#endif // TOOLS_ENABLED
```

- [ ] **Step 3: Update `register_types.cpp` to register the plugin**

Replace the entire file:

```cpp
// godot-toolkit/AgentBridge/register_types.cpp
#include "register_types.h"

#ifdef TOOLS_ENABLED
#include "editor/agent_bridge_plugin.h"
#include "editor/plugins/editor_plugin.h"
#endif

void initialize_agent_bridge_module(ModuleInitializationLevel p_level) {
#ifdef TOOLS_ENABLED
    if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
        GDREGISTER_CLASS(AgentBridgePlugin);
        EditorPlugins::add_by_type<AgentBridgePlugin>();
    }
#endif
}

void uninitialize_agent_bridge_module(ModuleInitializationLevel p_level) {
    // Cleanup is handled by plugin destructor.
}
```

- [ ] **Step 4: Compile**

```bash
cd D:/Projects/godot && scons platform=windows target=editor -j8
```

Expected: Compiles cleanly.

- [ ] **Step 5: Smoke-test the plugin loads**

```bash
D:/Projects/godot/bin/godot.windows.editor.x86_64.exe --editor --path D:/GodotProjects/demo-host
```

Expected output in console: `[AgentBridge] MCP server listening on port 8765`

Also check port file was created:
```bash
cat "D:/GodotProjects/demo-host/Saved/AgentBridge.port"
```
Expected: `8765`

- [ ] **Step 6: Test health endpoint**

```bash
curl http://localhost:8765/agent/health
```

Expected:
```json
{"port":8765,"status":"ok","version":"4.7"}
```

- [ ] **Step 7: Commit**

```bash
cd D:/Projects/godot-toolkit
git add AgentBridge/editor/agent_bridge_plugin.h AgentBridge/editor/agent_bridge_plugin.cpp AgentBridge/register_types.cpp
git commit -m "feat: wire EditorPlugin — MCP server starts with editor, port file written"
```

---

### Task 6: Tools — health, execute, get_log

**Files:**
- Modify: `godot-toolkit/AgentBridge/agent_bridge_server.cpp` (replace three `_tool_*` stubs)

- [ ] **Step 1: Replace `_tool_health` stub**

In `agent_bridge_server.cpp`, replace:
```cpp
String AgentBridgeServer::_tool_health(const Dictionary &args) {
    return _make_tool_result("TODO: implement in Task 6");
}
```
With:
```cpp
String AgentBridgeServer::_tool_health(const Dictionary &args) {
    Dictionary info;
    info["status"] = "ok";
    info["port"] = (int)port;
    info["version"] = "4.7";
    info["log_entries"] = log_capture->get_recent(1).size() >= 0 ? "available" : "unavailable";
    return _make_tool_result(JSON::stringify(info));
}
```

- [ ] **Step 2: Replace `_tool_execute` stub**

Replace:
```cpp
String AgentBridgeServer::_tool_execute(const Dictionary &args) {
    return _make_tool_result("TODO: implement in Task 6");
}
```
With:
```cpp
String AgentBridgeServer::_tool_execute(const Dictionary &args) {
    String code = args.get("code", "");
    if (code.is_empty()) {
        return _make_tool_result("Missing required argument: code", true);
    }
    AgentExecResult res = executor->execute(code);
    if (!res.ok) {
        return _make_tool_result(res.error, true);
    }
    return _make_tool_result(res.output.is_empty() ? "(no output)" : res.output);
}
```

- [ ] **Step 3: Replace `_tool_get_log` stub**

Replace:
```cpp
String AgentBridgeServer::_tool_get_log(const Dictionary &args) {
    return _make_tool_result("TODO: implement in Task 6");
}
```
With:
```cpp
String AgentBridgeServer::_tool_get_log(const Dictionary &args) {
    int max_lines = (int)args.get("max_lines", 100);
    max_lines = CLAMP(max_lines, 1, 2000);
    Vector<AgentLogEntry> entries = log_capture->get_recent(max_lines);
    String out;
    for (int i = 0; i < entries.size(); i++) {
        if (entries[i].is_error) {
            out += "[ERROR] ";
        }
        out += entries[i].message + "\n";
    }
    return _make_tool_result(out.is_empty() ? "(no log entries)" : out);
}
```

- [ ] **Step 4: Compile**

```bash
cd D:/Projects/godot && scons platform=windows target=editor -j8
```

Expected: Compiles cleanly.

- [ ] **Step 5: Test MCP initialize + tools/list**

With editor open:
```bash
curl -s -X POST http://localhost:8765/mcp \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-03-26","capabilities":{},"clientInfo":{"name":"test","version":"1.0"}}}'
```

Expected: JSON response with `result.serverInfo.name = "godot-bridge"`.

```bash
curl -s -X POST http://localhost:8765/mcp \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":2,"method":"tools/list","params":{}}'
```

Expected: JSON response with `result.tools` array containing 14 tools including `"execute"`.

- [ ] **Step 6: Test execute tool**

```bash
curl -s -X POST http://localhost:8765/mcp \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":3,"method":"tools/call","params":{"name":"execute","arguments":{"code":"print(\"hello from AgentBridge\")"}}}'
```

Expected: `result.content[0].text` contains `"hello from AgentBridge"`.

- [ ] **Step 7: Commit**

```bash
cd D:/Projects/godot-toolkit
git add AgentBridge/agent_bridge_server.cpp
git commit -m "feat: implement health, execute, get_log MCP tools"
```

---

### Task 7: Tools — Editor Control (play, stop, save, open)

**Files:**
- Modify: `godot-toolkit/AgentBridge/agent_bridge_server.cpp` (replace four `_tool_*` stubs)

These tools need `EditorInterface`. Add the include at the top of `agent_bridge_server.cpp`:
```cpp
#include "editor/editor_interface.h"
```

- [ ] **Step 1: Replace `_tool_play_scene` stub**

Replace:
```cpp
String AgentBridgeServer::_tool_play_scene(const Dictionary &args) {
    return _make_tool_result("TODO: implement in Task 7");
}
```
With:
```cpp
String AgentBridgeServer::_tool_play_scene(const Dictionary &args) {
    EditorInterface *ei = EditorInterface::get_singleton();
    String scene_path = args.get("scene", "");
    if (!scene_path.is_empty()) {
        ei->open_scene_from_path(scene_path);
        ei->play_current_scene();
        return _make_tool_result("Playing scene: " + scene_path);
    }
    ei->play_main_scene();
    return _make_tool_result("Playing main scene.");
}
```

- [ ] **Step 2: Replace `_tool_stop_scene` stub**

Replace:
```cpp
String AgentBridgeServer::_tool_stop_scene(const Dictionary &args) {
    return _make_tool_result("TODO: implement in Task 7");
}
```
With:
```cpp
String AgentBridgeServer::_tool_stop_scene(const Dictionary &args) {
    EditorInterface::get_singleton()->stop_playing_scene();
    return _make_tool_result("Stopped.");
}
```

- [ ] **Step 3: Replace `_tool_save_scene` stub**

Replace:
```cpp
String AgentBridgeServer::_tool_save_scene(const Dictionary &args) {
    return _make_tool_result("TODO: implement in Task 7");
}
```
With:
```cpp
String AgentBridgeServer::_tool_save_scene(const Dictionary &args) {
    EditorInterface *ei = EditorInterface::get_singleton();
    String path = args.get("path", "");
    if (!path.is_empty()) {
        ei->open_scene_from_path(path);
    }
    Error err = ei->save_scene();
    if (err != OK) {
        return _make_tool_result("save_scene() failed (Error " + itos(err) + ")", true);
    }
    return _make_tool_result("Scene saved.");
}
```

- [ ] **Step 4: Replace `_tool_save_all` stub**

Replace:
```cpp
String AgentBridgeServer::_tool_save_all(const Dictionary &args) {
    return _make_tool_result("TODO: implement in Task 7");
}
```
With:
```cpp
String AgentBridgeServer::_tool_save_all(const Dictionary &args) {
    EditorInterface::get_singleton()->save_all_scenes();
    return _make_tool_result("All scenes saved.");
}
```

- [ ] **Step 5: Replace `_tool_open_scene` stub**

Replace:
```cpp
String AgentBridgeServer::_tool_open_scene(const Dictionary &args) {
    return _make_tool_result("TODO: implement in Task 7");
}
```
With:
```cpp
String AgentBridgeServer::_tool_open_scene(const Dictionary &args) {
    String path = args.get("path", "");
    if (path.is_empty()) {
        return _make_tool_result("Missing required argument: path", true);
    }
    EditorInterface::get_singleton()->open_scene_from_path(path);
    return _make_tool_result("Opened: " + path);
}
```

- [ ] **Step 6: Compile**

```bash
cd D:/Projects/godot && scons platform=windows target=editor -j8
```

Expected: Compiles cleanly.

- [ ] **Step 7: Test play_scene**

With editor open and demo-host loaded:
```bash
curl -s -X POST http://localhost:8765/mcp \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":4,"method":"tools/call","params":{"name":"play_scene","arguments":{}}}'
```

Expected: Editor starts playing. Response text contains `"Playing main scene."`.

```bash
curl -s -X POST http://localhost:8765/mcp \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":5,"method":"tools/call","params":{"name":"stop_scene","arguments":{}}}'
```

Expected: Editor stops playing. Response text contains `"Stopped."`.

- [ ] **Step 8: Commit**

```bash
cd D:/Projects/godot-toolkit
git add AgentBridge/agent_bridge_server.cpp
git commit -m "feat: implement play_scene, stop_scene, save_scene, save_all, open_scene tools"
```

---

### Task 8: Tools — Settings + Resources

**Files:**
- Modify: `godot-toolkit/AgentBridge/agent_bridge_server.cpp` (replace five `_tool_*` stubs)

Add includes at the top of `agent_bridge_server.cpp`:
```cpp
#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "editor/editor_file_system.h"
```

- [ ] **Step 1: Replace `_tool_get_setting` stub**

Replace:
```cpp
String AgentBridgeServer::_tool_get_setting(const Dictionary &args) {
    return _make_tool_result("TODO: implement in Task 8");
}
```
With:
```cpp
String AgentBridgeServer::_tool_get_setting(const Dictionary &args) {
    String key = args.get("key", "");
    if (key.is_empty()) {
        return _make_tool_result("Missing required argument: key", true);
    }
    if (!ProjectSettings::get_singleton()->has_setting(key)) {
        return _make_tool_result("Setting not found: " + key, true);
    }
    Variant value = ProjectSettings::get_singleton()->get_setting(key);
    Dictionary out;
    out["key"] = key;
    out["value"] = value;
    return _make_tool_result(JSON::stringify(out));
}
```

- [ ] **Step 2: Replace `_tool_set_setting` stub**

Replace:
```cpp
String AgentBridgeServer::_tool_set_setting(const Dictionary &args) {
    return _make_tool_result("TODO: implement in Task 8");
}
```
With:
```cpp
String AgentBridgeServer::_tool_set_setting(const Dictionary &args) {
    String key = args.get("key", "");
    if (key.is_empty()) {
        return _make_tool_result("Missing required argument: key", true);
    }
    if (!args.has("value")) {
        return _make_tool_result("Missing required argument: value", true);
    }
    Variant value = args["value"];
    ProjectSettings::get_singleton()->set_setting(key, value);
    Error err = ProjectSettings::get_singleton()->save();
    if (err != OK) {
        return _make_tool_result("Setting updated but save() failed (Error " + itos(err) + ")", true);
    }
    return _make_tool_result("Setting saved: " + key);
}
```

- [ ] **Step 3: Replace `_tool_resource_delete` stub**

Replace:
```cpp
String AgentBridgeServer::_tool_resource_delete(const Dictionary &args) {
    return _make_tool_result("TODO: implement in Task 8");
}
```
With:
```cpp
String AgentBridgeServer::_tool_resource_delete(const Dictionary &args) {
    String path = args.get("path", "");
    if (path.is_empty()) {
        return _make_tool_result("Missing required argument: path", true);
    }
    String abs_path = ProjectSettings::get_singleton()->globalize_path(path);
    Error err = DirAccess::remove_absolute(abs_path);
    if (err != OK) {
        return _make_tool_result("Failed to delete " + path + " (Error " + itos(err) + ")", true);
    }
    EditorFileSystem::get_singleton()->update_file(path);
    return _make_tool_result("Deleted: " + path);
}
```

- [ ] **Step 4: Replace `_tool_resource_duplicate` stub**

Replace:
```cpp
String AgentBridgeServer::_tool_resource_duplicate(const Dictionary &args) {
    return _make_tool_result("TODO: implement in Task 8");
}
```
With:
```cpp
String AgentBridgeServer::_tool_resource_duplicate(const Dictionary &args) {
    String src = args.get("src", "");
    String dst = args.get("dst", "");
    if (src.is_empty() || dst.is_empty()) {
        return _make_tool_result("Missing required arguments: src, dst", true);
    }
    String abs_src = ProjectSettings::get_singleton()->globalize_path(src);
    String abs_dst = ProjectSettings::get_singleton()->globalize_path(dst);
    // Ensure destination directory exists.
    DirAccess::make_dir_recursive_absolute(abs_dst.get_base_dir());
    Error err = DirAccess::copy_absolute(abs_src, abs_dst);
    if (err != OK) {
        return _make_tool_result("Failed to copy " + src + " → " + dst + " (Error " + itos(err) + ")", true);
    }
    EditorFileSystem::get_singleton()->update_file(dst);
    return _make_tool_result("Duplicated: " + src + " → " + dst);
}
```

- [ ] **Step 5: Replace `_tool_resource_ensure` stub**

Replace:
```cpp
String AgentBridgeServer::_tool_resource_ensure(const Dictionary &args) {
    return _make_tool_result("TODO: implement in Task 8");
}
```
With:
```cpp
String AgentBridgeServer::_tool_resource_ensure(const Dictionary &args) {
    String path = args.get("path", "");
    String type = args.get("type", "Resource");
    if (path.is_empty()) {
        return _make_tool_result("Missing required argument: path", true);
    }
    if (FileAccess::exists(path)) {
        return _make_tool_result("Already exists: " + path);
    }
    String abs_path = ProjectSettings::get_singleton()->globalize_path(path);
    DirAccess::make_dir_recursive_absolute(abs_path.get_base_dir());

    // Write minimal content based on type.
    Ref<FileAccess> f = FileAccess::open(abs_path, FileAccess::WRITE);
    if (!f.is_valid()) {
        return _make_tool_result("Could not create file: " + path, true);
    }
    if (type == "GDScript") {
        f->store_string("extends RefCounted\n");
    } else {
        // Generic Godot resource text format header.
        f->store_string("[gd_resource type=\"" + type + "\" format=3]\n");
    }
    f.unref();

    EditorFileSystem::get_singleton()->update_file(path);
    return _make_tool_result("Created: " + path + " (type: " + type + ")");
}
```

- [ ] **Step 6: Compile**

```bash
cd D:/Projects/godot && scons platform=windows target=editor -j8
```

Expected: Compiles cleanly.

- [ ] **Step 7: Test get_setting**

```bash
curl -s -X POST http://localhost:8765/mcp \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":6,"method":"tools/call","params":{"name":"get_setting","arguments":{"key":"application/config/name"}}}'
```

Expected: `result.content[0].text` contains `"DemoHost"`.

- [ ] **Step 8: Commit**

```bash
cd D:/Projects/godot-toolkit
git add AgentBridge/agent_bridge_server.cpp
git commit -m "feat: implement settings and resource management MCP tools"
```

---

### Task 9: Tool — batch

**Files:**
- Modify: `godot-toolkit/AgentBridge/agent_bridge_server.cpp` (replace `_tool_batch` stub)

- [ ] **Step 1: Replace `_tool_batch` stub**

Replace:
```cpp
String AgentBridgeServer::_tool_batch(const Dictionary &args) {
    return _make_tool_result("TODO: implement in Task 9");
}
```
With:
```cpp
String AgentBridgeServer::_tool_batch(const Dictionary &args) {
    Variant calls_var = args.get("calls", Variant());
    if (calls_var.get_type() != Variant::ARRAY) {
        return _make_tool_result("Missing or invalid argument: calls (must be array)", true);
    }
    Array calls = calls_var.operator Array();

    Array results;
    for (int i = 0; i < calls.size(); i++) {
        if (calls[i].get_type() != Variant::DICTIONARY) {
            Dictionary err_item;
            err_item["index"] = i;
            err_item["isError"] = true;
            err_item["text"] = "Each call must be a {tool, arguments} object.";
            results.push_back(err_item);
            continue;
        }
        Dictionary call = calls[i].operator Dictionary();
        String tool_name = call.get("name", "");
        Dictionary tool_args = call.get("arguments", Dictionary());

        // Re-use the tools/call dispatch by building a fake params dict.
        Dictionary fake_params;
        fake_params["name"] = tool_name;
        fake_params["arguments"] = tool_args;

        Variant tool_result_var = _method_tools_call(Variant(), fake_params);
        Dictionary item;
        item["index"] = i;
        item["tool"] = tool_name;
        item["result"] = tool_result_var;
        results.push_back(item);
    }

    Dictionary out;
    out["results"] = results;
    return _make_tool_result(JSON::stringify(out));
}
```

- [ ] **Step 2: Compile**

```bash
cd D:/Projects/godot && scons platform=windows target=editor -j8
```

Expected: Compiles cleanly.

- [ ] **Step 3: Test batch**

```bash
curl -s -X POST http://localhost:8765/mcp \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":7,"method":"tools/call","params":{"name":"batch","arguments":{"calls":[{"name":"health","arguments":{}},{"name":"execute","arguments":{"code":"print(2 + 2)"}}]}}}'
```

Expected: `result.content[0].text` is JSON with `results` array of 2 items. Second item's result text contains `"4"`.

- [ ] **Step 4: Commit**

```bash
cd D:/Projects/godot-toolkit
git add AgentBridge/agent_bridge_server.cpp
git commit -m "feat: implement batch tool (sequential multi-tool dispatch)"
```

---

### Task 10: Install Scripts

**Files:**
- Create: `godot-toolkit/install.bat`
- Create: `godot-toolkit/install.sh`

- [ ] **Step 1: Write `install.bat`**

```bat
@echo off
setlocal

set GODOT_MODULES=D:\Projects\godot\modules\agent_bridge
set TOOLKIT_DIR=%~dp0AgentBridge

echo [AgentBridge] Linking module...
if exist "%GODOT_MODULES%" (
    rmdir "%GODOT_MODULES%"
)
mklink /J "%GODOT_MODULES%" "%TOOLKIT_DIR%"
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to create junction. Run as Administrator.
    exit /b 1
)
echo [AgentBridge] Module linked: %GODOT_MODULES%

echo.
echo [AgentBridge] Registering MCP server with Claude Code...
claude mcp add godot-bridge --url http://localhost:8765/mcp
echo [AgentBridge] MCP server registered as "godot-bridge".

echo.
echo [AgentBridge] Done. Next steps:
echo   1. Rebuild Godot: cd D:\Projects\godot ^&^& scons platform=windows target=editor -j8
echo   2. Open demo-host: D:\GodotProjects\demo-host
echo   3. Verify: claude mcp list

endlocal
```

- [ ] **Step 2: Write `install.sh`**

```bash
#!/usr/bin/env bash
set -euo pipefail

GODOT_MODULES="D:/Projects/godot/modules/agent_bridge"
TOOLKIT_DIR="$(cd "$(dirname "$0")/AgentBridge" && pwd)"

echo "[AgentBridge] Linking module..."
if [ -L "$GODOT_MODULES" ] || [ -e "$GODOT_MODULES" ]; then
    rm -rf "$GODOT_MODULES"
fi
ln -s "$TOOLKIT_DIR" "$GODOT_MODULES"
echo "[AgentBridge] Module linked: $GODOT_MODULES"

echo ""
echo "[AgentBridge] Registering MCP server with Claude Code..."
claude mcp add godot-bridge --url http://localhost:8765/mcp
echo "[AgentBridge] MCP server registered as 'godot-bridge'."

echo ""
echo "[AgentBridge] Done. Next steps:"
echo "  1. Rebuild Godot: cd D:/Projects/godot && scons platform=windows target=editor -j8"
echo "  2. Open demo-host: D:/GodotProjects/demo-host"
echo "  3. Verify: claude mcp list"
```

- [ ] **Step 3: Test install.bat (as Administrator)**

```powershell
D:\Projects\godot-toolkit\install.bat
```

Expected output ends with `claude mcp list` showing `godot-bridge`.

- [ ] **Step 4: Commit**

```bash
cd D:/Projects/godot-toolkit
git add install.bat install.sh
git commit -m "feat: add install scripts (junction + claude mcp registration)"
```

---

### Task 11: Skills

**Files:**
- Create: `godot-toolkit/skills/godot-coder/skill.md`
- Create: `godot-toolkit/skills/godot-scene/skill.md`
- Create: `godot-toolkit/skills/godot-debugger/skill.md`
- Create: `godot-toolkit/skills/godot-architect/skill.md`
- Create: `godot-toolkit/skills/godot-ui/skill.md`
- Create: `godot-toolkit/skills/godot-shader/skill.md`
- Create: `godot-toolkit/skills/godot-signals/skill.md`
- Create: `godot-toolkit/skills/godot-runner/skill.md`

- [ ] **Step 1: Write `skills/godot-coder/skill.md`**

```markdown
---
name: godot-coder
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
```

- [ ] **Step 2: Write `skills/godot-scene/skill.md`**

```markdown
---
name: godot-scene
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
root.add_child(node, true)  # true = set owner for serialization
node.owner = root
```

**Find a node by name:**
```gdscript
var root = EditorInterface.get_edited_scene_root()
var node = root.find_child("NodeName", true, false)
print(node)
```

**Save after changes:**
Always follow scene manipulation with `save_scene` to persist changes.

## Notes

- Always set `node.owner = root` after `add_child` or the node won't be saved.
- Use `EditorInterface.get_selection()` to get currently selected nodes.
- Use `EditorUndoRedoManager` for undoable operations (advanced).
```

- [ ] **Step 3: Write `skills/godot-debugger/skill.md`**

```markdown
---
name: godot-debugger
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
   var node = root.get_node_or_null("Path/To/Node")
   print("Found: ", node, " class: ", node.get_class() if node else "null")
   ''')
   ```

4. **Check project settings:**
   ```
   get_setting(key="physics/3d/physics_engine")
   ```

5. **Fix → save → play → check log:**
   ```
   save_scene() → play_scene() → get_log()
   ```

## Common GDScript Errors

| Error | Cause | Fix |
|-------|-------|-----|
| `Invalid get index 'x' on base 'Nil'` | Node not found via `$Path` | Use `@onready` or check node exists |
| `Cannot call method on null` | `@onready` var not set | Ensure node path is correct |
| `Cyclic reference` | Signal connected to itself | Check signal connections |
```

- [ ] **Step 4: Write `skills/godot-architect/skill.md`**

```markdown
---
name: godot-architect
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

## Checking current structure

```
execute(code='''
import DirAccess
var da = DirAccess.open("res://")
print(da.get_directories())
''')
```
```

- [ ] **Step 5: Write `skills/godot-ui/skill.md`**

```markdown
---
name: godot-ui
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

**Theme lookup:**
```gdscript
var font_size: int = get_theme_font_size("font_size", "Label")
```
```

- [ ] **Step 6: Write `skills/godot-shader/skill.md`**

```markdown
---
name: godot-shader
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

uniform sampler2D noise_tex : hint_default_white;
uniform float speed : hint_range(0.0, 5.0) = 1.0;

void fragment() {
    vec2 uv = UV + vec2(TIME * speed, 0.0);
    COLOR = texture(TEXTURE, UV) * texture(noise_tex, uv);
}
```

## Testing Shaders via AgentBridge

```
execute(code='''
var mat = StandardMaterial3D.new()
mat.albedo_color = Color(1, 0, 0)
EditorInterface.get_edited_scene_root().get_child(0).material_override = mat
''')
```
```

- [ ] **Step 7: Write `skills/godot-signals/skill.md`**

```markdown
---
name: godot-signals
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
    %HealthLabel.text = str(new_health)
```

## Inspect connections via AgentBridge

```
execute(code='''
var node = EditorInterface.get_edited_scene_root().find_child("MyNode", true, false)
for sig in node.get_signal_list():
    var conns = node.get_signal_connection_list(sig.name)
    if conns.size() > 0:
        print(sig.name, " → ", conns)
''')
```
```

- [ ] **Step 8: Write `skills/godot-runner/skill.md`**

```markdown
---
name: godot-runner
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
# Inspect project settings
print(ProjectSettings.get_setting("application/config/name"))
print(OS.get_name())
print(Engine.get_version_info())
''')
```

## Common diagnostic executes

**List all nodes in edited scene:**
```
execute(code='''
func list_nodes(node, indent=0):
    print("  " * indent + node.name + " [" + node.get_class() + "]")
    for child in node.get_children():
        list_nodes(child, indent + 1)
list_nodes(EditorInterface.get_edited_scene_root())
''')
```

**Check autoloads:**
```
execute(code='''
print(ProjectSettings.get_setting("autoload/GameManager"))
''')
```
```

- [ ] **Step 9: Commit**

```bash
cd D:/Projects/godot-toolkit
git add skills/
git commit -m "feat: add 8 Godot skills (coder, scene, debugger, architect, ui, shader, signals, runner)"
```

---

### Task 12: End-to-End Validation

**Goal:** Confirm the full system works from Claude Code → MCP → Godot editor.

- [ ] **Step 1: Run install**

```powershell
# As Administrator
D:\Projects\godot-toolkit\install.bat
```

Expected: Module linked, MCP registered.

- [ ] **Step 2: Compile Godot with the module**

```bash
cd D:/Projects/godot && scons platform=windows target=editor -j8
```

Expected: Clean build.

- [ ] **Step 3: Launch editor with demo-host**

```bash
D:/Projects/godot/bin/godot.windows.editor.x86_64.exe --editor --path D:/GodotProjects/demo-host
```

Expected: Console shows `[AgentBridge] MCP server listening on port 8765`.

- [ ] **Step 4: Verify port file**

```bash
cat "D:/GodotProjects/demo-host/Saved/AgentBridge.port"
```

Expected: `8765`

- [ ] **Step 5: Verify Claude Code sees the MCP server**

```bash
claude mcp list
```

Expected: `godot-bridge` listed.

- [ ] **Step 6: Run end-to-end MCP sequence**

```bash
# 1. Initialize
curl -s -X POST http://localhost:8765/mcp -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-03-26","capabilities":{},"clientInfo":{"name":"test","version":"1"}}}'

# 2. List tools
curl -s -X POST http://localhost:8765/mcp -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":2,"method":"tools/list","params":{}}'

# 3. Execute GDScript
curl -s -X POST http://localhost:8765/mcp -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":3,"method":"tools/call","params":{"name":"execute","arguments":{"code":"print(\"AgentBridge works! Godot \", Engine.get_version_info().string)"}}}'

# 4. Get project name
curl -s -X POST http://localhost:8765/mcp -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":4,"method":"tools/call","params":{"name":"get_setting","arguments":{"key":"application/config/name"}}}'
```

Expected for step 3: `content[0].text` contains `"AgentBridge works! Godot 4.7"`.
Expected for step 4: `content[0].text` contains `"DemoHost"`.

- [ ] **Step 7: Final commit**

```bash
cd D:/Projects/godot-toolkit
git add .
git commit -m "feat: Godot AgentBridge v1.0 — C++ MCP server, 14 tools, 8 skills"
```
