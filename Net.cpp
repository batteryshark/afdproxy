#include <WinSock2.h>
#include <ws2tcpip.h>
#pragma comment(lib,"ws2_32.lib")
#include <cstdint>

#include "Dbg.h"

#define bswap16(x) ((uint16_t)((((uint16_t) (x) & 0xff00) >> 8) | \
                               (((uint16_t) (x) & 0x00ff) << 8)))

#define bswap32(x) ((uint32_t)((((uint32_t) (x) & 0xff000000) >> 24) | \
                               (((uint32_t) (x) & 0x00ff0000) >> 8) | \
                               (((uint32_t) (x) & 0x0000ff00) << 8) | \
                               (((uint32_t) (x) & 0x000000ff) << 24)))

#include "Net.h"

namespace Net {

	void PrintSockaddr(const sockaddr* res) {
		struct sockaddr_in* addr_in = (struct sockaddr_in*)res;
		char* s = NULL;
		unsigned short port = 0;
		switch (res->sa_family) {
		case AF_INET: {
			struct sockaddr_in* addr_in = (struct sockaddr_in*)res;
			s = (char*)malloc(INET_ADDRSTRLEN);
			inet_ntop(AF_INET, &(addr_in->sin_addr), s, INET_ADDRSTRLEN);
			port = ntohs(addr_in->sin_port);
			break;
		}
		case AF_INET6: {
			struct sockaddr_in6* addr_in6 = (struct sockaddr_in6*)res;
			s = (char*)malloc(INET6_ADDRSTRLEN);
			inet_ntop(AF_INET6, &(addr_in6->sin6_addr), s, INET6_ADDRSTRLEN);
			port = ntohs(addr_in6->sin6_port);
			break;
		}
		default:
			break;
		}
		Dbg::dprintf("%s:%d", s, port);
		free(s);
	}

	bool SocketSetBlocking(SOCKET s) {
		unsigned long mode = 0;
		int result = ioctlsocket(s, FIONBIO, &mode);
		if (result == SOCKET_ERROR) { return false; }
		return true;
	}

	bool SocketSetNonBlocking(SOCKET s) {
		unsigned long mode = 1;
		int result = ioctlsocket(s, FIONBIO, &mode);
		if (result == SOCKET_ERROR) { return false; }
		return true;
	}

	// Connect to an IPv4 or IPv6 Address with a Given Socket (Supports Blocking and Non-Blocking).
	bool ConnectSocket(SOCKET s, const sockaddr* d, unsigned int* is_non_blocking_socket) {
		*is_non_blocking_socket = 0;
		// Check if We Can Connect to the Proxy First.
		Dbg::dprintf("[Socket: %04X] Connecting to: ", s);
		PrintSockaddr(d);
		int namelen = sizeof(sockaddr_in);
		if (d->sa_family == AF_INET6) {
			namelen = sizeof(sockaddr_in6);
		}
		if (connect(s, d, namelen) != SOCKET_ERROR) {
			Dbg::dprintf("Proxy Server Connected!");
			return true;
		}

		int ret = WSAGetLastError();
		if (ret != WSAEWOULDBLOCK) {
			Dbg::dprintf("Could Not Connect to SOCKS5 Proxy Server [%04X]", WSAGetLastError());
			return false;
		}

		// Everything Under Here Involves Non-Blocking Sockets
		fd_set Write, Err;
		FD_ZERO(&Write);
		FD_ZERO(&Err);
		FD_SET(s, &Write);
		FD_SET(s, &Err);

		TIMEVAL Timeout;
		Timeout.tv_sec = 10;
		Timeout.tv_usec = 0;

		ret = select(0, NULL, &Write, &Err, &Timeout);
		if (ret == SOCKET_ERROR) {
			Dbg::dprintf("Error in Select: %04X", WSAGetLastError());
			return false;
		}

		if (!ret) {
			Dbg::dprintf("Error in Select: WSAETIMEDOUT");
			return false;
		}



		if (FD_ISSET(s, &Err)) {
			int err;
			int err_len = sizeof(err);
			if (getsockopt(s, SOL_SOCKET, SO_ERROR, (char*)& err, &err_len) == SOCKET_ERROR) {
				Dbg::dprintf("Error in getsockopt: %04X", WSAGetLastError());
				return false;
			}
			Dbg::dprintf("Error in FD_ISSET: %04X", (int)err);
			return false;
		}

		SocketSetBlocking(s);
		*is_non_blocking_socket = 1;
		Dbg::dprintf("Proxy Server Connected [NonBlocking]!");
		return true;
	}

	// Get a Port Value from Generic SockAddr
	bool GetPort(const sockaddr* addr, unsigned short* pport) {
		if (!addr) {
			Dbg::dprintf("Error (GetPort): addr is null");
			return false;
		}
		if (!pport) {
			Dbg::dprintf("Error (GetPort): pport is null");
			return false;
		}
		if (addr->sa_family == AF_INET) {
			const sockaddr_in* in = (const sockaddr_in*)addr;
			*pport = ntohs(in->sin_port);
			return true;
		}
		else if (addr->sa_family == AF_INET6) {
			const sockaddr_in6* in = (const sockaddr_in6*)addr;
			*pport = ntohs(in->sin6_port);
			return true;
		}
		else {
			Dbg::dprintf("Error [GetPort]: Unrecognized AF (%04X)", addr->sa_family);
		}
		return false;
	}

	bool GetAddressFamily(const char* host_addr, ADDRESS_FAMILY* paf) {
		unsigned char tbuff[32] = { 0x00 };

		if (InetPtonA(AF_INET, host_addr, tbuff) == 1) { *paf = AF_INET; return true; }
		if (InetPtonA(AF_INET6, host_addr, tbuff) == 1) { *paf = AF_INET6; return true; }

		return false;
	}

	// Interpret some data to create a SOCKADDR_IN/6 Structure.
	bool CreateSockAddr(const char* host_addr, const char* host_port, sockaddr** addr) {
		unsigned short port = atoi(host_port) & 0xFFFF;
		ADDRESS_FAMILY af = 0;
		if (!GetAddressFamily(host_addr, &af)) {
			Dbg::dprintf("[CreateSockAddr]: Error Getting Address Family.");
			return false;
		}

		if (af == AF_INET) {
			*addr = (sockaddr*)calloc(1, sizeof(sockaddr_in));
			sockaddr_in* in = (sockaddr_in*)* addr;
			InetPtonA(af, host_addr, &in->sin_addr);
			in->sin_port = htons(port);
			in->sin_family = af;
		}
		else if (af == AF_INET6) {
			*addr = (sockaddr*)calloc(1, sizeof(sockaddr_in6));
			sockaddr_in6* in = (sockaddr_in6*)* addr;
			InetPtonA(af, host_addr, &in->sin6_addr);
			in->sin6_port = htons(port);
			in->sin6_family = af;
		}
		else {
			Dbg::dprintf("[CreateSockAddr]: Error - Unrecognized AF.");
			return false;
		}
		return true;
	}

	bool IsSocket(SOCKET s) {
		int socktype = NULL;
		int optlen = sizeof(socktype);
		if (!getsockopt(s, SOL_SOCKET, SO_TYPE, (char*)& socktype, &optlen)) { return true; }
		//Dbg::dprintf("[IsSocket] Fail for %04X: %04X", s, WSAGetLastError());
		return false;
	}

	// Connect TCP Socket to a Given Address
	bool ConnectTCP(const struct sockaddr* dest, SOCKET* s) {
		*s = SOCKET_ERROR;
		*s = socket(dest->sa_family, SOCK_STREAM, IPPROTO_TCP);
		if (*s == SOCKET_ERROR) {return false;}

		int namelen = sizeof(sockaddr_in);
		if (dest->sa_family == AF_INET6) {
			namelen = sizeof(sockaddr_in6);
		}
		if (connect(*s, dest, namelen) != SOCKET_ERROR) {
			return true;
		}
		return false;
	}


	bool ConvertAF(SOCKET s, int dest_af) {
		WSAPROTOCOL_INFOW protocolInfo;

		int protocolInfoSize = sizeof(protocolInfo);

		if (0 != getsockopt(s, SOL_SOCKET, SO_PROTOCOL_INFOW, (char*)& protocolInfo, &protocolInfoSize))
		{
			return false;
		}
		if (protocolInfo.iAddressFamily != dest_af) {
			Dbg::dprintf("[Socket] AF Mismatch - Altering...");
			protocolInfo.iAddressFamily = dest_af;
			if (0 != setsockopt(s, SOL_SOCKET, SO_PROTOCOL_INFOW, (char*)& protocolInfo, protocolInfoSize)) {
				return false;
			}
		}



		return true;
	}

}
