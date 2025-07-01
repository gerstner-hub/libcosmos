// cosmos
#include <cosmos/error/ApiError.hxx>
#include <cosmos/net/InterfaceEnumerator.hxx>

namespace cosmos {

// this type is only supposed to be a thin C++ wrapper that can be casted
// directory to the plain C struct
static_assert(sizeof(InterfaceInfo) == sizeof(struct if_nameindex));

void InterfaceEnumerator::clear() {
	if (m_list) {
		::if_freenameindex(m_list);
		m_list = nullptr;
	}
}

void InterfaceEnumerator::fetch() {
	clear();
	m_list = reinterpret_cast<InterfaceInfo*>(::if_nameindex());

	if (!m_list) {
		throw ApiError{"ifnameindex()"};
	}
}

} // end ns
