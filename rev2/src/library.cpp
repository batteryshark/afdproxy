#include <windows.h>

#include "afd_proxy.h"
#include "hooks.h"

#include "nativecore/mem/mem_utils.h"
#include "nativecore/syscall/syscall_utils.h"
#include "nativecore/dbg/dbg_utils.h"



// An Export For Import Binding - Just in Case :)
__declspec(dllexport) void dropkick(void) {};

static struct SysCallDirect_Stub* ntd_NtDeviceIoControlFile = nullptr;

static struct HotPatch_Info* hp_NtDeviceIoControlFile = nullptr;

// Administrative Tasks
void init(){
	DbgUtils__dbg_printf("AfdProxy Init...");
	// Install Necessary Hooks

    if(!SysCallUtils__create_direct_syscall_by_name("NtDeviceIoControlFile",ntd_NtDeviceIoControlFile)){
         DbgUtils__dbg_printf("SocketSys: NtDirect Bind Fail."); 
         return;
    }
    
	real_NtDeviceIoControlFile = reinterpret_cast<tNtDeviceIoControlFile>(ntd_NtDeviceIoControlFile->ptr);
    
    hp_NtDeviceIoControlFile->target_address = GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtDeviceIoControlFile");
    hp_NtDeviceIoControlFile->replacement_function_address = HK_NtDeviceIoControlFile;
    if(!MemUtils__hotpatch_function(hp_NtDeviceIoControlFile)){
        DbgUtils__dbg_printf("SocketSys: HotPatch Fail."); 
        return; 
    }
    
	// Set up our Proxy Info.
	init_proxy_info();
}

void cleanup() {
	DbgUtils__dbg_printf("AfdProxy Shutdown...");
}

// Entry-Point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH: // We only need to run this once.
		DisableThreadLibraryCalls(hinstDLL);
		init();
		break;
	case DLL_PROCESS_DETACH:
		cleanup();
		break;
	default:
		break;
	}
	return TRUE;
}