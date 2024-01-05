#pragma once

// C++
#include <array>

// Linux
#include <net/if.h>
// LOWER_UP and higher InterfaceFlag bits are only found in the kernel
// headers, not in net/if.h. The latter conflicts with the kernel header, if
// pulled in in the wrong order.
#include <linux/if_arp.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>

// cosmos
#include "cosmos/BitMask.hxx"

namespace cosmos {

/// A socket's family setting.
/**
 * The socket family determines the basic underlying mechanism used for the
 * socket.
 *
 * \note The integer used for family is an `int` at the `socket()` call but an
 * `unsigned short int` (`sa_family_t`) within `sockaddr` structures.
 **/
enum class SocketFamily : int {
	UNSPEC  = AF_UNSPEC,  ///< Unknown / not specified.
	INET    = AF_INET,    ///< IPv4.
	INET6   = AF_INET6,   ///< IPv6.
	UNIX    = AF_UNIX,    ///< UNIX domain sockets.
	NETLINK = AF_NETLINK, ///< Netlink sockets talking to the kernel.
	PACKET  = AF_PACKET,  ///< Network device level packets (raw).
};

/// A socket's type setting.
/**
 * The socket type defines a socket's properties in more detail beyond
 * what the SocketFamily already does. In particular it defines the kind of
 * guarantees that the network communication using this socket will offer.
 **/
enum class SocketType : int {
	ANY       = 0,                ///< can be used in AddressHints to return any socket types.
	STREAM    = SOCK_STREAM,      ///< connection oriented, reliable, in-order, but no record boundaries.
	DGRAM     = SOCK_DGRAM,       ///< connection-less, unreliable, unordered with length limitation, keeps message boundaries.
	RAW       = SOCK_RAW,         ///< raw packets as seen on network device level.
	SEQPACKET = SOCK_SEQPACKET,   ///< connection oriented, in-order, reliable with length limitation, keeps message boundaries.
	RDM       = SOCK_RDM,         ///< reliably delivered messages, datagrams without ordering, keeps boundaries.
};

/// Specific protocol to use on a socket.
/**
 * This is usually specified as zero (DEFAULT) but some special sockets may
 * offer different options.
 *
 * For IP these numbers correspond to the protocols found in /etc/protocols.
 *
 * For packet sockets these numbers correspond to the ethernet 802.3 ethernet
 * protocol ID.
 **/
enum class SocketProtocol : int {
	DEFAULT = 0, ///< if used on a packet socket then no packets will be received (until bind).
};

/// Additional socket settings used during socket creation.
enum class SocketFlag : int {
	CLOEXEC   = SOCK_CLOEXEC,  ///< the new socket fd will be automatically closed on exec().
	NONBLOCK  = SOCK_NONBLOCK, ///< the socket will be operating in non-blocking mode.
};

/// Collection of SocketFlag used for creating sockets.
using SocketFlags = BitMask<SocketFlag>;

/// Representation of socket option levels.
/**
 * These levels are used in the different socket options available for
 * sockets. It is an ioctl like API that differentiates the available controls
 * based on this option level.
 **/
enum class OptLevel : int {
	SOCKET = SOL_SOCKET, ///< used for generic socket options and UNIX domain sockets
	IP     = IPPROTO_IP,
	IPV6   = IPPROTO_IPV6,
	TCP    = IPPROTO_TCP,
	UDP    = IPPROTO_UDP
};

/// Representation of socket option names.
/**
 * The constants for options are many and widespread, we just use this
 * type for readability currently and don't model every possible value here.
 **/
enum class OptName : int {};

/// Maximum length of a network device name in bytes.
constexpr auto MAX_NET_INTERFACE_NAME = IFNAMSIZ;

/// A network device interface index.
/**
 * Linux APIs are somewhat inconsistent about the type of this. E.g. in the
 * `sockaddr_in6` structure it is an `uint32_t` while in netdevice it is an
 * `int`. So the signedness is unclear. In LinkLayerAddress it is also an `int`.
 **/
enum class InterfaceIndex : int {
	INVALID = 0, /// zero is in some contexts used for invalid (non-existing) devices.
	ANY     = 0, /// in other contexts it is interpreted as "any" device (packet sockets).
};

// TODO: integrate smart ByteOrder helper and make IPPort and IPRawAddress
// suitable for both, so we can e.g. drop the weak uint32_t in IP4Address and
// make the interface simpler.

/// A 16-bit IP port in network byte order.
enum class IPPort : in_port_t {
};

/// A 32-bit IPv4 binary address in network byte order.
enum class IP4RawAddress : uint32_t {
};

/// The loopback IPv4 address refering to the localhost.
constexpr uint32_t IP4_LOOPBACK_ADDR{INADDR_LOOPBACK};
/// The any IPv4 address specifying all possible addresses or a "don't care" address for automatic assignment, depending on context.
constexpr uint32_t IP4_ANY_ADDR{INADDR_ANY};
/// The IPv4 broadcast address to reach all hosts in the neighborhood.
constexpr uint32_t IP4_BROADCAST_ADDR{INADDR_BROADCAST};

/// A 128-bit IPv6 address
/**
 * \note Since the 128-bit IPv6 address does not correspond to a primitive
 * integer type any more but represents a sequence of bytes, there is no need
 * to consider host and network byte order anymore.
 **/
struct IP6RawAddress :
		public std::array<uint8_t, 16> {
};

/*
 * NOTE: there exist preprocessor defines for these IN6ADDR_ANY_INIT and
 * `in6_addr_any` but it seems easier to just redefine them and avoid all the
 * struct handling.
 */

/// The binary loopback IPv6 address `::1` refering to the localhost.
constexpr IP6RawAddress IP6_LOOPBACK{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
/// The binary IPv6 "any" address `::` specifying all possible addresses or a "don't care" address for automatic assignment, dependingo n context.
constexpr IP6RawAddress IP6_ANY_ADDR{};

/// A 48-bit ethernet 802.3 MAC address.
struct MACAddress :
		public std::array<uint8_t, 6> {
};

/// Flags available for the send() and recv() family of socket I/O functions.
enum class MessageFlag : int {
	/// Inform the link layer that a successful reply was received from the other side.
	/**
	 * If this is not received then the link layer will probe the peer
	 * using ARP. This flag is only supported for SocketType::DGRAM and
	 * SocketType::RAW on SocketFamily::INET and SocketFamily::INET6.
	 **/
	CONFIRM = MSG_CONFIRM,
	/// Don't use a gateway to send out the packet, send only to directly connected networks.
	/**
	 * This is only supported for protocol families that route. It is
	 * typically used for routing protocols or diagnostic programs.
	 **/
	DONT_ROUTE = MSG_DONTROUTE,
	/// Use non-blocking semantics for the I/O call.
	/**
	 * This is similar to setting OpenFlag::NONBLOCK on the file
	 * descriptor, but only affects a single I/O call as opposed to all
	 * calls refering to the same open file description.
	 **/
	DONT_WAIT = MSG_DONTWAIT,
	/// Terminates a record.
	/**
	 * This is only for socket types that support it like
	 * SocketType::SEQPACKET.
	 **/
	END_OF_RECORD = MSG_EOR,
	/// Indicate that more data to send is to follow.
	/**
	 * This is supported for UDP and TCP sockets. Data from multiple
	 * `send()` calls will be merged until a call without this flag set
	 * occurs. For UDP sockets the data will be combined into a single
	 * datagram.
	 **/
	MORE_DATA = MSG_MORE,
	/// Don't send a SIGPIPE signal if a stream oriented connection is closed.
	NO_SIGNAL = MSG_NOSIGNAL,
	/// Send or receive out of band data on protocols that support this.
	OUT_OF_BAND = MSG_OOB,
	/// Attempt a TCP fast-open and send data in the SYNC like a combined connect() and write().
	FASTOPEN = MSG_FASTOPEN,
	/// For SocketFamily::UNIX this requests for received file descriptors to carry the CLOEXEC flag.
	CLOEXEC = MSG_CMSG_CLOEXEC,
	/// Request extended error messages to be received as ancillary messages.
	ERRQUEUE = MSG_ERRQUEUE,
	/// Return data from the beginning of the receive queue, without removing it from the queue.
	PEEK = MSG_PEEK,
	/// Return the real length of a packet or datagram, even if longer than the supplied buffer.
	/**
	 * \warning For TCP sockets this has a different meaning: The received
	 * data will be discarded in the kernel and not be returned to the
	 * caller.
	 **/
	TRUNCATE = MSG_TRUNC,
	/// In recvmsg() `msg_flags` this indicates that some control data was discarded due to lack of space in the ancilliary data buffer.
	CTL_WAS_TRUNCATED = MSG_CTRUNC,
	/// Block on receiving until all requested data has been received.
	/**
	 * This may still return with (short, or empty) reads if a signal is
	 * caught, an error or disconnect occurs or the next data is of a
	 * different type. This has no effect on datagram sockets.
	 **/
	WAIT_ALL = MSG_WAITALL,
	/// Only for recvmsg(): Turn on DONT_WAIT after the first message has been received.
	WAIT_FOR_ONE = MSG_WAITFORONE,
	/// Operate in zerocopy I/O mode.
	/**
	 * In this mode the call will return immediately and the kernel will
	 * use the userspace buffer while the process continues to run. This
	 * of course opens the possibility for buffer corruption while the
	 * kernel still uses it. Kernel stability will not be affected, but
	 * the processed network data can suffer from this, naturally.
	 *
	 * This is currently only supported for TCP. See Linux kernel
	 * documentation `msg_zerocopy.rst`.
	 *
	 * When the kernel has finished transferring the data then an extended
	 * socket error message will be sent that can be obtained via
	 * `recvmsg()`.
	 *
	 * To use this flag socket SocketOptions::setZeroCopy() needs to be
	 * set on the socket.
	 **/
	ZEROCOPY = MSG_ZEROCOPY
};

using MessageFlags = BitMask<MessageFlag>;

} // end ns
