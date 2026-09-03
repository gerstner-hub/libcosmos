// cosmos
#include <cosmos/net/netlink/NetlinkOptions.hxx>
#include <cosmos/private/sockopts.hxx>

namespace cosmos {

void NetlinkOptions::setEnablePacketInfo(const bool on_off) {
	setBoolOption(OptName{NETLINK_PKTINFO}, on_off);
}

bool NetlinkOptions::getEnablePacketInfo() const {
	return getBoolOption(OptName{NETLINK_PKTINFO});
}

void NetlinkOptions::setEnableBroadcastError(const bool on_off) {
	setBoolOption(OptName{NETLINK_BROADCAST_ERROR}, on_off);
}

bool NetlinkOptions::getEnableBroadcastError() const {
	return getBoolOption(OptName{NETLINK_BROADCAST_ERROR});
}

void NetlinkOptions::setDisableNoBufsError(const bool disable) {
	setBoolOption(OptName{NETLINK_NO_ENOBUFS}, disable);
}

bool NetlinkOptions::getDisableNoBufsError() const {
	return getBoolOption(OptName{NETLINK_NO_ENOBUFS});
}

void NetlinkOptions::setEnableListenAllNSIDs(const bool on_off) {
	setBoolOption(OptName{NETLINK_LISTEN_ALL_NSID}, on_off);
}

bool NetlinkOptions::getEnableListenAllNSIDs() const {
	return getBoolOption(OptName{NETLINK_LISTEN_ALL_NSID});
}

void NetlinkOptions::setEnableCapACKs(const bool on_off) {
	setBoolOption(OptName{NETLINK_CAP_ACK}, on_off);
}

bool NetlinkOptions::getEnableCapAcks() const {
	return getBoolOption(OptName{NETLINK_CAP_ACK});
}

void NetlinkOptions::setExtendedACKs(const bool on_off) {
	setBoolOption(OptName{NETLINK_EXT_ACK}, on_off);
}

bool NetlinkOptions::getExtendedACKs() const {
	return getBoolOption(OptName{NETLINK_EXT_ACK});
}

void NetlinkOptions::setGetStrictCheck(const bool on_off) {
	setBoolOption(OptName{NETLINK_GET_STRICT_CHK}, on_off);
}

bool NetlinkOptions::getGetStrictCheck() const {
	return getBoolOption(OptName{NETLINK_GET_STRICT_CHK});
}

void NetlinkOptions::addMembership(const NetlinkGroup group) {
	setUIntOption(OptName{NETLINK_ADD_MEMBERSHIP}, to_integral(group));
}

void NetlinkOptions::dropMembership(const NetlinkGroup group) {
	setUIntOption(OptName{NETLINK_DROP_MEMBERSHIP}, to_integral(group));
}

std::set<NetlinkGroup> NetlinkOptions::listMemberships() const {
	std::vector<uint32_t> chunks;
	// let's start out with 64 groups and increase as needed
	chunks.resize(64);
	socklen_t have_bytes, needed_bytes;

	for (size_t i = 0; i< 32; i++) {
		have_bytes = chunks.size() * sizeof(uint32_t);
		needed_bytes = getsockopt(m_sock, OptLevel::NETLINK,
				OptName{NETLINK_LIST_MEMBERSHIPS},
				chunks.data(), have_bytes);

		chunks.resize(needed_bytes / sizeof(uint32_t));

		if (needed_bytes <= have_bytes) {
			break;
		}  else {
			continue;
		}
	}

	if (needed_bytes > have_bytes) {
		throw RuntimeError{"unable to establish proper size of memberships array"};
	}

	std::set<NetlinkGroup> ret;

	for (const auto group: chunks) {
		ret.insert(NetlinkGroup{group});
	}

	return ret;
}

} // end ns
