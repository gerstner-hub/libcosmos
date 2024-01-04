#pragma once

// C++
#include <string_view>
#include <utility>

// cosmos
#include "cosmos/net/UnixConnection.hxx"
#include "cosmos/net/UnixDatagramSocket.hxx"
#include "cosmos/net/types.hxx"

namespace cosmos::net {

/// Creates a pair of unnamed connected SocketType::STREAM unix domains sockets
COSMOS_API std::pair<UnixConnection, UnixConnection>         create_stream_socket_pair();
/// Creates a pair of unnamed connected SocketType::SEQPACKET unix domain sockets
COSMOS_API std::pair<UnixConnection, UnixConnection>         create_seqpacket_socket_pair();
/// Creates a pair of unnamed connected SocketType::DGRAM unix domain sockets
COSMOS_API std::pair<UnixDatagramSocket, UnixDatagramSocket> create_dgram_socket_pair();

/// Translates a network interface name to an InterfaceIndex.
/**
 * Each network interface in the system has a unique InterfaceIndex. Given the
 * human readable interface \c name this function returns the corresponding
 * index. On error an ApiError is thrown.
 **/
COSMOS_API InterfaceIndex nameToIndex(const std::string_view name);

/// Translates an InterfaceIndex to a human readable network interface name.
/**
 * This performs the reverse operation of \c nameToIndex().
 **/
COSMOS_API std::string    indexToName(const InterfaceIndex index);

} // end ns
