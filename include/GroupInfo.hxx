#pragma once

// Linux
#include <grp.h>

// C++
#include <string_view>

// cosmos
#include <cosmos/InfoBase.hxx>
#include <cosmos/string.hxx>
#include <cosmos/types.hxx>
#include <cosmos/utils.hxx>

namespace cosmos {

/// Group Database Information.
/**
 * This type obtains and stores data for an individual group account ID as
 * found in the /etc/group database.
 **/
class COSMOS_API GroupInfo :
		public InfoBase<struct group> {
public: // functions

	/// Obtains GroupInfo for the given group name `name`.
	explicit GroupInfo(const SysString name);

	/// Obtains GroupInfo for the given numerical group id `gid`.
	explicit GroupInfo(const GroupID gid);

	/// The groups numerical ID.
	GroupID gid() const { return GroupID{m_info.gr_gid}; }

	/// Returns the name associated with the group.
	SysString name() const { return SysString{m_info.gr_name}; }

	/// Returns the optional encrypted group password.
	SysString passwd() const { return SysString{m_info.gr_passwd}; }

	/// Returns a vector containing the name of users that are members of this group.
	SysStringVector members() const;
};

} // end ns
