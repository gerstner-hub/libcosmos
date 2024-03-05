// cosmos
#include <cosmos/net/IPOptions.hxx>
#include <cosmos/private/sockopts.hxx>

namespace cosmos {

// this requires linux 6.3 headers - define it ourselves if necessary
#ifndef IP_LOCAL_PORT_RANGE
#	define IP_LOCAL_PORT_RANGE 51
#endif
void IP4Options::setLocalPortRange(const uint16_t lower_bound, const uint16_t upper_bound) {
	// the upper 16 bits are the upper_bound, the lower 16 bits the lower bound.
	const uint32_t setting = (static_cast<uint32_t>(upper_bound) << 16) |
		static_cast<uint32_t>(lower_bound);
	setsockopt(m_sock, M_LEVEL, OptName{IP_LOCAL_PORT_RANGE}, setting);
}

std::pair<uint16_t, uint16_t> IP4Options::getLocalPortRange() const {
	const auto bounds = getsockopt<uint32_t>(m_sock, M_LEVEL, OptName{IP_LOCAL_PORT_RANGE});

	const uint16_t lower_bound = static_cast<uint16_t>(bounds);
	const uint16_t upper_bound = static_cast<uint16_t>(bounds >> 16);

	return std::make_pair(upper_bound, lower_bound);
}

void IP4Options::setTypeOfService(const ToS tos) {
	setsockopt(m_sock, M_LEVEL, OptName{IP_TOS}, to_integral(tos));
}

IP4Options::ToS IP4Options::getTypeOfService() const {
	const auto tos = getsockopt<uint8_t>(m_sock, M_LEVEL, OptName{IP_TOS});
	return ToS{tos};
}

}; // end ns
