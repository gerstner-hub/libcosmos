// cosmos
#include <cosmos/error/RuntimeError.hxx>
#include <cosmos/net/unix/UnixAddress.hxx>

namespace cosmos {

void UnixAddress::setPath(const std::string_view path, const Abstract abstract) {

	if (path.size() > maxPathLen()) {
		throw RuntimeError{"UNIX address path too long"} ;
	}

	char *ptr = m_addr.sun_path;

	if (abstract) {
		*ptr = '\0';
		ptr++;
	}

	std::memcpy(ptr, path.data(), path.size());

	if (!abstract) {
		ptr[path.size()] = '\0';
	}

	m_path_len = path.size();
}

void UnixAddress::update(size_t new_length) {
	if (new_length < BASE_SIZE) {
		m_addr.sun_family = to_integral(family());
		m_path_len = 0;
		throw RuntimeError{"short address on update"} ;
	}

	new_length -= BASE_SIZE;

	// we don't count the null terminator (leading for abstract path, or
	// trailing for regular path).
	new_length--;

	m_path_len = new_length;
}

std::string UnixAddress::label() const {
	if (isUnnamed()) {
		return "<unnamed>";
	}

	std::string ret;

	if(isAbstract()) {
		ret.push_back('@');
		ret.append(getPath());
	} else {
		ret = std::string{getPath()};
	}

	while (ret.back() == '\0') {
		ret.pop_back();
	}

	for (auto it = ret.begin(); it != ret.end(); it++) {
		if (!std::isprint(*it)) {
			*it = '?';
		}
	}

	return ret;
}

} // end ns
