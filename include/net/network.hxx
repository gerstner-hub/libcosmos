#pragma once

// C++
#include <utility>

// cosmos
#include "cosmos/net/UnixConnection.hxx"
#include "cosmos/net/UnixDatagramSocket.hxx"

namespace cosmos::net {

/// Creates a pair of unnamed connected SocketType::STREAM unix domains sockets
COSMOS_API std::pair<UnixConnection, UnixConnection> create_stream_socket_pair();
/// Creates a pair of unnamed connected SocketType::SEQPACKET unix domain sockets
COSMOS_API std::pair<UnixConnection, UnixConnection> create_seqpacket_socket_pair();
/// Creates a pair of unnamed connected SocketType::DGRAM unix domain sockets
COSMOS_API std::pair<UnixDatagramSocket, UnixDatagramSocket> create_dgram_socket_pair();

} // end ns
