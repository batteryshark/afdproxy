#include <WinSock2.h>
#include <ws2tcpip.h>
#pragma comment(lib,"ws2_32.lib")

#include "Dbg.h"
#include <cstdlib>
#include <cstring>

#include "Net.h"

#include "Afdshared.h"
#include "AfdProxy.h"
#include "SOCKS5.h"

// Uncomment this to perform an exploratory connection to the given host to see if it's available.
// This will make a pretty obvious call to an endpoint, so disable it if necessary, it serves
// primarily to pre-empt a connection state as we will be connecting to a proxy which is 
// almost assuredly available.
//#define PRE_EMPT_REAL_DESTINATION 

const char* g_proxy_address_ipv6 = "::1";
const char* g_proxy_address_ipv4 = "127.0.0.1";
const char* g_proxy_port = "55573";
sockaddr* g_proxy_addr_ipv4 = nullptr;
sockaddr* g_proxy_addr_ipv6 = nullptr;


AfdConnection::AfdConnection(unsigned long io_code, int s, unsigned char* data, unsigned int length) {
	this->io_code = io_code;
	this->is_valid = false;
	this->s = -1;
	this->af = 0;
	this->connect_info_length = length;
	this->is_redirected = false;
	this->pconnect_info = nullptr;

	//Dbg::dprintdata(data, length);

	// Determine Address Info - Cache AF.
	sockaddr* d_addr = nullptr;
	sockaddr* p_addr = nullptr;
	int ep = -1;
	if (io_code == IOCTL_AFD_CONNECT) {
		AFD_CONNECT_INFO* info = (AFD_CONNECT_INFO*)data;
		d_addr = &info->RemoteAddress;
		ep = (int)info->ConnectEndpoint & 0xFFFFFFFF;
	}
	else if (io_code == IOCTL_AFD_SUPER_CONNECT) {
		AFD_SUPERCONNECT_INFO* info = (AFD_SUPERCONNECT_INFO*)data;
		d_addr = &info->RemoteAddress;
	}
	else {
		return; // If we don't understand the ioctl called, leave.
	}

	// Set up Socket
	if (Net::IsSocket(s)) {
		this->s = s;
	}
	else if (Net::IsSocket(ep)) {
		this->s = ep;
	}
	else {
		Dbg::dprintf("IsSockets All Failed! :(");
		return;
	}


	this->af = d_addr->sa_family;
	if (this->af == AF_INET) {
		p_addr = g_proxy_addr_ipv4;
	}
	else if (this->af == AF_INET6) {
		p_addr = g_proxy_addr_ipv6;
	}
	else {
		Dbg::dprintf("Error - Invalid Ioctl Packet [AF is Invalid]"); return;
	}


	// If the port matches our Proxy's port, leave.
	unsigned short dest_port = 0;
	unsigned short proxy_port = 0;
	if (!Net::GetPort(d_addr, &dest_port) || !Net::GetPort(g_proxy_addr_ipv4, &proxy_port)) { Dbg::dprintf("Error Getting Port Values"); return; }
	if (dest_port == proxy_port) { return; }


#ifdef PRE_EMPT_REAL_DESTINATION
	// Check our endpoint to see if we can connect to it.
	if (!SOCKS5::SOCKS5Test(d_addr, p_addr)) {return;}
#endif

	this->is_valid = true;

	// Allocate space for our connection data copy.
	this->pconnect_info = malloc(length);
	if (!this->pconnect_info) { return; }

	memcpy(this->pconnect_info, data, length);

	return;
}

AfdConnection::~AfdConnection() {
	this->io_code = 0;
	this->is_valid = false;
	this->s = NULL;
	this->af = 0;
	
	this->is_redirected = false;
	if (this->connect_info_length) {
		if (this->pconnect_info) {
			free(this->pconnect_info);
		}
	}
	this->pconnect_info = nullptr;
	this->connect_info_length = 0;
}


// Performs SOCKS5 Initalization on this socket.
bool AfdConnection::InitRedirect() {

	sockaddr* d_addr = nullptr;

	if (this->io_code == IOCTL_AFD_CONNECT) {
		AFD_CONNECT_INFO* info = (AFD_CONNECT_INFO*)this->pconnect_info;
		d_addr = &info->RemoteAddress;
	}
	else if (io_code == IOCTL_AFD_SUPER_CONNECT) {
		AFD_SUPERCONNECT_INFO* info = (AFD_SUPERCONNECT_INFO*)this->pconnect_info;
		d_addr = &info->RemoteAddress;
	}
	else {return false;}

	// Greeting, Handshake, etc.
	if (!SOCKS5::SOCKS5Greeting(this->s,0)) { return false; }

	if (!SOCKS5::SOCKS5ConnectRequest(this->s, d_addr,0)) { return false; }
	
	this->is_redirected = true;
	return true;
}

// Generates an ioctl compatible address structure given the original types.
// Used for spoofing a connection request.
bool AfdConnection::CreateProxyInfo(unsigned char** data, unsigned int* length) {
	if (!data || !length) { return false; }
	
	// First, we have to figure out how big the base component of our data was:
	// totaldata - sockaddr_in/in6 = base.
	
	size_t base_length = this->connect_info_length;
	void* pproxy = nullptr;
	size_t proxy_length = 0;
	if (this->af == AF_INET) {
		base_length -= sizeof(struct sockaddr_in);
		proxy_length = sizeof(struct sockaddr_in);
		pproxy = g_proxy_addr_ipv4;

	}
	else if (this->af == AF_INET6) {
		base_length -= sizeof(struct sockaddr_in6);
		proxy_length = sizeof(struct sockaddr_in6);
		pproxy = g_proxy_addr_ipv6;
	}
	
	// Now, we build a total amount.
	*length = proxy_length + base_length;
	*data = (unsigned char*)malloc(*length);
	if (!*data) { return false; }

	// Next, we copy the base to our output.
	memcpy(*data, this->pconnect_info, base_length);
	// Next, we copy the proxy info to our output.
	memcpy(*data + base_length, pproxy, proxy_length);
	return true; // REMEMBER TO DEALLOCATE THIS WHEN YOU'RE DONE!!!
}

bool AfdConnection::GetOrigAddrInfo(unsigned char** data, unsigned int* length) {
	if (!data || !length) { return false; }

	if (this->af == AF_INET) {
		*length = sizeof(sockaddr_in);
	}
	else if (this->af == AF_INET6) {
		*length = sizeof(sockaddr_in6);
	}else { return false; }

	sockaddr* d_addr = nullptr;

	if (this->io_code == IOCTL_AFD_CONNECT) {
		AFD_CONNECT_INFO* info = (AFD_CONNECT_INFO*)this->pconnect_info;
		d_addr = &info->RemoteAddress;
	}
	else if (io_code == IOCTL_AFD_SUPER_CONNECT) {
		AFD_SUPERCONNECT_INFO* info = (AFD_SUPERCONNECT_INFO*)this->pconnect_info;
		d_addr = &info->RemoteAddress;
	}
	else { return false; }

	*data = (unsigned char*)malloc(*length);
	if (!*data) { return false; }
	memcpy(*data, d_addr, *length);
	return true;
}

void init_proxy_info() {
	if (!Net::CreateSockAddr(g_proxy_address_ipv4, g_proxy_port, &g_proxy_addr_ipv4)) {
		Dbg::dprintf("WARN: Proxy Address Was Not Resolved - IPv4 Proxy will Not Work!");
	}
	if (!Net::CreateSockAddr(g_proxy_address_ipv6, g_proxy_port, &g_proxy_addr_ipv6)) {
		Dbg::dprintf("WARN: Proxy Address Was Not Resolved - IPv6 Proxy will Not Work!");
	}
	Dbg::dprintf("Info: Proxy Configured: ");
	Net::PrintSockaddr(g_proxy_addr_ipv4);
	Net::PrintSockaddr(g_proxy_addr_ipv6);
}


