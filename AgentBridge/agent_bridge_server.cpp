// godot-toolkit/AgentBridge/agent_bridge_server.cpp
#include "agent_bridge_server.h"

#include "core/io/ip_address.h"
#include "core/io/json.h"
#include "core/os/os.h"
#include "core/string/ustring.h"
#include "core/variant/array.h"
#include "core/variant/dictionary.h"

#ifdef TOOLS_ENABLED
#include "editor/file_system/editor_file_system.h"
#include "editor/editor_interface.h"
#endif
#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"

// ── Section A — startup / shutdown ───────────────────────────────────────────

AgentBridgeServer::AgentBridgeServer(AgentBridgeLogCapture *p_log, AgentBridgeExecutor *p_exec) :
        log_capture(p_log), executor(p_exec) {
    tcp_server.instantiate();
}

AgentBridgeServer::~AgentBridgeServer() {
    stop();
}

Error AgentBridgeServer::start() {
    Error err = tcp_server->listen(0, IPAddress("127.0.0.1"));
    if (err == OK) {
        port = tcp_server->get_local_port();
        return OK;
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

// ── Section B — poll loop ─────────────────────────────────────────────────────

void AgentBridgeServer::poll() {
    if (!is_running()) {
        return;
    }

    while (tcp_server->is_connection_available()) {
        Ref<StreamPeerTCP> peer = tcp_server->take_connection();
        if (peer.is_valid()) {
            AgentConnection conn;
            conn.peer = peer;
            connections.push_back(conn);
        }
    }

    for (int i = connections.size() - 1; i >= 0; i--) {
        AgentConnection &c = connections.write[i];

        if (c.peer->get_status() != StreamPeerTCP::STATUS_CONNECTED) {
            connections.remove_at(i);
            continue;
        }

        int available = c.peer->get_available_bytes();
        if (available > 0) {
            int old_size = c.recv_buf.size();
            c.recv_buf.resize(old_size + available);
            int received = 0;
            c.peer->get_partial_data(c.recv_buf.ptrw() + old_size, available, received);
            c.recv_buf.resize(old_size + received);
        }

        if (!c.headers_done) {
            if (!_parse_headers(c)) {
                continue;
            }
        }

        int body_start = c.header_end;
        int body_needed = MAX(0, c.content_length);
        if (c.recv_buf.size() < body_start + body_needed) {
            continue;
        }

        String body;
        if (body_needed > 0) {
            body = String::utf8(
                    (const char *)(c.recv_buf.ptr() + body_start),
                    body_needed);
        }

        String http_response = _handle_request(c.method, c.path, body);
        if (!http_response.is_empty()) {
            PackedByteArray bytes = http_response.to_utf8_buffer();
            c.peer->put_data(bytes.ptr(), bytes.size());
        }
        c.peer->disconnect_from_host();
        connections.remove_at(i);
    }
}

// ── Section C — HTTP header parser ───────────────────────────────────────────

bool AgentBridgeServer::_parse_headers(AgentConnection &c) {
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
        return false;
    }

    c.header_end = header_end;
    c.headers_done = true;

    String headers_str = String::utf8((const char *)data, header_end);
    PackedStringArray lines = headers_str.split("\r\n");
    if (lines.is_empty()) {
        return false;
    }

    PackedStringArray req_parts = lines[0].split(" ");
    if (req_parts.size() >= 2) {
        c.method = req_parts[0].to_upper();
        c.path = req_parts[1];
    }

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

// ── Section D — HTTP response builder + REST router ──────────────────────────

String AgentBridgeServer::_build_http_response(int status_code, const String &body,
        const String &content_type) {
    String status_text;
    switch (status_code) {
        case 200: status_text = "OK"; break;
        case 400: status_text = "Bad Request"; break;
        case 404: status_text = "Not Found"; break;
        default:  status_text = "Error"; break;
    }
    PackedByteArray body_bytes = body.to_utf8_buffer();
    return String("HTTP/1.1 ") + itos(status_code) + " " + status_text + "\r\n" +
            "Content-Type: " + content_type + "\r\n" +
            "Content-Length: " + itos(body_bytes.size()) + "\r\n" +
            "Access-Control-Allow-Origin: *\r\n" +
            "Connection: close\r\n" +
            "\r\n" + body;
}

String AgentBridgeServer::_rest_ok() {
    return R"({"ok":true})";
}

String AgentBridgeServer::_rest_error(const String &msg) {
    Dictionary d;
    d["error"] = msg;
    return JSON::stringify(d);
}

String AgentBridgeServer::_rest_output(const String &text) {
    Dictionary d;
    d["output"] = text;
    return JSON::stringify(d);
}

bool AgentBridgeServer::_is_error(const String &json) {
    Variant v = JSON::parse_string(json);
    if (v.get_type() != Variant::DICTIONARY) {
        return false;
    }
    return v.operator Dictionary().has("error");
}

String AgentBridgeServer::_handle_request(const String &method, const String &path, const String &body) {
    if (method == "OPTIONS") {
        return _build_http_response(200, "");
    }

    // Parse JSON body if present.
    Dictionary args;
    if (!body.is_empty()) {
        Variant parsed = JSON::parse_string(body);
        if (parsed.get_type() == Variant::DICTIONARY) {
            args = parsed.operator Dictionary();
        }
    }

    // Route table.
    if (path == "/agent/health") {
        return _build_http_response(200, _tool_health(args));
    }
    if (path == "/agent/execute" && method == "POST") {
        String r = _tool_execute(args);
        return _build_http_response(_is_error(r) ? 400 : 200, r);
    }
    if (path == "/agent/log" && method == "POST") {
        return _build_http_response(200, _tool_get_log(args));
    }
    if (path == "/agent/play" && method == "POST") {
        String r = _tool_play_scene(args);
        return _build_http_response(_is_error(r) ? 400 : 200, r);
    }
    if (path == "/agent/stop" && method == "POST") {
        String r = _tool_stop_scene(args);
        return _build_http_response(_is_error(r) ? 400 : 200, r);
    }
    if (path == "/agent/save" && method == "POST") {
        String r = _tool_save_scene(args);
        return _build_http_response(_is_error(r) ? 400 : 200, r);
    }
    if (path == "/agent/save-all" && method == "POST") {
        String r = _tool_save_all(args);
        return _build_http_response(_is_error(r) ? 400 : 200, r);
    }
    if (path == "/agent/open" && method == "POST") {
        String r = _tool_open_scene(args);
        return _build_http_response(_is_error(r) ? 400 : 200, r);
    }
    if (path == "/agent/setting/get" && method == "POST") {
        String r = _tool_get_setting(args);
        return _build_http_response(_is_error(r) ? 400 : 200, r);
    }
    if (path == "/agent/setting/set" && method == "POST") {
        String r = _tool_set_setting(args);
        return _build_http_response(_is_error(r) ? 400 : 200, r);
    }
    if (path == "/agent/resource/delete" && method == "POST") {
        String r = _tool_resource_delete(args);
        return _build_http_response(_is_error(r) ? 400 : 200, r);
    }
    if (path == "/agent/resource/duplicate" && method == "POST") {
        String r = _tool_resource_duplicate(args);
        return _build_http_response(_is_error(r) ? 400 : 200, r);
    }
    if (path == "/agent/resource/ensure" && method == "POST") {
        String r = _tool_resource_ensure(args);
        return _build_http_response(_is_error(r) ? 400 : 200, r);
    }
    if (path == "/agent/batch" && method == "POST") {
        String r = _tool_batch(args);
        return _build_http_response(_is_error(r) ? 400 : 200, r);
    }

    return _build_http_response(404, R"({"error":"not found"})");
}

// ── Section E — tool implementations ─────────────────────────────────────────

String AgentBridgeServer::_tool_health(const Dictionary &args) {
    Dictionary info;
    info["status"] = "ok";
    info["port"] = (int)port;
    info["version"] = "4.7";
    return JSON::stringify(info);
}

String AgentBridgeServer::_tool_execute(const Dictionary &args) {
    String code = args.get("code", "");
    if (code.is_empty()) {
        return _rest_error("Missing required field: code");
    }
    AgentExecResult res = executor->execute(code);
    if (!res.ok) {
        return _rest_error(res.error);
    }
    return _rest_output(res.output.is_empty() ? "(no output)" : res.output);
}

String AgentBridgeServer::_tool_get_log(const Dictionary &args) {
    int max_lines = (int)args.get("lines", 100);
    max_lines = CLAMP(max_lines, 1, 2000);
    Vector<AgentLogEntry> entries = log_capture->get_recent(max_lines);
    String out;
    for (int i = 0; i < entries.size(); i++) {
        if (entries[i].is_error) {
            out += "[ERROR] ";
        }
        out += entries[i].message + "\n";
    }
    return _rest_output(out.is_empty() ? "(no log entries)" : out);
}

String AgentBridgeServer::_tool_play_scene(const Dictionary &args) {
#ifdef TOOLS_ENABLED
    EditorInterface *ei = EditorInterface::get_singleton();
    String scene_path = args.get("scene", "");
    if (!scene_path.is_empty()) {
        ei->open_scene_from_path(scene_path);
        ei->play_current_scene();
    } else {
        ei->play_main_scene();
    }
    return _rest_ok();
#else
    return _rest_error("Editor not available.");
#endif
}

String AgentBridgeServer::_tool_stop_scene(const Dictionary &args) {
#ifdef TOOLS_ENABLED
    EditorInterface::get_singleton()->stop_playing_scene();
    return _rest_ok();
#else
    return _rest_error("Editor not available.");
#endif
}

String AgentBridgeServer::_tool_save_scene(const Dictionary &args) {
#ifdef TOOLS_ENABLED
    EditorInterface *ei = EditorInterface::get_singleton();
    String path = args.get("path", "");
    if (!path.is_empty()) {
        ei->open_scene_from_path(path);
    }
    Error err = ei->save_scene();
    if (err != OK) {
        return _rest_error("save_scene() failed (Error " + itos((int)err) + ")");
    }
    return _rest_ok();
#else
    return _rest_error("Editor not available.");
#endif
}

String AgentBridgeServer::_tool_save_all(const Dictionary &args) {
#ifdef TOOLS_ENABLED
    EditorInterface::get_singleton()->save_all_scenes();
    return _rest_ok();
#else
    return _rest_error("Editor not available.");
#endif
}

String AgentBridgeServer::_tool_open_scene(const Dictionary &args) {
#ifdef TOOLS_ENABLED
    String path = args.get("path", "");
    if (path.is_empty()) {
        return _rest_error("Missing required field: path");
    }
    EditorInterface::get_singleton()->open_scene_from_path(path);
    return _rest_ok();
#else
    return _rest_error("Editor not available.");
#endif
}

String AgentBridgeServer::_tool_get_setting(const Dictionary &args) {
    String key = args.get("key", "");
    if (key.is_empty()) {
        return _rest_error("Missing required field: key");
    }
    if (!ProjectSettings::get_singleton()->has_setting(key)) {
        return _rest_error("Setting not found: " + key);
    }
    Variant value = ProjectSettings::get_singleton()->get_setting(key);
    Dictionary out;
    out["key"] = key;
    out["value"] = value;
    return JSON::stringify(out);
}

String AgentBridgeServer::_tool_set_setting(const Dictionary &args) {
    String key = args.get("key", "");
    if (key.is_empty()) {
        return _rest_error("Missing required field: key");
    }
    if (!args.has("value")) {
        return _rest_error("Missing required field: value");
    }
    ProjectSettings::get_singleton()->set_setting(key, args["value"]);
    Error err = ProjectSettings::get_singleton()->save();
    if (err != OK) {
        return _rest_error("Setting updated but save() failed (Error " + itos((int)err) + ")");
    }
    return _rest_ok();
}

String AgentBridgeServer::_tool_resource_delete(const Dictionary &args) {
    String path = args.get("path", "");
    if (path.is_empty()) {
        return _rest_error("Missing required field: path");
    }
    String abs_path = ProjectSettings::get_singleton()->globalize_path(path);
    Error err = DirAccess::remove_absolute(abs_path);
    if (err != OK) {
        return _rest_error("Failed to delete " + path + " (Error " + itos((int)err) + ")");
    }
#ifdef TOOLS_ENABLED
    EditorFileSystem::get_singleton()->update_file(path);
#endif
    return _rest_ok();
}

String AgentBridgeServer::_tool_resource_duplicate(const Dictionary &args) {
    String src = args.get("src", "");
    String dst = args.get("dst", "");
    if (src.is_empty() || dst.is_empty()) {
        return _rest_error("Missing required fields: src, dst");
    }
    String abs_src = ProjectSettings::get_singleton()->globalize_path(src);
    String abs_dst = ProjectSettings::get_singleton()->globalize_path(dst);
    DirAccess::make_dir_recursive_absolute(abs_dst.get_base_dir());
    Error err = DirAccess::copy_absolute(abs_src, abs_dst);
    if (err != OK) {
        return _rest_error("Failed to copy " + src + " -> " + dst + " (Error " + itos((int)err) + ")");
    }
#ifdef TOOLS_ENABLED
    EditorFileSystem::get_singleton()->update_file(dst);
#endif
    return _rest_ok();
}

String AgentBridgeServer::_tool_resource_ensure(const Dictionary &args) {
    String path = args.get("path", "");
    String type = args.get("type", "Resource");
    if (path.is_empty()) {
        return _rest_error("Missing required field: path");
    }
    if (FileAccess::exists(path)) {
        return _rest_ok();
    }
    String abs_path = ProjectSettings::get_singleton()->globalize_path(path);
    DirAccess::make_dir_recursive_absolute(abs_path.get_base_dir());
    Ref<FileAccess> f = FileAccess::open(abs_path, FileAccess::WRITE);
    if (!f.is_valid()) {
        return _rest_error("Could not create file: " + path);
    }
    if (type == "GDScript") {
        f->store_string("extends RefCounted\n");
    } else {
        f->store_string("[gd_resource type=\"" + type + "\" format=3]\n");
    }
    f.unref();
#ifdef TOOLS_ENABLED
    EditorFileSystem::get_singleton()->update_file(path);
#endif
    return _rest_ok();
}

// ── Section F — batch + dispatch ─────────────────────────────────────────────

String AgentBridgeServer::_dispatch_tool(const String &tool, const Dictionary &args) {
    if (tool == "health")              return _tool_health(args);
    if (tool == "execute")             return _tool_execute(args);
    if (tool == "log")                 return _tool_get_log(args);
    if (tool == "play")                return _tool_play_scene(args);
    if (tool == "stop")                return _tool_stop_scene(args);
    if (tool == "save")                return _tool_save_scene(args);
    if (tool == "save-all")            return _tool_save_all(args);
    if (tool == "open")                return _tool_open_scene(args);
    if (tool == "setting/get")         return _tool_get_setting(args);
    if (tool == "setting/set")         return _tool_set_setting(args);
    if (tool == "resource/delete")     return _tool_resource_delete(args);
    if (tool == "resource/duplicate")  return _tool_resource_duplicate(args);
    if (tool == "resource/ensure")     return _tool_resource_ensure(args);
    return _rest_error("Unknown tool: " + tool);
}

String AgentBridgeServer::_tool_batch(const Dictionary &args) {
    Variant calls_var = args.get("calls", Variant());
    if (calls_var.get_type() != Variant::ARRAY) {
        return _rest_error("Missing or invalid field: calls (must be array)");
    }
    Array calls = calls_var.operator Array();
    Array results;
    for (int i = 0; i < calls.size(); i++) {
        if (calls[i].get_type() != Variant::DICTIONARY) {
            Dictionary err_item;
            err_item["index"] = i;
            err_item["error"] = "Each call must be a {tool, args} object.";
            results.push_back(err_item);
            continue;
        }
        Dictionary call = calls[i].operator Dictionary();
        String tool = call.get("tool", "");
        Dictionary call_args = call.get("args", Dictionary());
        String result_json = _dispatch_tool(tool, call_args);
        Variant result_var = JSON::parse_string(result_json);
        Dictionary item;
        item["index"] = i;
        item["tool"] = tool;
        item["result"] = result_var;
        results.push_back(item);
    }
    Dictionary out;
    out["results"] = results;
    return JSON::stringify(out);
}
