#ifndef COSMOS_GROUPINFO_HXX
#define COSMOS_GROUPINFO_HXX

// Linux
#include "grp.h"

// C++
#include <string_view>

// cosmos
#include "cosmos/InfoBase.hxx"
#include "cosmos/ostypes.hxx"
#include "cosmos/types.hxx"

namespace cosmos {

/// Group Database Information
/**
 * This type obtains and stores data for an individual group account ID as
 * found in the /etc/group database.
 **/
class COSMOS_API GroupInfo :
		public InfoBase<struct group> {
public: // functions

	/// Obtains GroupInfo for the given group name \c name
	explicit GroupInfo(const std::string_view name);

	/// Obtains GroupInfo for the given numerical group id \c gid
	explicit GroupInfo(const GroupID gid);

	/// The groups numerical ID
	GroupID getGID() const { return GroupID{m_info.gr_gid}; }

	/// Returns the name associated with the group
	const std::string_view getName() const { return getSV(m_info.gr_name); }

	/// Returns the optional encrypted group password
	const std::string_view getPasswd() const { return getSV(m_info.gr_passwd); }

	/// Returns a vector containing the name of users that are members of this group
	const StringViewVector getMembers() const;
};

} // end ns

#endif // inc. guard
