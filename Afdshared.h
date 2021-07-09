#pragma once
// Lifted from AFD.sys Windows 10x64 (1903 - 20H1)

#define IOCTL_AFD_BIND                         0x12003
#define IOCTL_AFD_CONNECT                      0x12007
#define IOCTL_AFD_START_LISTEN                 0x1200B
#define IOCTL_AFD_WAITFOR_LISTEN               0x1200C
#define IOCTL_AFD_ACCEPT                       0x12010
#define IOCTL_AFD_RECEIVE                      0x12017
#define IOCTL_AFD_RECEIVE_DATAGRAM             0x1201B
#define IOCTL_AFD_SEND                         0x1201F
#define IOCTL_AFD_SEND_DATAGRAM                0x12023
#define IOCTL_AFD_POLL                         0x12024
#define IOCTL_AFD_PARTIAL_DISCONNECT           0x1202B
#define IOCTL_AFD_GET_ADDRESS                  0x1202F
#define IOCTL_AFD_QUERY_RECEIVE_INFORMATION    0x12033
#define IOCTL_AFD_QUERY_HANDLES                0x12037
#define IOCTL_AFD_SET_INFORMATION              0x1203B
#define IOCTL_AFD_GET_REMOTE_ADDRESS           0x1203F
#define IOCTL_AFD_GET_CONTEXT                  0x12043
#define IOCTL_AFD_SET_CONTEXT                  0x12047
#define IOCTL_AFD_SET_CONNECT_DATA             0x1204B
#define IOCTL_AFD_SET_CONNECT_DATA_2           0x1204F
#define IOCTL_AFD_SET_CONNECT_DATA_3           0x12053
#define IOCTL_AFD_SET_CONNECT_DATA_4           0x12057
#define IOCTL_AFD_SET_CONNECT_DATA_5           0x1205B
#define IOCTL_AFD_SET_CONNECT_DATA_6           0x1205F
#define IOCTL_AFD_SET_CONNECT_DATA_7           0x12063
#define IOCTL_AFD_SET_CONNECT_DATA_8           0x12067
#define IOCTL_AFD_SET_CONNECT_DATA_9           0x1206B
#define IOCTL_AFD_SET_CONNECT_DATA_10          0x1206F
#define IOCTL_AFD_SET_CONNECT_DATA_11          0x12073
#define IOCTL_AFD_SET_CONNECT_DATA_12          0x12077
#define IOCTL_AFD_GET_INFORMATION              0x1207B
#define IOCTL_AFD_TRANSMIT_FILE                0x1207F
#define IOCTL_AFD_SUPER_ACCEPT                 0x12083
#define IOCTL_AFD_EVENT_SELECT                 0x12087
#define IOCTL_AFD_ENUM_NETWORK_EVENTS          0x1208B
#define IOCTL_AFD_DEFER_ACCEPT                 0x1208C
#define IOCTL_AFD_WAITFOR_LISTEN_2             0x12090
#define IOCTL_AFD_SETQOS                       0x12094
#define IOCTL_AFD_GETQOS                       0x12098
#define IOCTL_AFD_NO_OPERATION                 0x1209F
#define IOCTL_AFD_VALIDATE_GROUP               0x120A0
#define IOCTL_AFD_GET_UNACCEPTED_CONNECT_DATA  0x120A7
#define IOCTL_AFD_ROUTING_INTERFACE_QUERY      0x120AB
#define IOCTL_AFD_ROUTING_INTERFACE_CHANGE     0x120AC
#define IOCTL_AFD_ADDRESS_LIST_QUERY           0x120B3
#define IOCTL_AFD_ADDRESS_LIST_CHANGE          0x120B4
#define IOCTL_AFD_CONNECT_2                    0x120BB
#define IOCTL_AFD_TL_IO_CONTROL                0x120BF
#define IOCTL_AFD_TRANSMIT_PACKETS             0x120C3
#define IOCTL_AFD_SUPER_CONNECT                0x120C7
#define IOCTL_AFD_SUPER_DISCONNECT             0x120CB
#define IOCTL_AFD_RECEIVE_DATAGRAM_2           0x120CF
#define IOCTL_AFD_SEND_MESSAGE_DISPATCH        0x120D3
#define IOCTL_AFD_SAN_FAST_CEMENT_ENDPOINT     0x120D7
#define IOCTL_AFD_SAN_FAST_SET_EVENTS          0x120DB
#define IOCTL_AFD_SAN_FAST_RESET_EVENTS        0x120DF
#define IOCTL_AFD_SAN_CONNECT_HANDLER          0x120E2
#define IOCTL_AFD_SAN_FAST_COMPLETE_ACCEPT     0x120E7
#define IOCTL_AFD_SAN_FAST_COMPLETE_REQUEST    0x120EB
#define IOCTL_AFD_SAN_FAST_COMPLETE_IO         0x120EF
#define IOCTL_AFD_SAN_FAST_REFRESH_ENDPOINT    0x120F3
#define IOCTL_AFD_SAN_FAST_GET_PHYSICAL_ADDR   0x120F7
#define IOCTL_AFD_SAN_ACQUIRE_CONTEXT          0x120FB
#define IOCTL_AFD_SAN_FAST_TRANSFER_CTX        0x120FF
#define IOCTL_AFD_SAN_FAST_GET_SERVICE_PID     0x12103
#define IOCTL_AFD_SAN_FAST_SET_SERVICE_PROCESS 0x12107
#define IOCTL_AFD_SAN_FAST_PROVIDER_CHANGE     0x1210B
#define IOCTL_AFD_SAN_ADDR_LIST_CHANGE         0x1210C
#define IOCTL_AFD_UNBIND_SOCKET                0x12113
#define IOCTL_AFD_SQM                          0x12117
#define IOCTL_AFD_RIO                          0x1211B
#define IOCTL_AFD_SOCKET_TRANSFER_BEGIN        0x1211F
#define IOCTL_AFD_SOCKET_TRANSFER_END          0x12123



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



