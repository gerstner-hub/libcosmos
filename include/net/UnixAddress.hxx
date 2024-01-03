#pragma once

// Linux
#include <sys/un.h>

// C++
#include <string_view>

// cosmos
#include "cosmos/dso_export.h"
#include "cosmos/net/SocketAddress.hxx"
#include "cosmos/utils.hxx"

namespace cosmos {

/// Address type for local UNIX domain sockets.
/**
 * UNIX domain addresses come in three flavours on Linux:
 *
 * - unnamed: these have not been bound to any path. They are either not *yet*
 *   bound or they are anonymous as returned from
 *   cosmos::net::create_stream_socket_pair().
 * - path based: an actual file system path is used. The application has to
 *   manage the possibility that the path already exists, and has to remove
 *   the file system entry when the socket is no longer needed.
 * - abstract: this is a Linux extension. The path starts with a nul
 *   terminator '\0' (in strings often displayed using a leading '@' character).
 *   These sockets don't show up in the file system and are always reference
 *   counted i.e. if no process remains using it, the socket is cleaned up
 *   automatically. Further '\0' characters in the address have no special
 *   meaning, only the `size()` communicated to the kernel will determine the
 *   length of the address.
 *
 * There is some ambiguity regarding specifying the size of a `sockaddr_un` in
 * system calls. You can either specify the size of the actual number of bytes
 * *used* in the structure, or you can specify the full size of the structure.
 * For abstract socket addresses this can become problematic, since \0
 * characters don't terminate paths here, i.e. if you specify the full size of
 * `sockaddr_un` then you end up using a different address then when you
 * specify only the actual number of bytes used. For this reason the current
 * implementation only reports the actual number of bytes used for a path.
 * When communicating with applications that follow a different notion here,
 * it can happen that you won't be able to communicate with them.
 *
 * \warning File system based sockets are restricted by the permissions of the
 * directory they're placed in, as well by the socket's file mode. Only if a
 * process has write access to the socket, may it connect to it.
 * Sockets living in the abstract namespace, however, have no kernel side
 * permission checking. Any process in the system may connect to them. Thus
 * applications have to check UnixOptions::credentials() for access control.
 **/
class COSMOS_API UnixAddress :
		public SocketAddress {
public: // types

	/// Strong boolean type to indicate the use an abstract address.
	using Abstract = NamedBool<struct abstract_address_t, false>;

public: // functions

	/// Creates an empty address.
	UnixAddress() {
		m_addr.sun_family = to_integral(family());
		m_addr.sun_path[0] = '\0';
	}

	/// Creates an address from the given path which can also be abstract.
	explicit UnixAddress(const std::string_view path, const Abstract abstract = Abstract{false}) :
			UnixAddress{} {
		setPath(path, abstract);
	}

	SocketFamily family() const override {
		return SocketFamily::UNIX;
	}

	/// Returns the size of the structure considering the currently set path length only.
	size_t size() const override {
		return BASE_SIZE + m_path_len + 1;
	}

	/// Returns the maximum address size without taking into account the currently set path.
	size_t maxSize() const override {
		return sizeof(m_addr);
	}

	/// Maximum path length that can be stored in a UnixAddress structure.
	/**
	 * The returned value is not counting `\0' terminators. For both
	 * abstract and non-abstract addresses one byte for a null terminator
	 * (leading or trailing) is subtracted.
	 **/
	size_t maxPathLen() const {
		return sizeof(m_addr) - BASE_SIZE - 1;
	}

	/// Sets a new path for the address.
	/**
	 * Depending on the \c abstract setting this will be either a file
	 * system path, or an abstract label. No '\0' terminators should be
	 * embedded into \c path for the abstract case. The implementation
	 * will take care of this transparently.
	 **/
	void setPath(const std::string_view path, const Abstract abstract = Abstract{false});

	/// Returns the currently set path.
	/**
	 * This returns the string used in setPath(). If an abstract path is
	 * currently set then the leading '\0' terminator will not be
	 * contained in the string_view.
	 **/
	std::string_view getPath() const {
		if (isAbstract()) {
			return std::string_view{m_addr.sun_path + 1, m_path_len};
		} else {
			return std::string_view{m_addr.sun_path, m_path_len};
		}
	}

	/// Returns a human readable label for the contained path.
	/**
	 * This returns an implementation defined, human readable label
	 * describing the currently set path. In particular abstract paths are
	 * transformed in a way to indicate their abstract nature. Also
	 * unnamed addresses are specially marked in the returned string.
	 **/
	std::string label() const;

	/// Returns whether currently an abstract path is contained.
	bool isAbstract() const {
		return m_path_len > 1 && m_addr.sun_path[0] == '\0';
	}

	/// Returns whether this address is currently unnamed (empty).
	bool isUnnamed() const {
		return m_path_len == 0;
	}

	bool operator==(const UnixAddress &other) const {
		if (m_path_len != other.m_path_len)
			return false;
		if (m_addr.sun_family != other.m_addr.sun_family)
			return false;

		return std::memcmp(m_addr.sun_path, other.m_addr.sun_path, m_path_len + 1) == 0;
	}

	bool operator!=(const UnixAddress &other) const {
		return !(*this == other);
	}

protected: // functions

	void update(size_t new_length) override;

	sockaddr* basePtr() override {
		return reinterpret_cast<sockaddr*>(&m_addr);
	}

	const sockaddr* basePtr() const override {
		return reinterpret_cast<const sockaddr*>(&m_addr);
	}

protected: // data

	static constexpr size_t BASE_SIZE = offsetof(struct sockaddr_un, sun_path);
	sockaddr_un m_addr;
	size_t m_path_len = 0; ///< used bytes in m_addr.sun_path excluding terminator
};

} // end ns
