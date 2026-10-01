#pragma once

#include <cstdint>

namespace cosmos {

/**
 * @file
 *
 * Basic types for Linux namespaces.
 **/

/// An identifier for a network namespace.
/*
 * This is a system-wide unique identifier for a network namespace. It needs
 * to be kept apart from a namespace inode (which is 32-bit `int`) and a
 * network namespace netlink ID (which is another 32-bit `int`).
 **/
enum class NetworkNS : uint64_t {
};

/// A Netlink identifier for a network namespace.
/**
 * This network namespace identifier is specific to Netlink APIs and is not
 * system-wide unique, but differs depending on from which namespace the query
 * is made.
 **/
enum class NetlinkNSID : int {
};

} // end ns
