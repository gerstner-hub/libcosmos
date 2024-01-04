#pragma once

// C++
#include <string_view>

// cosmos
#include "cosmos/net/types.hxx"

namespace cosmos {

/// Network interface name to index mapping info.
/**
 * This type can only be obtained via InterfaceEnumerator.
 **/
struct InterfaceInfo :
		protected ::if_nameindex {

	/// Returns the network interface name.
	std::string_view name() const {
		return std::string_view{this->if_name};
	}

	/// Returns the network interface index.
	InterfaceIndex index() const {
		return InterfaceIndex{static_cast<int>(this->if_index)};
	}

protected: // functions

	constexpr InterfaceInfo() :
			if_nameindex{0, nullptr} {
	}

	friend class InterfaceIterator;
	friend class InterfaceEnumerator;
};

} // end ns
