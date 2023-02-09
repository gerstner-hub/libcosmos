#ifndef COSMOS_GROUPINFO_HXX
#define COSMOS_GROUPINFO_HXX

// Linux
#include "grp.h"

// C++
#include <string_view>
#include <vector>

// cosmos
#include "cosmos/ostypes.hxx"
#include "cosmos/types.hxx"

namespace cosmos {

/// Group Database Information
/**
 * This type obtains and stores data for an individual group account ID as
 * found in the /etc/group database.
 **/
class COSMOS_API GroupInfo {
public: // functions

	/// Obtains GroupInfo for the given group name \c name
	explicit GroupInfo(const std::string_view name);

	/// Obtains GroupInfo for the given numerical group id \c gid
	explicit GroupInfo(const GroupID gid);

	/// The groups numerical ID
	GroupID getGID() const { return GroupID{m_group.gr_gid}; }

	/// Returns the name associated with the group
	const std::string_view getName() const { return getSV(m_group.gr_name); }

	/// Returns the optional encrypted group password
	const std::string_view getPasswd() const { return getSV(m_group.gr_passwd); }

	/// Returns a vector containing the name of users that are members of this group
	const StringViewVector getMembers() const;

	const struct group* getRaw() const { return &m_group; }
	struct group* getRaw() { return &m_group; }

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
		return ptr ? std::string_view{ptr} : std::string_view{};
	}

protected: // data

	bool m_valid = false;
	struct group m_group;
	/// Extra space for storing the strings of struct group
	std::vector<char> m_buf;
};

} // end ns

#endif // inc. guard
