// Linux
#include <sys/utsname.h>

// cosmos
#include <cosmos/error/ApiError.hxx>
#include <cosmos/uname.hxx>

namespace cosmos {

Uname::Uname(const bool fetch_data) : m_buf{new utsname} {
	if (fetch_data) {
		update();
	} else {
		m_buf->sysname[0] = 0;
		m_buf->nodename[0] = 0;
		m_buf->domainname[0] = 0;
		m_buf->release[0] = 0;
		m_buf->version[0] = 0;
		m_buf->machine[0] = 0;
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

void Uname::update() {
	if (::uname(m_buf.get()) != 0) {
		throw ApiError{"uname()"};
	}
}

} // end ns
