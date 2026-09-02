#pragma once

// Linux
#include <linux/if_packet.h>

// cosmos
#include <cosmos/net/SockOptBase.hxx>

namespace cosmos {

/// Options specific to PacketSockets.
class COSMOS_API PacketOptions :
		public SockOptBase<OptLevel::PACKET> {
	friend class PacketSocket;
public: // types

	/// Type used with addMembership() and dropMembership().
	/**
	 * Only one of the settings setPromiscuous(), setMulticast() or
	 * setAllMulticast() may be active at any given time. The functions
	 * return a reference to the own object to allow to use them in
	 * temporary objects like
	 * `PacketOptions{}.addMembership(MembershipReq{}.setPromiscuous())`.
	 **/
	class MembershipReq :
			protected packet_mreq {
		friend class PacketOptions;
	public: // functions

		/// Create a membership request for the given interface.
		MembershipReq(const InterfaceIndex index = InterfaceIndex::INVALID);

		void setIndex(const InterfaceIndex index) {
			this->mr_ifindex = cosmos::to_integral(index);
		}

		/// Enable promiscuous mode, reception of all packets.
		/**
		 * Normally a packet socket will only receive packets destined
		 * for the host. With this setting all packets observed will
		 * be delivered to the socket.
		 **/
		const MembershipReq& setPromiscuous();

		/// Enable reception of multicast packets sent to the given multicast `addr`.
		const MembershipReq& addMulticast(const MACAddress &addr);

		/// Enable reception of all multicast packets observed.
		const MembershipReq& setAllMulticast();

		/// Enable reception of packets destined for the additional unicast address `addr`.
		/**
		 * Contrary to promiscuous mode this explicitly only adds
		 * reception of packets destined for `addr` additionally to the
		 * hosts own address.
		 **/
		const MembershipReq& addUnicast(const MACAddress &addr);

	protected: // functions

		void setMAC(const MACAddress &addr);
	};

public: // functions

	/// Add receive membership options based on `req`.
	void addMembership(const MembershipReq &req);

	/// Drop a receive membership which was previously applied via addMembership().
	void dropMembership(const MembershipReq &req);

	/// Enables the reception of PacketAuxData via PacketSocket::receiveMessage().
	/**
	 * If set then the receiveMessage() call on a packet socket will
	 * deliver a control message which can be handled by the PacketAuxData
	 * type.
	 **/
	void setEnableAuxData(const bool on_off);

	/// Returns the current enable-aux-data setting.
	bool getEnableAuxData() const;

	/// Enables the bypass of the kernel's qdisc (traffic control) layer.
	/**
	 * This is useful to avoid throttling on a packet socket in case your
	 * application intends to brute-force load on a network for testing
	 * purposes.
	 **/
	void setQDiscBypass(const bool on_off);

	/// Return the current qdisc bypass setting.
	bool getQDiscByPass() const;

protected: // functions

	using SockOptBase::SockOptBase;
};

} // end ns
