// cosmos
#include <cosmos/memory.hxx>
#include <cosmos/net/netlink/headers.hxx>

namespace cosmos {

NetlinkHeader::NetlinkHeader(const nlmsghdr *hdr) {
	if (hdr) {
		std::memcpy(this, hdr, sizeof(nlmsghdr));
	} else {
		zero_object(*raw());
	}
}

void NetlinkErrorMsg::setMsgHeader(const NetlinkHeader &hdr) {
	std::memcpy(&this->msg, &hdr, sizeof(hdr));
}

static_assert(sizeof(NetlinkHeader) == sizeof(nlmsghdr), "nlmsghdr vs. NetlinkHeader size mismatch");
static_assert(sizeof(NetlinkErrorMsg) == sizeof(nlmsgerr), "nlmsgerr vs. NetlinkErrorMsg size mismatch");

} // end ns
