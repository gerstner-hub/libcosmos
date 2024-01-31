#pragma once

// C++
#include <utility>

// cosmos
#include "cosmos/SysString.hxx"
#include "cosmos/net/UnixConnection.hxx"
#include "cosmos/net/UnixDatagramSocket.hxx"
#include "cosmos/net/types.hxx"

namespace cosmos::net {

/// Creates a pair of unnamed connected SocketType::STREAM unix domains sockets
COSMOS_API std::pair<UnixConnection, UnixConnection>         create_stream_socket_pair(
		const SocketFlags flags = SocketFlags{SocketFlag::CLOEXEC});
/// Creates a pair of unnamed connected SocketType::SEQPACKET unix domain sockets
COSMOS_API std::pair<UnixConnection, UnixConnection>         create_seqpacket_socket_pair(
		const SocketFlags flags = SocketFlags{SocketFlag::CLOEXEC});
/// Creates a pair of unnamed connected SocketType::DGRAM unix domain sockets
COSMOS_API std::pair<UnixDatagramSocket, UnixDatagramSocket> create_dgram_socket_pair(
		const SocketFlags flags = SocketFlags{SocketFlag::CLOEXEC});

/// Translates a network interface name to an InterfaceIndex.
/**
 * Each network interface in the system has a unique InterfaceIndex. Given the
 * human readable interface `name` this function returns the corresponding
 * index. On error an ApiError is thrown.
 **/
COSMOS_API InterfaceIndex name_to_index(const SysString name);

/// Translates an InterfaceIndex to a human readable network interface name.
/**
 * This performs the reverse operation of name_to_index().
 **/
COSMOS_API std::string    index_to_name(const InterfaceIndex index);

/// Returns the network hostname of the current process.
/**
 * On Linux this returns the hostname of the current UTS namespace, which can
 * change within container environments.
 **/
COSMOS_API std::string get_hostname();

} // end ns
