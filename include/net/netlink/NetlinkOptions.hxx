#pragma once

// C++
#include <set>

// cosmos
#include <cosmos/net/SockOptBase.hxx>
#include <cosmos/net/netlink/NetlinkAddress.hxx>

namespace cosmos {

class COSMOS_API NetlinkOptions :
		public SockOptBase<OptLevel::NETLINK> {
	friend class NetlinkSocket;
public: // functions

	/// Enables reception of NetlinkPacketInfo auxiliary message.
	void setEnablePacketInfo(const bool on_off);

	/// Get the current NetlinkPacketInfo auxiliary message setting.
	bool getEnablePacketInfo() const;

	/// Enables ENOBUFS error reports in broadcast messages, which are otherwise ignored.
	void setEnableBroadcastError(const bool on_off);

	/// Get the current error report behaviour for broadcasts.
	bool getEnableBroadcastError() const;

	/// Disables the generation of ENOBUFS errors.
	void setDisableNoBufsError(const bool disable);

	/// Get the current ENOBUFS error settings.
	bool getDisableNoBufsError() const;

	/// Enable reception of Netlink notifications from all network namespaces.
	/**
	 * The `nsid` the message refers to will be provided via ancillary
	 * data, see `NetlinkNamespaceIDMessage`. Setting this option requires
	 * `CAP_NET_BROADCAST` in the user namespace the socket was created
	 * with.
	 **/
	void setEnableListenAllNSIDs(const bool on_off);

	/// Get the current network namespace reception setting.
	bool getEnableListenAllNSIDs() const;

	/// Allow the kernel to cut-off the original Netlink message in ACK replies.
	/**
	 * This prevents allocation errors in the kernel due to the necessity
	 * to loop back the original Netlink message in ACK messages. Instead
	 * application can guess the ACK the message relates to based on the
	 * sequence ID in the header.
	 **/
	void setEnableCapACKs(const bool on_off);

	/// Get the current cap ACK settings.
	bool getEnableCapAcks() const;

	/// Enable reporting of extended ACK information in NLMSG_ERROR and NLMSG_DONE messages.
	void setExtendedACKs(const bool on_off);

	/// Get the current extended ACK setting of the socket.
	bool getExtendedACKs() const;

	/// Enables strict error checking in GET requests for NetlinkFamily::ROUTE messages.
	/**
	 * Historically the kernel did not check the validity of message
	 * fields that have been unused. This made future changes to the
	 * protocol difficult without potentially breaking existing
	 * applications.
	 *
	 * This option allows to opt-in to strict checking of the respective
	 * requests, potentially resulting in additional error responses when
	 * bad messages are sent to the kernel. This only affects GET requests
	 * in the NetlinkFamily::ROUTE protocol.
	 **/
	void setGetStrictCheck(const bool on_off);

	/// Returns the current get-strict-check setting.
	bool getGetStrictCheck() const;

	/// Become a member of the given multicast `group`.
	/**
	 * Contrary to NetlinkAddress, this API allows the use of multicast
	 * group values > 32.
	 **/
	void addMembership(const NetlinkGroup group);

	/// Leave the multicast `group`.
	void dropMembership(const NetlinkGroup group);

	/// Returns a set describing the multicast group memberships of the socket.
	std::set<NetlinkGroup> listMemberships() const;

protected: // functions

	using SockOptBase::SockOptBase;
};

} // end ns
