// stdlib
#include <cstring>
#include <sstream>

// Linux
#include <errno.h>

// cosmos
#include "cosmos/errors/ApiError.hxx"

namespace cosmos {

ApiError::ApiError(const char *prefix) :
	ApiError(errno)
{
	if (prefix) {
		m_msg = prefix;
		m_msg += ": ";
	}
}

ApiError::ApiError(const int p_errno) :
	CosmosError("ApiError"),
	m_errno(p_errno)
{}

void ApiError::generateMsg() const {
	std::stringstream ss;

	ss << msg(m_errno) << " (" << m_errno << ")";

	m_msg += ss.str();
}

std::string ApiError::msg(const int no) {
	char error[512];

	const char *text = ::strerror_r(no, &error[0], sizeof(error));

	return text;
}

} // end ns
