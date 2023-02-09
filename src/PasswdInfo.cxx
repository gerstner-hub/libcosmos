// c++
#include <functional>
#include <cstring>

// cosmos
#include "cosmos/algs.hxx"
#include "cosmos/errors/ApiError.hxx"
#include "cosmos/errors/RuntimeError.hxx"
#include "cosmos/PasswdInfo.hxx"

namespace cosmos {

namespace {

constexpr size_t BUF_INIT_SIZE = 512;

bool handleGetPW(std::function< int(struct passwd **)> getpw, std::vector<char> &buf) {
	struct passwd *res;
	buf.resize(BUF_INIT_SIZE);

	while (true) {
		const auto err = getpw(&res);
		switch(Errno{err}) {
			case Errno::NO_ERROR:
				return res != nullptr;
			case Errno::RANGE: buf.resize(buf.size() << 1); break;
			default: cosmos_throw(ApiError(Errno{err})); break;
		}

		if (buf.size() > 65535) {
			// don't get too crazy here
			cosmos_throw(RuntimeError("buffer size limit reached"));
		}
	}

	return false;
}

} // end anon ns

PasswdInfo::PasswdInfo(const std::string_view name) {
	auto call = [&](struct passwd **res) -> int {
		return getpwnam_r(name.data(), &m_passwd,
			m_buf.data(), m_buf.size(),
			res);
	};

	m_valid = handleGetPW(call, m_buf);
	if (!m_valid)
		invalidate();
}

PasswdInfo::PasswdInfo(const UserID uid) {
	auto call = [&](struct passwd **res) -> int {
		return getpwuid_r(to_integral(uid), &m_passwd,
			m_buf.data(), m_buf.size(),
			res);
	};

	m_valid = handleGetPW(call, m_buf);
	if (!m_valid)
		invalidate();
}

void PasswdInfo::invalidate() {
	m_buf.clear();
	std::memset(&m_passwd, 0, sizeof(m_passwd));
	m_valid = false;
}

} // end ns
