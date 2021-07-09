#include <WinSock2.h>
#include <Ws2ipdef.h>

#include "Net.h"
#include "Dbg.h"

#include "SOCKS5.h"




namespace SOCKS5 {

	bool SOCKS5Greeting(SOCKET s, int use_async) {
		if (use_async) {
			Net::SocketSetBlocking(s);
		}
		unsigned char greeting_req[3] = {
		0x05,  // Version
		0x01,  // Number of Authentication Methods
		0x00   // Authentication Methods 
		};
		unsigned char greeting_res[2] = { 0xFF };


		
		// If send doesn't work, we will try again using async then sync.
		if (send(s, (const char*)greeting_req, sizeof(greeting_req), 0) != sizeof(greeting_req)) { 
			Net::SocketSetBlocking(s);
			use_async = 1;
			if (send(s, (const char*)greeting_req, sizeof(greeting_req), 0) != sizeof(greeting_req)) { return false; }
		}

		int bytes_recv = -1;
		bytes_recv = recv(s, (char*)greeting_res, sizeof(greeting_res), MSG_WAITALL);
		if (bytes_recv == SOCKET_ERROR) { 
			if (WSAGetLastError() == 0x273D) {
				Net::SocketSetBlocking(s);
				use_async = 1;
				bytes_recv = recv(s, (char*)greeting_res, sizeof(greeting_res), MSG_WAITALL);
				if (bytes_recv == SOCKET_ERROR) {
					return false;
				}
			}
			
			//Dbg::dprintf("SOCKS5 Greeting Recv Failed: %04X!",WSAGetLastError());  return false; 
		}

		if (greeting_res[0] != 0x05 || greeting_res[1] == 0xFF) {
			Dbg::dprintf("SOCKS5 Greeting Failed!");
			return false;
		}

		if (use_async) {
			Net::SocketSetNonBlocking(s);
		}

		return true;
	}


	

	bool SOCKS5ConnectRequest(SOCKET s, const struct sockaddr* d, int use_async) {
		if (use_async) {
			Net::SocketSetBlocking(s);
		}
		
		int addr_length = 0;
		// Buffer overrun likely in some bullshit future event... don't do this.
		unsigned char connect_req[22] = { 0x00 };
		unsigned char connect_res[22] = { 0xFF };

		// Build and Send SOCKS5 Connect Request
	// -- Version ID (SOCKS5 so... yeah - 5)
		connect_req[0] = 0x05;
		// -- Establish a TCP/IP Stream Connection
		connect_req[1] = 0x01;
		// -- Reserved, must be 0x00
		connect_req[2] = 0x00;
	

		if (d->sa_family == AF_INET) {
			const struct sockaddr_in* in = (const struct sockaddr_in*)d;
			connect_req[3] = 0x01; // SOCKS5_IPv4

			connect_req[4] = (in->sin_addr.S_un.S_addr >> 0) & 0xFF;
			connect_req[5] = (in->sin_addr.S_un.S_addr >> 8) & 0xFF;
			connect_req[6] = (in->sin_addr.S_un.S_addr >> 16) & 0xFF;
			connect_req[7] = (in->sin_addr.S_un.S_addr >> 24) & 0xFF;
			connect_req[8] = (in->sin_port >> 0) & 0xFF;
			connect_req[9] = (in->sin_port >> 8) & 0xFF;
			addr_length = 6;
		}
		else if (d->sa_family == AF_INET6) {

			const struct sockaddr_in6* in6 = (const struct sockaddr_in6*)d;
			connect_req[3] = 0x04; // SOCKS5_IPv6
			connect_req[4] = in6->sin6_addr.u.Byte[0];
			connect_req[5] = in6->sin6_addr.u.Byte[1];
			connect_req[6] = in6->sin6_addr.u.Byte[2];
			connect_req[7] = in6->sin6_addr.u.Byte[3];
			connect_req[8] = in6->sin6_addr.u.Byte[4];
			connect_req[9] = in6->sin6_addr.u.Byte[5];
			connect_req[10] = in6->sin6_addr.u.Byte[6];
			connect_req[11] = in6->sin6_addr.u.Byte[7];
			connect_req[12] = in6->sin6_addr.u.Byte[8];
			connect_req[13] = in6->sin6_addr.u.Byte[9];
			connect_req[14] = in6->sin6_addr.u.Byte[10];
			connect_req[15] = in6->sin6_addr.u.Byte[11];
			connect_req[16] = in6->sin6_addr.u.Byte[12];
			connect_req[17] = in6->sin6_addr.u.Byte[13];
			connect_req[18] = in6->sin6_addr.u.Byte[14];
			connect_req[19] = in6->sin6_addr.u.Byte[15];
			connect_req[20] = (in6->sin6_port >> 0) & 0xFF;
			connect_req[21] = (in6->sin6_port >> 8) & 0xFF;
			addr_length = 18;
	
		}
		else { return false; }
	
		// If send didn't work, we'll use the async version instead.
		if (send(s, (const char*)connect_req, 4 + addr_length, 0) != (4 + addr_length)) {
			Net::SocketSetBlocking(s);
			use_async = 1;
			if (send(s, (const char*)connect_req, 4 + addr_length, 0) != (4 + addr_length)) {
				Dbg::dprintf("Send Error!!!");
				return false;
			}
		}
		

		// Receive First Part of Response
		// Receive and Process SOCKS5 Connect Response
		int bytes_recv = recv(s, (char*)connect_res, 4 + addr_length, MSG_WAITALL);
		if (bytes_recv == SOCKET_ERROR) {

			Net::SocketSetBlocking(s);
			use_async = 1;
			bytes_recv = recv(s, (char*)connect_res, 4 + addr_length, MSG_WAITALL);
			if (bytes_recv == SOCKET_ERROR) {
				Dbg::dprintf("RECV ERROR!!!");
				return false;
			}
		}
		
		if (connect_res[0] != 0x05 || connect_res[1] != 0x00) {
			Dbg::dprintf("SOCKS5 Connect Failed!");
			return false;
		}
	
		if (use_async) {
			Net::SocketSetNonBlocking(s);
		}
		
		return true;
	}

	bool SOCKS5Test(const struct sockaddr* dest, const struct sockaddr* proxy) {
		SOCKET s = SOCKET_ERROR;
		if (!Net::ConnectTCP(proxy, &s)) { closesocket(s); return false; }
		if (!SOCKS5Greeting(s, 0)) { closesocket(s); return false; }
		if (!SOCKS5ConnectRequest(s, dest, 0)) { closesocket(s); return false; }
		closesocket(s);
		return true;
	}
}

