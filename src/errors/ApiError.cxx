// stdlib
#include <cstring>
#include <iostream>
#include <sstream>

// Linux
#include <errno.h>

// cosmos
#include "cosmos/algs.hxx"
#include "cosmos/errors/ApiError.hxx"

namespace cosmos {

ApiError::ApiError(const std::optional<std::string_view> prefix) :
	ApiError(Errno(errno))
{
	if (prefix) {
		m_msg = prefix.value();
		m_msg += ": ";
	}
}

ApiError::ApiError(const Errno err) :
	CosmosError("ApiError"),
	m_errno(err)
{}

void ApiError::generateMsg() const {
	std::stringstream ss;
	ss << m_errno;

	m_msg += ss.str();
}

std::string ApiError::msg(const Errno err) {
	char error[512];

	const char *text = ::strerror_r(to_integral(err), &error[0], sizeof(error));

	return text;
}

} // end ns

std::ostream& operator<<(std::ostream &o, const cosmos::Errno err) {
	o << cosmos::ApiError::msg(err) << " (" << cosmos::to_integral(err) << ")";
	return o;
}
