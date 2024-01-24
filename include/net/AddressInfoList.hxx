#pragma once

// cosmos
#include "cosmos/dso_export.h"
#include "cosmos/net/AddressHints.hxx"
#include "cosmos/net/AddressInfo.hxx"
#include "cosmos/net/AddressInfoIterator.hxx"

namespace cosmos {

/// Resolve DNS names and provide the resulting list of AddressInfos.
/**
 * This type allows to resolve internet host names and service names into
 * SocketAddress types suitable for binding a socket on or for connecting a
 * socket to.
 *
 * This API is restricted to IP based porotocols. By default it reports socket
 * addresses for all available combinations socket families
 * (SocketFamily::INET and SocketFamily::INET6) and socket types (e.g.
 * SocketType::STREAM and SocketType::DGRAM). To filter the result list the
 * AddressHints structure is used which can be set via setHints() or
 * manipulated in-place using hints(). The default flags for AddressHints are
 * {AddressHints::Flag::V4_MAPPED, AddressHints::Flag::ADDR_CONFIG}. This
 * matches the default behaviour of `getaddrinfo()` with no hints provided.
 *
 * To iterate over the result list the AddressInfoIterator type is provided. A
 * simple range based for loop does the trick for iterating over all results.
 **/
class COSMOS_API AddressInfoList {
public: // functions

	~AddressInfoList() {
		clear();
	}

	/// Resolve addresses for the given node/service name combination.
	/**
	 * The given `node` name is either an internet host name to be
	 * resolved, or a numerical IP address. If
	 * AddressHints::Flag::NUMERIC_HOST is set then it is always expected
	 * to be a numerical IP address string.
	 *
	 * If `node` is empty and AddressHints::Flag::PASSIVE is set then an
	 * address suitable for binding (listening) on is returned.
	 *
	 * If `node` is empty and AddressHints::Fag::PASSIVE is *not* set
	 * then an address based on the loopback device suitable for
	 * communication on the localhost is returned.
	 *
	 * The given `service` is either an IP service name as found in
	 * /etc/services, or a numerical IP port number. If
	 * AddressHints::Flag::NUMERIC_SERVICE is set then it is always
	 * expected to be a numerical port string.
	 *
	 * If `service` is empty then the returned socket addresses will have
	 * no port portion set.
	 *
	 * Either `node` or `service` may be specified as empty, but not
	 * both.
	 *
	 * If resolving the requested parameters fails then a specialized
	 * ResolveError exception is thrown to describe the error.
	 **/
	void resolve(const SysString node, const SysString service);

	/// Clear a previously stored resolve result.
	void clear();

	/// Access the stored AddressHints to modify the resolve behaviour.
	/**
	 * Changes to the AddressHints will only become effective for the next
	 * call to resolve().
	 **/
	auto& hints() {
		return m_hints;
	}

	/// Set a new AddressHints structure for modifying the resolve behaviour.
	/**
	 * \see hints()
	 **/
	void setHints(const AddressHints &hints) {
		m_hints = hints;
	}

	/// Returns whether currently a valid resolve result is stored.
	/**
	 * If there is no resolve result then begin() equals end() and
	 * iterating over the list yields no elements.
	 **/
	bool valid() const {
		return m_addrs != nullptr;
	}

	AddressInfoIterator begin() const {
		return AddressInfoIterator{reinterpret_cast<const AddressInfo*>(m_addrs)};
	}

	AddressInfoIterator end() const {
		return AddressInfoIterator{nullptr};
	}

protected: // data

	AddressHints m_hints;
	struct addrinfo *m_addrs = nullptr;
};

} // end ns
