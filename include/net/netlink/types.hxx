#pragma once

// C++
#include <cstdint>

// Linux
#include <linux/netlink.h>

namespace cosmos {

/// Netlink unicast port ID.
/**
 * This is the unicast identifier used by userspace programs for
 * addressing purposes. The value 0 is special and refers to the
 * kernel.
 *
 * While `netlink(7)` claims this is of type `pid_t`, it is actually
 * declared as `uint32_t` in system headers these days.
 **/
enum class NetlinkPort : uint32_t {
	KERNEL = 0
};

/// Supported Netlink sub-protocols.
/**
 * These are the values supported for the `protocol` field in Netlink socket
 * creation. They define the application-specific protocol based on Netlink.
 **/
enum class NetlinkFamily : int {
	ROUTE          = NETLINK_ROUTE,
	USERSOCK       = NETLINK_USERSOCK,  ///< user-mode socket protocols.
	SOCK_DIAG      = NETLINK_SOCK_DIAG, ///< information about various protocol families.
	XFRM           = NETLINK_XFRM,      ///< IPsec.
	SELINUX        = NETLINK_SELINUX,   ///< SELinux event notifications.
	ISCSI          = NETLINK_ISCSI,     ///< Open-iSCSI.
	AUDIT          = NETLINK_AUDIT,     ///< Auditing.
	FIB_LOOKUP     = NETLINK_FIB_LOOKUP,
	CONNECTOR      = NETLINK_CONNECTOR, ///< kernel connector.
	NETFILTER      = NETLINK_NETFILTER,
	SCSI_RANSPORT  = NETLINK_SCSITRANSPORT,
	RDMA           = NETLINK_RDMA,      ///< Infiniband RDMA.
	IP6_FW         = NETLINK_IP6_FW,    ///< ip6_queue module.
	DNRTMSG        = NETLINK_DNRTMSG,   ///< DECnet routing messages.
	KOBJECT_UEVENT = NETLINK_KOBJECT_UEVENT, ///< kernel messages to user space.
	GENERIC        = NETLINK_GENERIC,
	CRYPTO         = NETLINK_CRYPTO,    ///< information about and configuration of kernel crypto API.
};

/// A multicast group ID.
enum class NetlinkGroup : uint32_t {
};

} // end ns
