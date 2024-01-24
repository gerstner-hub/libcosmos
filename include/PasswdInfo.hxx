#pragma once

// Linux
#include <pwd.h>

// cosmos
#include "cosmos/InfoBase.hxx"
#include "cosmos/types.hxx"
#include "cosmos/SysString.hxx"

namespace cosmos {

/// Password Database Information for Users
/**
 * This type obtains and stores data for an individual user account ID as
 * found in the /etc/passwd database.
 **/
class COSMOS_API PasswdInfo :
		public InfoBase<struct passwd> {
public: // functions

	/// Obtains PasswdInfo for the given username `name`.
	/**
	 * If an error occurs obtaining the entry then an ApiError exception
	 * is thrown.
	 *
	 * If simply no matching entry exists then *no* exception is thrown
	 * but valid() return false and all members are empty.
	 **/
	explicit PasswdInfo(const SysString name);

	/// Obtains PasswdInfo for the given numerical user id `uid`.
	/**
	 * \see PasswdInfo(const SysString)
	 **/
	explicit PasswdInfo(const UserID uid);

	SysString name() const { return m_info.pw_name; }

	/// Returns the optional encrypted password.
	SysString passwd() const { return m_info.pw_passwd; }

	UserID uid() const { return UserID{m_info.pw_uid}; }

	/// The user's main group ID.
	GroupID gid() const { return GroupID{m_info.pw_gid}; }

	/// Returns the comment field which is used for different things like a full user name.
	SysString gecos() const { return m_info.pw_gecos; }

	/// Path to the user's home directory.
	SysString homeDir() const { return m_info.pw_dir; }

	/// Optional command interpreter for the user.
	SysString shell() const { return m_info.pw_shell; }
};

} // end ns
