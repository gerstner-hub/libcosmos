#pragma once

// C++
#include <string_view>
#include <string>

// cosmos
#include "cosmos/fs/FDFile.hxx"
#include "cosmos/net/SocketOptions.hxx"
#include "cosmos/net/types.hxx"
#include "cosmos/utils.hxx"

namespace cosmos {

class SocketAddress;
class SendMessageHeader;
class ReceiveMessageHeader;

/// Base class for Socket types with ownership of a FileDescriptor.
/**
 * Specializations of Socket carry ownership of a socket FileDescriptor. The
 * exact type of socket is defined by the specialization.
 *
 * This type inherits the StreamIO interface for operating using regular
 * streaming file I/O on the socket. Not all socket types support this (most
 * notably listening sockets that are only used to accept new connections).
 * These socket types mark the I/O APIs as protected in their implementation.
 *
 * Furthermore this Socket base class implements a range of Socket specific
 * operations and I/O functions, all of which are marked protected.
 * Specializations of Socket need to make these functions accessible as far as
 * they make sense for the concrete socket type.
 *
 * This base class also provides access to the basic SocketOptions for the
 * socket.
 **/
class COSMOS_API Socket :
		public FDFile {

public: // types

	/// Type used in Socket::shutdown().
	enum class Direction : int {
		READ = SHUT_RD,
		WRITE = SHUT_WR,
		READ_WRITE = SHUT_RDWR
	};

	/// Boolean flag used in receiveFrom() to signify if a peer address could be provided.
	using AddressFilledIn = NamedBool<struct addr_filled_in_t, false>;

public: // functions

	auto sockOptions() {
		return SocketOptions{m_fd};
	}

	auto sockOptions() const {
		return SocketOptions{m_fd};
	}

	/// Returns the current address that the socket is bound to, if any.
	void getSockName(SocketAddress &addr);

protected: // functions

	/// Creates a new socket using the given properties.
	Socket(
			const SocketFamily family,
			const SocketType type,
			const SocketFlags flags = SocketFlags{SocketFlag::CLOEXEC},
			const SocketProtocol protocol = SocketProtocol::DEFAULT);

	/// Creates a new socket from the given existing file descriptor.
	Socket(FileDescriptor fd, const AutoCloseFD auto_close) :
			FDFile{fd, auto_close}
	{}

	/// Bind the socket to the given local address.
	/**
	 * This operation is used for connection oriented sockets to define
	 * the local address at which it will be listening for new connections.
	 *
	 * For connectionless sockets this can be used to explicitly choose a
	 * local address for outgoing packets, and to receive packets at this
	 * address by default.
	 **/
	void bind(const SocketAddress &addr);

	/// Establish a new connection using the given destination address.
	/**
	 * For connection oriented sockets this performs the protocol
	 * operations to establish a new connection to the given address.
	 *
	 * For connectionless sockets this defines the default send
	 * destination and a filter for incoming packets.
	 *
	 * Depending on the non-blocking mode of the socket the connection
	 * will either block and only return successfully if the connection
	 * has been established. Or, if non-blocking mode is enabled, the call
	 * will return immediately, and the result can be obtained via
	 * SocketOption::lastError() at a later time. The connect() behaviour
	 * can also be influenced by further socket specific options.
	 **/
	void connect(const SocketAddress &addr);

	/// Shutdown part or all of the connection on protocol level.
	/**
	 * This is distinct from a close() operation in that it performs a
	 * graceful shutdown of a network connection. It can be limited to the
	 * receiving end, the sending end or affect both.
	 *
	 * On Linux this can also be used to stop a blocking `accept()` call
	 * on a listening socket.
	 **/
	void shutdown(const Direction dir);

	/// Enter into a passive listen state, allowing new connections.
	/**
	 * This is only possible for connection mode socket types, when you
	 * want to accept new connections. For client side sockets this is not
	 * necessary.
	 *
	 * \param[in] backlog The number of pending connections that may be
	 * queued in the kernel.
	 **/
	void listen(const size_t backlog);

	/// Accept a new connection on the socket.
	/**
	 * This is only possible for connection mode socket types that have
	 * been put into the listen state via the listen() function.
	 *
	 * The returned file descriptor refers to the new connection that has
	 * been accepted. The optional \c addr parameter will receive the
	 * address of the newly accepted connection.
	 *
	 * This is only a lower level function that returns an unmanaged file
	 * descriptor. The implementation needs to take care that the file
	 * descriptor is not lost but encapsulated in an object that can
	 * manage its lifetime.
	 **/
	FileDescriptor accept(SocketAddress *addr);

	/// Send the given data over the socket, using specific send flags.
	/**
	 * This is like a regular write() call but allows to specify socket
	 * specific MessageFlags to adjust various behavour.
	 **/
	size_t send(const void *buf, size_t length, const MessageFlags flags = MessageFlags{});

	/// Variant of send() that takes a std::string_view container instead of a raw input buffer.
	size_t send(const std::string_view data, const MessageFlags flags = MessageFlags{}) {
		return send(data.data(), data.size(), flags);
	}

	/// Send a packet to a specific destination address.
	/**
	 * This call is like send() but takes a specific destination address
	 * to send the data to. This can be used e.g. with datagram sockets
	 * where one socket can be used to talk to multiple remote
	 * destinations.
	 *
	 * This is not supported for connection mode socket types (since there
	 * is only one possible peer address defined during connect() time).
	 **/
	size_t sendTo(const void *buf, size_t length, const SocketAddress &addr,
			const MessageFlags flags = MessageFlags{});

	/// Variant of sendTo() that takes a std::string_view container instead of a raw input buffer.
	size_t sendTo(const std::string_view data, const SocketAddress &addr,
			const MessageFlags flags = MessageFlags{}) {
		return sendTo(data.data(), data.size(), addr, flags);
	}

	/// Sends a message over the socket using extended SendMessageHeader data.
	/**
	 * This variant of the send family of functions allows the following
	 * advanced features compared to sendTo():
	 *
	 * - sending data from multiple scatter/gather memory locations using
	 *   the WriteIOVector `iovec` member of `header`.
	 * - passing additional ancillary data using the `control_msg` member
	 *   of `header`.
	 *
	 * If \c addr is passed then this specific target address is used for
	 * sending the data.
	 *
	 * The amount of sent data is reflected in an updated `iovec` member
	 * of the `header` parameter. Partial sends are possible. If a
	 * `control_msg` is set in `header` and at least one succesful (even
	 * partial) sendMessage() call was performed, then the control message
	 * will have been processed completely. The `control_msg` member will
	 * be reset automaically in this case to avoid it being sent multiple
	 * times without intent.
	 **/
	void sendMessage(SendMessageHeader &header, const SocketAddress* addr = nullptr);

	/// Receive data from the socket, using specific receive flags.
	/**
	 * This is like a regular read() call but allows to specify socket
	 * specific MessageFlags to adjust various behaviour.
	 **/
	size_t receive(void *buf, size_t length, const MessageFlags flags = MessageFlags{});

	/// Receive a packet, filling in the sender's address.
	/**
	 * This call is like receive() but fills in the sender's address in \c
	 * addr, if possible. This generally doesn't make sense with
	 * connection mode socket types (where the peer is defined during
	 * connect() time). If the sender's address isn't available then \c
	 * addr is left unchanged.
	 *
	 * The returned pair contains the number of bytes written to \c buf
	 * and a boolean type indicating whether \c addr was filled in or not.
	 **/
	std::pair<size_t, AddressFilledIn> receiveFrom(
			void *buf, size_t length, SocketAddress &addr,
			const MessageFlags flags = MessageFlags{});

	/// Receives a message from the socket using extended ReceiveMessageHeader data.
	/**
	 * This variant of the receive family of functions allows the
	 * following advanced features compared to recvFrom():
	 *
	 * - receiving data into multiple scatter/gather memory locations
	 *   using the ReadIOVector `iovec` member of `header`.
	 * - reception of additional ancillary data using
	 *   ReceiveMessageHeader::setControlBufferSize().
	 *
	 * If \c addr is passed then the source address of the message is
	 * placed in this SocketAddress structure, if one is available.
	 * Whether it was filled in is indicated by the return value.
	 *
	 * The amount of received data is reflected in an updated `iovec`
	 * member of the `header` parameter. Partial receives are possible. If
	 * the control message buffer is setup in `header` then you need to
	 * check for received ancillary messages using the iterator interface
	 * of `header`. Control messages can have side effects when using them
	 * on UNIX domain sockets (file descriptors being allocated in the
	 * receiving process). Therefore accepting ancillary messages should
	 * not be taken lightly.
	 **/
	AddressFilledIn receiveMessage(ReceiveMessageHeader &header, SocketAddress *addr = nullptr);
};

} // end ns
