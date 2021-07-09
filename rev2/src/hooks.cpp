

#include "nativecore/dbg/dbg_utils.h"

#include "afd_const.h"
#include "connection_db.h"

#include "hooks.h"

// Some Defines

tNtDeviceIoControlFile real_NtDeviceIoControlFile = nullptr;

NTSTATUS NTAPI HK_NtDeviceIoControlFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, ULONG IoControlCode, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength) {
	//DbgUtils__dbg_printf("IOCTL: %04X", IoControlCode);

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
				DbgUtils__dbg_printf("Our Spoof length is Bigger than the allocated buffer!");
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
