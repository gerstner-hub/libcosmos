// cosmos
#include <cosmos/error/RuntimeError.hxx>
#include <cosmos/net/netlink/NetlinkAddress.hxx>

namespace cosmos {

namespace {

void verify_group(const NetlinkGroup group) {
	if (to_integral(group) > 31) {
		throw RuntimeError{"multicast group value too large for 32-bit mask"};
	}
}

} // end anon ns

bool NetlinkAddress::isGroupSet(const NetlinkGroup group) const {
	verify_group(group);
	const auto mask = groupMask();

	return mask[1 << to_integral(group)];
}

void NetlinkAddress::addGroup(const NetlinkGroup group) {
	verify_group(group);

	auto mask = groupMask();
	mask.set(1 << to_integral(group), true);
	setGroupMask(mask);
}

} // end ns
