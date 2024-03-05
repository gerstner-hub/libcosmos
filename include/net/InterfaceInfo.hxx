#pragma once

// cosmos
#include <cosmos/net/types.hxx>
#include <cosmos/SysString.hxx>

namespace cosmos {

/// Network interface name to index mapping info.
/**
 * This type can only be obtained via InterfaceEnumerator.
 **/
struct InterfaceInfo :
		protected ::if_nameindex {

	/// Returns the network interface name.
	SysString name() const {
		return SysString{this->if_name};
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
