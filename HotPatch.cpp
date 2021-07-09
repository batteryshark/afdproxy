#include <Windows.h>
#include "HotPatch.h"

#ifdef _WIN64
#define HOTPATCH_ADDRESS_OFFSET 2
static unsigned char hotpatch_stub[] = {
	0x48,0xB8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // mov rax, [Abs Jump Address]
	0xFF,0xE0,                                         // jmp rax
	0xC3,                                              // ret
};
#else
#define HOTPATCH_ADDRESS_OFFSET 1
static unsigned char hotpatch_stub[] = {
	0xB8, 0x00, 0x00, 0x00, 0x00, // mov eax, [Abs Jump Address]
	0xFF,0xE0,                    // jmp eax
	0xC3                          // ret
};
#endif

HotPatch::HotPatch(void* target) {
	this->backup_length = 0;
	RtlZeroMemory(this->backup_bytes, sizeof(this->backup_bytes));
	this->patched_address = target;
	this->hotpatch_fptr = nullptr;

}

BOOL HotPatch::patch(void* dest_fptr) {
	// Return if already patched.
	if (!this->patched_address || this->hotpatch_fptr) { return FALSE; }
	unsigned char stub_code[sizeof(hotpatch_stub)] = { 0 };
	memcpy(stub_code, hotpatch_stub, sizeof(hotpatch_stub));

	memcpy(stub_code + HOTPATCH_ADDRESS_OFFSET, &dest_fptr, sizeof(DWORD_PTR));

	// Copy the stub code
	LPVOID target_addr = this->patched_address;
	DWORD dwOldProtect = 0;

	size_t stub_size = sizeof(stub_code);
	if (!VirtualProtect(target_addr, stub_size, PAGE_EXECUTE_READWRITE, &dwOldProtect)) { return FALSE; }
	memcpy(backup_bytes, target_addr, stub_size);
	this->backup_length = stub_size;
	memcpy(target_addr, stub_code, stub_size);
	if (!VirtualProtect(target_addr, stub_size, dwOldProtect, &dwOldProtect)) { return FALSE; }
	this->hotpatch_fptr = dest_fptr;
	return TRUE;
}

BOOL HotPatch::unpatch() {

	// If we haven't set up a target address or no patch is applied, fail out.
	if (!this->patched_address || !this->hotpatch_fptr) { return FALSE; }

	// Copy the stub code
	DWORD dwOldProtect = 0;
	if (!VirtualProtect((LPVOID)this->patched_address, this->backup_length, PAGE_EXECUTE_READWRITE, &dwOldProtect)) { return FALSE; }
	memcpy((LPVOID)this->patched_address, this->backup_bytes, this->backup_length);
	if (!VirtualProtect((LPVOID)this->patched_address, this->backup_length, dwOldProtect, &dwOldProtect)) { return FALSE; }
	return TRUE;
}

HotPatch::~HotPatch() {
	// Might as well unpatch it if we hit the destructor.
	this->unpatch();
	RtlZeroMemory(this->backup_bytes, sizeof(this->backup_bytes));
	this->backup_length = 0;
	this->patched_address = nullptr;
	this->hotpatch_fptr = nullptr;
}