// godot-toolkit/AgentBridge/agent_bridge_server.h
#pragma once

#include "agent_bridge_executor.h"
#include "agent_bridge_log_capture.h"
#include "core/io/tcp_server.h"
#include "core/io/stream_peer_tcp.h"
#include "core/string/ustring.h"
#include "core/templates/vector.h"
#include "core/variant/dictionary.h"

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
    String _handle_request(const String &method, const String &path, const String &body);

    // ── REST response helpers ─────────────────────────────────────────────
    static String _rest_ok();
    static String _rest_error(const String &msg);
    static String _rest_output(const String &text);
    static bool   _is_error(const String &json);

    // ── Tool handlers ─────────────────────────────────────────────────────
    String _tool_health(const Dictionary &args);
    String _tool_execute(const Dictionary &args);
    String _tool_get_log(const Dictionary &args);
    String _tool_play_scene(const Dictionary &args);
    String _tool_stop_scene(const Dictionary &args);
    String _tool_save_scene(const Dictionary &args);
    String _tool_save_all(const Dictionary &args);
    String _tool_open_scene(const Dictionary &args);
    String _tool_get_setting(const Dictionary &args);
    String _tool_set_setting(const Dictionary &args);
    String _tool_resource_delete(const Dictionary &args);
    String _tool_resource_duplicate(const Dictionary &args);
    String _tool_resource_ensure(const Dictionary &args);
    String _tool_batch(const Dictionary &args);
    String _dispatch_tool(const String &tool, const Dictionary &args);

public:
    AgentBridgeServer(AgentBridgeLogCapture *p_log, AgentBridgeExecutor *p_exec);
    ~AgentBridgeServer();

    Error start();
    void stop();
    void poll(); // call every frame from EditorPlugin

    uint16_t get_port() const { return port; }
    bool is_running() const;
};
