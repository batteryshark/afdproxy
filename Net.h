#pragma once

namespace Net {
	bool SocketSetNonBlocking(SOCKET s);
	bool SocketSetBlocking(SOCKET s);
	bool ConnectSocket(SOCKET s, const sockaddr* d, unsigned int* is_non_blocking_socket);
	void PrintSockaddr(const sockaddr* res);
	bool GetPort(const sockaddr* addr, unsigned short* pport);
	bool CreateSockAddr(const char* host_addr, const char* host_port, sockaddr** addr);
	bool IsSocket(SOCKET s);
	bool ConvertAF(SOCKET s, int dest_af);
	bool ConnectTCP(const struct sockaddr* dest, SOCKET* s);
}
