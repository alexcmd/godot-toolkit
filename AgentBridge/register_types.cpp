#include "register_types.h"

#include "core/object/class_db.h"

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
    // Cleanup handled by plugin destructor.
}
