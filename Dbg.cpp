#include <cstdio>
#include <cstdarg>
#include <string.h>

#include <Windows.h>

#include "Dbg.h"


int byteArrayToHexString(unsigned char* byte_array, int byte_array_len, char* hexstr, int hexstr_len) {
	int off = 0;
	int i;

	for (i = 0; i < byte_array_len; i++) {
		off += snprintf(hexstr + off, hexstr_len - off,
			"%02x", byte_array[i]);
	}

	hexstr[off] = '\0';

	return off;
}

namespace Dbg {

	void dprintf(const char* format, ...) {
#ifdef ENABLE_DEBUG
		char s[8192];
		va_list args;
		ZeroMemory(s, 8192 * sizeof(s[0]));
		va_start(args, format);
		vsnprintf(s, 8191, format, args);
		va_end(args);
		s[8191] = 0;
		OutputDebugStringA(s);
#endif
	}

	void dprintdata(unsigned char* ptr, size_t length) {
		int datalen = (length * 2) + 1;
		char* dataptr = (char*)malloc(datalen);
		byteArrayToHexString(ptr, length, dataptr, datalen);
		dprintf(dataptr);
		dprintf("\n");
	}

}