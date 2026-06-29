// Linux
#include <sys/utsname.h>

// cosmos
#include <cosmos/error/ApiError.hxx>
#include <cosmos/uname.hxx>

namespace cosmos {

Uname::Uname() : m_buf{new utsname} {
	if (::uname(m_buf.get()) != 0) {
		throw ApiError{"uname()"};
	}
}

/* this needs to be in the compilation unit to make unique_ptr work with the
 * incomplete utsname type */
Uname::~Uname() = default;

std::string_view Uname::sysName() const {
	return m_buf->sysname;
}

std::string_view Uname::nodeName() const {
	return m_buf->nodename;
}

std::string_view Uname::domainName() const {
	return m_buf->domainname;
}

std::string_view Uname::release() const {
	return m_buf->release;
}

std::string_view Uname::version() const {
	return m_buf->version;
}

std::string_view Uname::machine() const {
	return m_buf->machine;
}

} // end ns
