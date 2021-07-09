#pragma once

namespace ConnectionDB {
bool ProxyEntry(unsigned long io_code, int s, unsigned char* pdata, unsigned int data_length, unsigned char** spoof_data, unsigned int* spoof_length);
bool ConnectProxy(int s);
bool GetSpoofData(int s, unsigned char** spoof_data, unsigned int* spoof_length);

}