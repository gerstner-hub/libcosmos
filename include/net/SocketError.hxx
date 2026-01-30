#pragma once

// C++
#include <cstdint>
#include <optional>
#include <type_traits>
#include <utility>

// Linux
#include <sys/time.h>
#include <linux/errqueue.h>

// Cosmos
#include <cosmos/error/errno.hxx>
#include <cosmos/net/IPAddress.hxx>
#include <cosmos/net/traits.hxx>
#include <cosmos/net/types.hxx>

namespace cosmos {

/// Wrapper for socket extended errors ancillary message of types IP4Message::RECVERR and IP6Message::RECVERR.
/**
 * This data structure is passed for IP based sockets if the
 * IP4Options::setReceiveErrors() or IP6Options::setReceiveErrors() option is
 * enabled. Extended error reporting generally only works for
 * SocketType::DGRAM sockets. All errors on the socket will be queued in a
 * separate error message queue and these errors can be received using
 * Socket::receiveMessage() with the MessageFlag::ERRQUEUE set.
 *
 * Various kinds of data can be part of this structure. The exact
 * interpretation depends upon the Origin and error() code. For this reason
 * various functions only return optional values.
 **/
template <SocketFamily FAMILY>
class SocketErrorT :
		protected sock_extended_err {
public: // types

	SocketErrorT() {
		// explicitly do nothing since we use this type with placement
		// new on network data.
	}

	using IPAddress = typename FamilyTraits<FAMILY>::Address;

	/// This defines where the extended error originated.
	enum class Origin : uint8_t {
		NONE     = SO_EE_ORIGIN_NONE,
		/// The local networking stack detected an error.
		LOCAL    = SO_EE_ORIGIN_LOCAL,
		/// An ICMPv4 error was reported.
		ICMP     = SO_EE_ORIGIN_ICMP,
		/// An ICMPv6 error was reported.
		ICMP6    = SO_EE_ORIGIN_ICMP6,
		TXSTATUS = SO_EE_ORIGIN_TXSTATUS,
		/// Status report for zerocopy operation (see Linux kernel documentation networking/msg_zerocopy.txt).
		ZEROCOPY = SO_EE_ORIGIN_ZEROCOPY,
		TXTIME   = SO_EE_ORIGIN_TXTIME
	};

	/// Code definitions for Origin::ZEROCOPY.
	enum class ZeroCopyCode : uint8_t {
		/// No zerocopy was performed, the kernel performed a copy.
		ZEROCOPY_COPIED      = SO_EE_CODE_ZEROCOPY_COPIED,
	};

	/// Code definitions for Origin::TXTIME.
	enum class TxTimeCode : uint8_t {
		TXTIME_INVALID_PARAM = SO_EE_CODE_TXTIME_INVALID_PARAM,
		TXTIME_MISSED        = SO_EE_CODE_TXTIME_MISSED
	};

public: // functions

	/// The origin defines how the rest of the error data is interpreted.
	Origin origin() const {
		return Origin{this->ee_origin};
	}

	/// The error code is always available, but may be Errno::NO_ERROR.
	Errno errnum() const {
		// ee_errno is unsigned, while errno is signed
		return Errno{static_cast<std::underlying_type<Errno>::type>(this->ee_errno)};
	}

	/// If errnum() is Errno::MSG_TOO_LARGE then this returns the currently known MTU.
	std::optional<uint32_t> discoveredMTU() const {
		if (errnum() == Errno::MSG_TOO_LARGE) {
			return this->ee_info;
		}

		return std::nullopt;
	}

	std::optional<ZeroCopyCode> zeroCopyCode() const {
		if (origin() == Origin::ZEROCOPY) {
			return ZeroCopyCode{this->ee_code};
		}

		return std::nullopt;
	}

	std::optional<TxTimeCode> txTimeCode() const {
		if (origin() == Origin::TXTIME) {
			return TxTimeCode{this->ee_code};
		}

		return std::nullopt;
	}

	/// Return the copied ranges for zerocopy status reports.
	/**
	 * If this is a zerocopy status report from the kernel then this
	 * returns the range of transmissions that have been completed.
	 **/
	std::optional<std::pair<uint32_t, uint32_t>> zeroCopyRange() const {
		if (origin() == Origin::ZEROCOPY) {
			return std::make_pair(this->ee_info, this->ee_data);
		}

		return std::nullopt;
	}

	bool originIsICMP() const {
		return origin() == Origin::ICMP || origin() == Origin::ICMP6;
	}

	std::optional<uint8_t> icmpType() const {
		if (originIsICMP()) {
			return this->ee_type;
		}

		return std::nullopt;
	}

	std::optional<uint8_t> icmpCode() const {
		if (originIsICMP()) {
			return this->ee_code;
		}

		return std::nullopt;
	}

	SocketFamily offenderAddressFamily() const {
		return SocketFamily(offenderAddr()->sa_family);
	}

	/// Check whether the offender IP address is available.
	/**
	 * If available then the kernel also provides the IP address of the
	 * node that caused the error. offenderAddress() allows to access the
	 * address, if applicable.
	 **/
	bool hasOffenderAddress() const {
		return offenderAddressFamily() != SocketFamily::UNSPEC;
	}

	std::optional<IPAddress> offenderAddress() const {
		if (offenderAddressFamily() != FAMILY)
			return std::nullopt;

		using RawAddr = typename FamilyTraits<FAMILY>::RawAddr;
		return IPAddress{*reinterpret_cast<const RawAddr*>(offenderAddr())};
	}

protected: // functions

	const struct sockaddr* offenderAddr() const {
		// this is piggyback data found after the end of the struct
		// sock_extended_err. It is only valid if sa_family !=
		// AF_UNSPEC.
		return SO_EE_OFFENDER(this);
	}
};

using IP4SocketError = SocketErrorT<SocketFamily::INET>;
using IP6SocketError = SocketErrorT<SocketFamily::INET6>;

} // end ns
