// cosmos
#include "cosmos/net/UnixOptions.hxx"
#include "cosmos/private/sockopts.hxx"

namespace cosmos {

UnixCredentials UnixOptions::credentials() const {
	UnixCredentials ret;
	getsockopt(m_sock, M_LEVEL, OptName{SO_PEERCRED}, &ret, sizeof(ret));
	return ret;
}

}; // end ns
