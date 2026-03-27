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
        res.error = "GDScript parse/compile error (Error code " + itos((int)err) + ")";
        res.ok = false;
        return res;
    }

    if (!script->can_instantiate()) {
        res.error = "Script cannot be instantiated.";
        res.ok = false;
        return res;
    }

    // Create a host object of the script's native base class, then attach script.
    if (script->get_native().is_null()) {
        res.error = "Script has no native base class.";
        res.ok = false;
        return res;
    }
    Object *obj = ClassDB::instantiate(script->get_native()->get_name());
    if (!obj) {
        res.error = "Failed to create script instance.";
        res.ok = false;
        return res;
    }

    // Wrap in Ref<RefCounted> so memory is managed automatically.
    Ref<RefCounted> guard(Object::cast_to<RefCounted>(obj));
    obj->set_script(script);

    // Call _run() via the script instance.
    Callable::CallError ce;
    obj->callp(StringName("_run"), nullptr, 0, ce); // return value intentionally ignored

    if (ce.error != Callable::CallError::CALL_OK) {
        res.error = "Runtime error in _run() (CallError " + itos((int)ce.error) + ")";
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
