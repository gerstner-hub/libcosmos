#pragma once

// cosmos
#include "cosmos/SysString.hxx"
#include "cosmos/dso_export.h"
#include "cosmos/fs/FileDescriptor.hxx"
#include "cosmos/net/types.hxx"

namespace cosmos {

/// Base class for Socket option helpers for different OptLevels.
/**
 * This base class offers some common infrastructure for dealing with socket
 * options. Implementations of this class need to specify the OptLevel they
 * cover as template argument to this class.
 **/
template <OptLevel LEVEL>
class COSMOS_API SockOptBase {
protected: // functions

	/// Perform socket options on the given file descriptor.
	explicit SockOptBase(FileDescriptor fd) :
		m_sock{fd} {}

	/// Return a boolean style option.
	bool getBoolOption(const OptName name) const;
	/// Set a boolean style option.
	void setBoolOption(const OptName name, const bool val);

	/// Return an integer option.
	int getIntOption(const OptName name) const;
	/// Set an integer option.
	void setIntOption(const OptName name, const int val);

	/// Return a null terminated string option.
	std::string getStringOption(const OptName name, size_t max_len) const;
	/// Set a null terminated string option.
	void setStringOption(const OptName name, const SysString str);

	/// Returns the labeled IPSEC or NetLabel of the peer.
	/**
	 * This only works if IPSEC or NetLabel is configured on both the
	 * sending and receiving hosts. This option is supported for TCP and
	 * SCTP sockets on IP level or for UNIX domain sockets.
	 *
	 * The returned string will have the proper length and null
	 * termination. The encoding of the returned string is unspecified
	 * though. In particular it is not guaranteed to be ASCII or UTF-8.
	 **/
	std::string getPeerSec() const;

	/* disallow copy/assignment for option helpers */

	SockOptBase(const SockOptBase &) = delete;
	SockOptBase& operator=(const SockOptBase&) = delete;


protected: // data

	static constexpr OptLevel M_LEVEL = LEVEL; ///< The option level to operate on.
	FileDescriptor m_sock; ///< The socket file descriptor to operate on.
};

extern template class COSMOS_API SockOptBase<OptLevel::SOCKET>;
extern template class COSMOS_API SockOptBase<OptLevel::IP>;
extern template class COSMOS_API SockOptBase<OptLevel::IPV6>;
extern template class COSMOS_API SockOptBase<OptLevel::TCP>;
extern template class COSMOS_API SockOptBase<OptLevel::UDP>;

}; // end ns
