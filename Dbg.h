#pragma once

// Undefine this to disable debug messages.
#define ENABLE_DEBUG

namespace Dbg {
	void dprintf(const char* format, ...);
	void dprintdata(unsigned char* ptr, size_t length);
}