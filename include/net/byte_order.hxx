#ifndef COSMOS_BYTE_ORDER_HXX
#define COSMOS_BYTE_ORDER_HXX

// Linux
#include <arpa/inet.h>

namespace cosmos::net {

inline uint16_t to_network_order(uint16_t host) {
	return ::htons(host);
}

inline uint32_t to_host_order(uint32_t host) {
	return ::htonl(host);
}

inline uint16_t to_host_order(uint16_t network) {
	return ::ntohs(network);
}

inline uint32_t to_network_order(uint32_t network) {
	return ::ntohl(network);
}

} // end ns

#endif // inc. guard
