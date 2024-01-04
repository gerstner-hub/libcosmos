// cosmos
#include "cosmos/error/RuntimeError.hxx"
#include "cosmos/net/LinkLayerAddress.hxx"

namespace cosmos {

MACAddress LinkLayerAddress::macAddress() const {
	MACAddress ret;
	if (m_addr.sll_halen != ret.size()) {
		cosmos_throw(RuntimeError("LinkLayerAddress does not contain a MAC address"));
	}

	std::memcpy(ret.data(), m_addr.sll_addr, ret.size());
	return ret;
}

} // end ns
