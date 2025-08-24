// C++
#include <cstring>
#include <sstream>

// cosmos
#include <cosmos/error/ApiError.hxx>
#include <cosmos/private/strerror.hxx>
#include <cosmos/utils.hxx>

namespace cosmos {

ApiError::ApiError(const std::string_view prefix,
		const SourceLocation &src_loc) :
		ApiError{prefix, Errno{errno}, src_loc} {
}

ApiError::ApiError(const std::string_view prefix, const Errno err,
			const SourceLocation &src_loc) :
		CosmosError{"ApiError", {}, src_loc},
		m_errno{err} {
	if (!prefix.empty()) {
		m_msg = prefix;
		m_msg += ": ";
	}
}

void ApiError::generateMsg() const {
	std::stringstream ss;
	ss << m_errno;

	m_msg += ss.str();
}

std::string ApiError::msg(const Errno err) {
	char error[512];

	auto ret = xsi_strerror_r(to_integral(err), &error[0], sizeof(error));

	if (ret != 0) {
		std::stringstream ss;
		ss << "failed to format errno (error code:" << (ret < 0 ? errno : ret) << ")";
		return ss.str();
	}

	return std::string{error};
}

} // end ns
