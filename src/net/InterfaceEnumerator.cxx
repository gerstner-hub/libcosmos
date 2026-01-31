// cosmos
#include <cosmos/error/ApiError.hxx>
#include <cosmos/net/InterfaceEnumerator.hxx>

namespace cosmos {

void InterfaceEnumerator::clear() {
	if (m_list) {
		::if_freenameindex(m_list);
		m_list = nullptr;
	}
}

void InterfaceEnumerator::fetch() {
	clear();
	m_list = ::if_nameindex();

	if (!m_list) {
		throw ApiError{"ifnameindex()"};
	}
}

} // end ns
