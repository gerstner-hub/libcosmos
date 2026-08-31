// cosmos
#include <cosmos/memory.hxx>
#include <cosmos/net/packet/PacketOptions.hxx>
#include <cosmos/private/sockopts.hxx>

namespace cosmos {

using MembershipReq = PacketOptions::MembershipReq;

MembershipReq::MembershipReq(const InterfaceIndex index) {
	zero_object(static_cast<struct packet_mreq&>(*this));
	setIndex(index);
}

const MembershipReq& MembershipReq::setPromiscuous() {
	this->mr_type = PACKET_MR_PROMISC;
	return *this;
}

const MembershipReq& MembershipReq::addMulticast(const MACAddress &addr) {
	this->mr_type = PACKET_MR_MULTICAST;
	setMAC(addr);
	return *this;
}

const MembershipReq& MembershipReq::setAllMulticast() {
	this->mr_type = PACKET_MR_ALLMULTI;
	return *this;
}

const MembershipReq& MembershipReq::addUnicast(const MACAddress &addr) {
	this->mr_type = PACKET_MR_UNICAST;
	setMAC(addr);
	return *this;
}

void MembershipReq::setMAC(const MACAddress &addr) {
	this->mr_alen = addr.size();
	std::memcpy(this->mr_address, addr.data(), addr.size());
}

void PacketOptions::addMembership(const MembershipReq &req) {
	setsockopt(m_sock, M_LEVEL, OptName{PACKET_ADD_MEMBERSHIP}, req);
}

void PacketOptions::dropMembership(const MembershipReq &req) {
	setsockopt(m_sock, M_LEVEL, OptName{PACKET_DROP_MEMBERSHIP}, req);
}

void PacketOptions::enableAuxData(const bool on_off) {
	setBoolOption(OptName{PACKET_AUXDATA}, on_off);
}

void PacketOptions::setQDiscBypass(const bool on_off) {
	setBoolOption(OptName{PACKET_QDISC_BYPASS}, on_off);
}

} // end ns
