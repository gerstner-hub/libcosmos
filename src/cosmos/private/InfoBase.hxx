#ifndef COSMOS_PRIVATE_INFOBASE_HXX
#define COSMOS_PRIVATE_INFOBASE_HXX

// C++
#include <cstring>

// cosmos
#include "cosmos/error/ApiError.hxx"
#include "cosmos/error/RuntimeError.hxx"

namespace cosmos {

constexpr size_t BUF_INIT_SIZE = 512;
constexpr size_t BUF_MAX_SIZE = 65535;

template <typename DB_STRUCT>
bool InfoBase<DB_STRUCT>::getInfo(std::function<int(DB_STRUCT**)> get_func) {
	DB_STRUCT *res;
	m_buf.resize(BUF_INIT_SIZE);

	while (true) {
		const auto err = get_func(&res);
		switch(Errno{err}) {
			case Errno::NO_ERROR:
				return res != nullptr;
			case Errno::RANGE: m_buf.resize(m_buf.size() << 1); break;
			default: cosmos_throw(ApiError(Errno{err})); break;
		}

		if (m_buf.size() > BUF_MAX_SIZE) {
			// don't get too crazy here
			cosmos_throw(RuntimeError("buffer size limit reached"));
		}
	}

	return false;
}

template <typename DB_STRUCT>
void InfoBase<DB_STRUCT>::invalidate() {
	m_buf.clear();
	std::memset(&m_info, 0, sizeof(m_info));
	m_valid = false;
}

} // end ns

#endif // inc. guard
