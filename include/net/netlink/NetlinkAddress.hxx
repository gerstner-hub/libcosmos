#pragma once

// C++
#include <bitset>

// cosmos
#include <cosmos/net/SocketAddress.hxx>
#include <cosmos/net/netlink/types.hxx>

namespace cosmos {

/// A Netlink unicast or multicast address.
/**
 * The Netlink protocol uses "ports" for unicast addressing. These are simply
 * 32-bit unsigned integers where every participant needs to have a unique
 * value. The special value 0 is used for the kernel. Formerly the term "pid"
 * was used for unicast addresses and application were supposed to use their
 * process ID as unicast address. Since this only works for one Netlink socket
 * in a process the model did out work out well and it was changed to "port".
 *
 * To select a random free port call bind() with NetlinkPort{0} to let the
 * kernel select the next free port value to use.
 *
 * There also exist multicast groups. The address structure contains a 32-bit
 * mask where every bit represents a group. This limits the amount of groups
 * to 32 and is no longer sufficient these days, which is why the kernel
 * offers Netlink socket options to add and remove multicast membership even
 * for larger group values. See NetlinkOptions::addMembership().
 *
 * Receiving and sending multicast messages is subject to privilege checks
 * which are context-dependent with respect to the Netlink protocol in
 * effect.
 **/
class NetlinkAddress :
		public SocketAddress {
public: // types

	/// Netlink multicast groups consisting of 32 distinct groups (bits).
	using GroupMask = std::bitset<32>;

public: // functions

	explicit NetlinkAddress(const NetlinkPort port = NetlinkPort::KERNEL) {
		clear();
		setPort(port);
	}

	SocketFamily family() const override {
		return SocketFamily::NETLINK;
	}

	/// Returns the size of the structure, which is fixed for NetlinkAddress.
	size_t size() const override {
		return sizeof(m_addr);
	}

	NetlinkPort port() const {
		return NetlinkPort{m_addr.nl_pid};
	}

	void setPort(const NetlinkPort _port) {
		m_addr.nl_pid = to_integral(_port);
	}

	GroupMask groupMask() const {
		return GroupMask{m_addr.nl_groups};
	}

	void setGroupMask(const GroupMask groups) {
		m_addr.nl_groups = groups.to_ulong();
	}

	/// Checks whether the given multicast group is set in the address.
	/**
	 * If `group` cannot be represented in a 32-bit mask (i.e. is larger
	 * than 31) then an exception is thrown.
	 **/
	bool isGroupSet(const NetlinkGroup group) const;

	/// Adds the given multicast group to the multicast set.
	/**
	 * If `group` cannot be represented in 32-bit mask (i.e. is larger
	 * than 31) then an exception is thrown.
	 **/
	void addGroup(const NetlinkGroup group);

protected: // functions

	sockaddr* basePtr() override {
		return reinterpret_cast<sockaddr*>(&m_addr);
	}

	const sockaddr* basePtr() const override {
		return reinterpret_cast<const sockaddr*>(&m_addr);
	}

protected: // data

	sockaddr_nl m_addr;
};

} // end ns
