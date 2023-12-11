#ifndef COSMOS_SOCKET_ERROR_HXX
#define COSMOS_SOCKET_ERROR_HXX

// C++
#include <cstdint>

// Linux
#include <time.h>
#include <linux/errqueue.h>

namespace cosmos {

/// Wrapper for socket extended errors `sock_extended_err`
class SocketError {
public: // types

	enum class Origin : uint8_t {
		NONE     = SO_EE_ORIGIN_NONE,
		LOCAL    = SO_EE_ORIGIN_LOCAL,
		ICMP     = SO_EE_ORIGIN_ICMP,
		ICMP6    = SO_EE_ORIGIN_ICMP6,
		TXSTATUS = SO_EE_ORIGIN_TXSTATUS,
		ZEROCOPY = SO_EE_ORIGIN_ZEROCOPY,
		TXTIME   = SO_EE_ORIGIN_TXTIME
	};

	enum class Code : uint8_t {
		ZEROCOPY_COPIED      = SO_EE_CODE_ZEROCOPY_COPIED,
		TXTIME_INVALID_PARAM = SO_EE_CODE_TXTIME_INVALID_PARAM,
		TXTIME_MISSED        = SO_EE_CODE_TXTIME_MISSED
	};

public: // functions

	Origin origin() const {
		return Origin{m_err->ee_origin};
	}

	Code code() const {
		return Code{m_err->ee_code};
	}

protected: // data

	struct sock_extended_err *m_err;
};

} // end ns

#endif // inc. guard
