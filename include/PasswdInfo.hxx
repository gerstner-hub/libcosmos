#ifndef COSMOS_PASSWDINFO_HXX
#define COSMOS_PASSWDINFO_HXX

// Linux
#include <pwd.h>

// cosmos
#include "cosmos/ostypes.hxx"
#include "cosmos/InfoBase.hxx"

namespace cosmos {

/// Password Database Information for Users
/**
 * This type obtains and stores data for an individual user account ID as
 * found in the /etc/passwd database.
 **/
class COSMOS_API PasswdInfo :
		public InfoBase<struct passwd> {
public: // functions

	/// Obtains PasswdInfo for the given username \c name
	/**
	 * If an error occurs obtaining the entry then an ApiError exception
	 * is thrown.
	 *
	 * If simply no matching entry exists then *no* exception is thrown
	 * but isValid() return false and all members are empty.
	 **/
	explicit PasswdInfo(const std::string_view name);

	/// Obtains PasswdInfo for the given numerical user id \c uid
	/**
	 * \see PasswdInfo(const std::string_view)
	 **/
	explicit PasswdInfo(const UserID uid);

	const std::string_view getName() const { return getSV(m_info.pw_name); }

	/// Returns the optional encrypted password
	const std::string_view getPasswd() const { return getSV(m_info.pw_passwd); }

	UserID getUID() const { return UserID{m_info.pw_uid}; }

	/// The user's main group ID
	GroupID getGID() const { return GroupID{m_info.pw_gid}; }

	/// Returns the comment field which is used for different things like a full user name
	const std::string_view getGecos() const { return getSV(m_info.pw_gecos); }

	/// Path to the user's home directory
	const std::string_view getHomeDir() const { return getSV(m_info.pw_dir); }

	/// Optional command interpreter for the user
	const std::string_view getShell() const { return getSV(m_info.pw_shell); }
};

} // end ns

#endif // inc. guard
