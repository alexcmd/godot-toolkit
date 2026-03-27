#ifdef TOOLS_ENABLED

#include "agent_bridge_plugin.h"

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/os/os.h"

AgentBridgePlugin::AgentBridgePlugin() {}

AgentBridgePlugin::~AgentBridgePlugin() {
    // _notification(EXIT_TREE) handles cleanup; this is a safety guard.
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
    String project_dir = ProjectSettings::get_singleton()->get_resource_path();
    return project_dir + "/.godot/agentbridge.port";
}

void AgentBridgePlugin::_write_port_file(uint16_t p_port) {
    String path = _get_port_file_path();
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

            Error err = server->start();
            if (err == OK) {
                const uint16_t bound_port = server->get_port();
                _write_port_file(bound_port);
                print_line(vformat("[AgentBridge] REST API listening on port %d", bound_port));
            } else {
                print_error("[AgentBridge] Failed to start REST API server.");
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
