// TODO: Add IPv6 Support (rather, fix it).
#include <WinSock2.h>
#include <Windows.h>
#include <winternl.h>

#include "Dbg.h"
#include "HotPatch.h"
#include "NtDirect.h"
#include "Afdshared.h"
#include "AfdProxy.h"
#include "ConnectionDB.h"

// An Export For Import Binding - Just in Case :)
__declspec(dllexport) void dropkick(void) {};

// Some Defines
#define STATUS_SUCCESS 0
typedef NTSTATUS(*NTAPI tNtDeviceIoControlFile)(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, ULONG IoControlCode, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength);
tNtDeviceIoControlFile real_NtDeviceIoControlFile = nullptr;

static NtDirect* ntd_NtDeviceIoControlFile = nullptr;

static HotPatch* hp_NtDeviceIoControlFile = nullptr;


NTSTATUS NTAPI HK_NtDeviceIoControlFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, ULONG IoControlCode, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength) {
	//Dbg::dprintf("IOCTL: %04X", IoControlCode);

	if (IoControlCode == IOCTL_AFD_CONNECT || IoControlCode == IOCTL_AFD_SUPER_CONNECT) {
		int s = (int)FileHandle;

		unsigned char* proxy_data = nullptr;
		unsigned int proxy_length = 0;
		if (ConnectionDB::ProxyEntry(IoControlCode, s, (unsigned char*)InputBuffer, InputBufferLength, &proxy_data, &proxy_length)) {
			NTSTATUS res = real_NtDeviceIoControlFile(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode, proxy_data, proxy_length, OutputBuffer, OutputBufferLength);
			if (proxy_data) { free(proxy_data); }
			return res;
		}
	}
	// Generally the first thing a tcp socket does - set up our socks EP properly.
	else if (IoControlCode == IOCTL_AFD_POLL) {
		if (!ConnectionDB::ConnectProxy((int)FileHandle)) {
		}

	}// Python Requests / Urllib uses this.

	else if (IoControlCode == IOCTL_AFD_GET_ADDRESS) {
		ConnectionDB::ConnectProxy((int)FileHandle);
	}


	// Generally the first thing an async socket for winhttp hits.
	// This functionality is two-fold:
	// 1. We'll check to see if the proxy needs to be set up.
	// 2. We'll spoof the address data on return to make it look like
	//    our socket is connected to another ep.
	else if (IoControlCode == IOCTL_AFD_GET_REMOTE_ADDRESS) {
		ConnectionDB::ConnectProxy((int)FileHandle);
		unsigned char* spoof_data = nullptr;
		unsigned int spoof_length = 0;
		if (ConnectionDB::GetSpoofData((int)FileHandle, &spoof_data, &spoof_length)) {
			if (spoof_length > OutputBufferLength) {
				Dbg::dprintf("Our Spoof length is Bigger than the allocated buffer!");
			}
			else {

				memcpy(OutputBuffer, spoof_data, spoof_length);
			}
			free(spoof_data);
			return STATUS_SUCCESS;
		}
	}
	
	return real_NtDeviceIoControlFile(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
}

/*
32 Bit: Curl -> AFD_CONNECT, AFD_SELECT
32 Bit: PPQ -> AFD_SUPER_CONNECT, AFD_GETINFO
64 Bit: Curl-> AFD_CONNECT, AFD_SELECT
64 Bit: Winhttp -> AFD_SUPER_CONNECT, AFD_GETINFO
64 Bit: Python  ->  AFD_CONNECT, AFD_GET_ADDRESS
*/




// Administrative Tasks
void init(){
	Dbg::dprintf("AfdProxy Init...");
	// Install Necessary Hooks

	ntd_NtDeviceIoControlFile = new NtDirect(0, "NtDeviceIoControlFile");
	if (!ntd_NtDeviceIoControlFile) { Dbg::dprintf("SocketSys: NtDirect Bind Fail."); return; }
	real_NtDeviceIoControlFile = reinterpret_cast<tNtDeviceIoControlFile>(ntd_NtDeviceIoControlFile->ptr);

	hp_NtDeviceIoControlFile = new HotPatch(GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtDeviceIoControlFile"));
	if (!hp_NtDeviceIoControlFile->patch(HK_NtDeviceIoControlFile)) { Dbg::dprintf("SocketSys: HotPatch Fail."); return; }

	// Set up our Proxy Info.
	init_proxy_info();
}

void cleanup() {
	Dbg::dprintf("AfdProxy Shutdown...");
	// If necessary, restore Redirect
	delete hp_NtDeviceIoControlFile;
	delete ntd_NtDeviceIoControlFile;
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