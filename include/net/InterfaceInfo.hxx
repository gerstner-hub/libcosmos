#pragma once

// cosmos
#include <cosmos/net/types.hxx>
#include <cosmos/SysString.hxx>

namespace cosmos {

/// Network interface name to index mapping info.
/**
 * This type can only be obtained via InterfaceEnumerator. Is is only a view
 * onto the underlying InterfaceEnumerator data and its lifetime is tied to
 * it.
 **/
struct InterfaceInfo {

	/// Returns the network interface name.
	SysString name() const {
		return SysString{m_info->if_name};
	}

	/// Returns the network interface index.
	InterfaceIndex index() const {
		return InterfaceIndex{static_cast<int>(m_info->if_index)};
	}

protected: // functions

	explicit InterfaceInfo(const struct if_nameindex *info) :
			m_info{info} {
	}

protected: // data

	const struct if_nameindex *m_info = nullptr;

	friend class InterfaceIterator;
	friend class InterfaceEnumerator;
};

} // end ns
