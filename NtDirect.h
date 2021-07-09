#pragma once
class NtDirect
{
public:
	void* ptr = nullptr;
	unsigned int get_service_id(const char* service_name);
	NtDirect(unsigned int service_id, const char* service_name);
	~NtDirect();
};
