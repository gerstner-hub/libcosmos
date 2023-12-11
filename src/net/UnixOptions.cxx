// cosmos
#include "cosmos/net/UnixOptions.hxx"
#include "cosmos/private/sockopts.hxx"

namespace cosmos {

UnixOptions::Credentials UnixOptions::credentials() const {
	Credentials ret;
	getsockopt(m_sock, M_LEVEL, OptName{SO_PEERCRED}, &ret, sizeof(ret));
	return ret;
}

}; // end ns
