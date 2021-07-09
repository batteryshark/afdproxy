#pragma once
#include <WinSock2.h>
#include <stdbool.h>

namespace SOCKS5 {
	bool SOCKS5Greeting(SOCKET s, int use_async);
	bool SOCKS5ConnectRequest(SOCKET s, const struct sockaddr* d, int use_async);
	bool SOCKS5Test(const struct sockaddr* dest, const struct sockaddr* proxy);
}

