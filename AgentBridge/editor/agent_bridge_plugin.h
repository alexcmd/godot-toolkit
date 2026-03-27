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
