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
