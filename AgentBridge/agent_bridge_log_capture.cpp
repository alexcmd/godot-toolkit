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
		self->buffer.write[self->count] = entry;
		self->count++;
		self->write_pos = self->count % self->capacity;
	} else {
		self->buffer.write[self->write_pos] = entry;
		self->write_pos = (self->write_pos + 1) % self->capacity;
	}
}

AgentBridgeLogCapture::AgentBridgeLogCapture(int p_capacity) :
		capacity(MAX(1, p_capacity)) {
	buffer.resize(capacity); // pre-allocate to avoid per-entry reallocation
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
