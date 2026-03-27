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
