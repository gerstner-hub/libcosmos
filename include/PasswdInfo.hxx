#ifndef COSMOS_PASSWDINFO_HXX
#define COSMOS_PASSWDINFO_HXX

// Linux
#include <pwd.h>

// C++
#include <string_view>
#include <vector>

// cosmos
#include "cosmos/ostypes.hxx"

namespace cosmos {

/// Password Database Information for Users
/**
 * This type obtains and stores data for an individual user account ID as
 * found in the /etc/passwd database.
 **/
class COSMOS_API PasswdInfo {
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

	const std::string_view getName() const { return getSV(m_passwd.pw_name); }

	/// Returns the optional encrypted password
	const std::string_view getPasswd() const { return getSV(m_passwd.pw_passwd); }

	UserID getUID() const { return m_passwd.pw_uid; }

	/// The user's main group ID
	GroupID getGID() const { return m_passwd.pw_gid; }

	/// Returns the comment field which is used for different things like a full user name
	const std::string_view getGecos() const { return getSV(m_passwd.pw_gecos); }

	/// Path to the user's home directory
	const std::string_view getHomeDir() const { return getSV(m_passwd.pw_dir); }

	/// Optional command interpreter for the user
	const std::string_view getShell() const { return getSV(m_passwd.pw_shell); }

	const struct passwd* getRaw() const { return &m_passwd; }
	struct passwd* getRaw() { return &m_passwd; }

	/// Returns whether data is present in the object
	/**
	 * if no corresponding entry was found during construction time then
	 * this returns \c false.
	 **/
	bool isValid() const { return m_valid; }

	/// Zero out all data
	void invalidate();

protected: // functions

	const std::string_view getSV(const char *ptr) const {
		// string_view can't take nullptr, thus use this
		return ptr ? std::string_view(ptr) : std::string_view{};
	}

protected: // data

	bool m_valid = false;
	struct passwd m_passwd;
	/// Extra space for storing the strings of struct passwd
	std::vector<char> m_buf;
};

} // end ns

#endif // inc. guard
