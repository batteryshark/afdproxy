#include "Dbg.h"
#include "AfdProxy.h"
#include <vector>

static std::vector<AfdConnection*> conn_db;

namespace ConnectionDB {
	// Find our proxy entry and send the socks init stuff out if we haven't already.
	bool ConnectProxy(int s) {
		for (int i = 0; i < conn_db.size(); i++) {
			if (conn_db[i]->s == s) {
				if (conn_db[i]->is_redirected) {return true;}
				return conn_db[i]->InitRedirect();
			}
		}
		return false;
	}

	bool ProxyEntry(unsigned long io_code, int s, unsigned char* pdata, unsigned int data_length, unsigned char** spoof_data, unsigned int* spoof_length) {
		if (!spoof_data || !spoof_length) { return false; }

		// If we have a duplicate socket, fail out.
		for (int i = 0; i < conn_db.size(); i++) {
			if (conn_db[i]->s == s) {
				return false;
			}
		}


		AfdConnection* nc = new AfdConnection(io_code, s, pdata, data_length);
		// If something's not right, burn the house down and go home... wat
		if (!nc->is_valid) {
			delete nc;
			return false;
		}
		if (!nc->CreateProxyInfo(spoof_data, spoof_length)) {
			delete nc;
			return false;
		}

		conn_db.push_back(nc);
		return true;
	}

	bool GetSpoofData(int s, unsigned char** spoof_data, unsigned int* spoof_length) {
		for (int i = 0; i < conn_db.size(); i++) {
			if (conn_db[i]->s == s) {
				return conn_db[i]->GetOrigAddrInfo(spoof_data, spoof_length);
			}
		}
		return false;
	}

}