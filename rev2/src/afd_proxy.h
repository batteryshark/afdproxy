#pragma once
#include <winsock2.h>

typedef struct _AFD_CONNECT_INFO {
	void* UseSAN;
	void* Root;
	void* ConnectEndpoint;
	struct sockaddr RemoteAddress;
} AFD_CONNECT_INFO, * PAFD_CONNECT_INFO;


#define PACKED
#pragma pack(push,1)
typedef struct _AFD_SUPERCONNECT_INFO {
	unsigned long long  Unknown;
	unsigned short      AddrLen;
	struct sockaddr     RemoteAddress;
}AFD_SUPERCONNECT_INFO, * PAFD_SUPERCONNECT_INFO;
#pragma pack(pop)
#undef PACKED




class AfdConnection {
public:
	unsigned long io_code; // Code of the operation that spawned this connection.
	int s; // Socket Handle Value
	unsigned int af; // Our Connection's Address Family (Without having to know the sockaddr structure).
	bool is_redirected;  // A flag to denote if we've redirected this already.
	bool is_valid;       // A flag to denote if the input data was parsed and initial checks validate this connection.
	unsigned int connect_info_length; // The size of our connect_info.
	void* pconnect_info; // A Pointer to our connection information.
	AfdConnection(unsigned long io_code, int s, unsigned char* data, unsigned int length);
	~AfdConnection();
	bool InitRedirect();
	bool CreateProxyInfo(unsigned char** data, unsigned int* length);
	bool GetOrigAddrInfo(unsigned char** data, unsigned int* length);
};

void init_proxy_info();