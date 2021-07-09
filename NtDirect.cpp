#include <Windows.h>

#include "NtDirect.h"

// Uncomment this to build for native 32 bit Windows Operating Systems (e.g. 32 bit processes running on a 32 bit OS).
// If commented, the fallback is the far more common 32 bit processes on WOW64.
//#define NATIVE_X86 1

#define PAGE_SIZE 0x1000

#ifdef _WIN64
// Syscall template for 64 bit Processes on a 64 bit OS.
static unsigned char syscall_stub[] = {
	0xB8, 0x00, 0x00, 0x00, 0x00, // mov eax, [SERVICE_ID]
	0x4C, 0x8B, 0xD1,             // mov r10, rcx
	0x0F, 0x05,                   // syscall
	0xC3                          // ret
};
#elif NATIVE_X86
// Syscall templte for 32 bit Processes on a 32 bit OS.
static unsigned char syscall_stub[] = {
	0xB8,0x00,0x00,0x00,0x00,     // mov eax, [SERVICE_ID]
	0xBA, 0x00, 0x03, 0xFE, 0x7F, // mov edx, offset SharedUserData!SystemCallStub
	0xFF, 0x12,                   // call dword ptr [edx]
	0xC3
};
#else
// Syscall template for 32 bit Processes on a 64 bit OS (WOW64/Heaven's Gate)
static unsigned char syscall_stub[] = {
	0xB8,0x00,0x00,0x00,0x00, // mov eax, [SERVICE_ID]
	0x64,0xFF, 0x15, 0xC0, 0x00, 0x00, 0x00, // call dword ptr fs : [0xC0] -- wow64cpu!X86SwitchTo64BitMode
	0xC3
};

#endif


// TODO: Might want to inline this to not use virtualalloc.
LPVOID allocate_exec_page() {
	return VirtualAlloc(nullptr, PAGE_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
}

void clear_exec_page(LPVOID addr) {
	// Copy the stub code
	DWORD dwOldProtect = 0;
	VirtualProtect((LPVOID)addr, PAGE_SIZE, PAGE_EXECUTE_READWRITE, &dwOldProtect);
	RtlZeroMemory(addr, PAGE_SIZE);
	VirtualProtect((LPVOID)addr, PAGE_SIZE, dwOldProtect, &dwOldProtect);

	VirtualFree(addr, 0, MEM_RELEASE);
}



// Optional find service id.
unsigned int NtDirect::get_service_id(const char* service_name)
{
	unsigned int service_id = 0;
	FARPROC faddr = GetProcAddress(GetModuleHandleA("ntdll.dll"), service_name);
	// Temporarily copy some data here to make it easier to iterate cross arch.
	unsigned char pchk[8] = { 0x00 };
	memcpy(pchk, faddr, sizeof(pchk));
	for (int i = 0; i < sizeof(pchk); i++) {
		if (pchk[i] == 0xB8) {
			memcpy(&service_id, pchk + i + 1, sizeof(unsigned int));
		}
	}
	RtlZeroMemory(pchk, sizeof(pchk));
	return service_id;
}

NtDirect::NtDirect(unsigned int service_id, const char* service_name) {
	this->ptr = nullptr;

	// If service_name is nullptr, use the service id and make the new function.
	unsigned int id = service_id;
	if (service_name != nullptr) {
		id = this->get_service_id(service_name);
	}
	// If we couldn't resolve the opcode at all, just die already.
	if (!id) { return; }

	// Allocate our syscall stub area
	LPVOID exec_page = allocate_exec_page();
	if (!exec_page) { return; }
	unsigned char cstub[sizeof(syscall_stub)] = { 0x00 };
	memcpy(cstub, syscall_stub, sizeof(syscall_stub));
	*(unsigned int*)(cstub + 1) = id;

	// Copy the stub code
	DWORD dwOldProtect = 0;
	if (!VirtualProtect((LPVOID)exec_page, PAGE_SIZE, PAGE_EXECUTE_READWRITE, &dwOldProtect)) { return; }
	memcpy((void*)exec_page, cstub, sizeof(syscall_stub));

	if (!VirtualProtect((LPVOID)exec_page, PAGE_SIZE, dwOldProtect, &dwOldProtect)) { return; }
	this->ptr = exec_page;

}


NtDirect::~NtDirect() {
	clear_exec_page(this->ptr);
	this->ptr = nullptr;
}