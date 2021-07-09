#pragma once

class HotPatch
{
public:
	void* patched_address = nullptr;
	void* hotpatch_fptr = nullptr;
	unsigned char backup_bytes[16];
	size_t backup_length;
	HotPatch(void* target);
	BOOL patch(void* dest_fptr);
	BOOL unpatch(void);
	~HotPatch();
};
