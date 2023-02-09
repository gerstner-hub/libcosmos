#include <iostream>

#include "cosmos/formatting.hxx"
#include "cosmos/GroupInfo.hxx"
#include "cosmos/PasswdInfo.hxx"
#include "cosmos/proc/Process.hxx"


int main() {
	const auto our_uid = cosmos::proc::getRealUserID();

	cosmos::PasswdInfo pi(our_uid);

	if (!pi.isValid()) {
		std::cerr << "failed to get passwd entry for " << our_uid << std::endl;
		return 1;
	}

	std::cout << "uid " << our_uid << " has username " << pi.getName() << std::endl;

	cosmos::PasswdInfo pi2(pi.getName());

	if (!pi.isValid()) {
		std::cerr << "failed to get passwd entry for " << pi.getName() << std::endl;
		return 1;
	}

	std::cout << pi.getName() << " has uid " << pi.getUID() << std::endl;

	if (our_uid != pi.getUID()) {
		std::cerr << "mismatch in UIDs!" << std::endl;
		return 1;
	}

	const auto strange_user = "some_strange_user";
	cosmos::PasswdInfo pi3(strange_user);

	if (pi3.isValid()) {
		std::cerr << "unexpectedly '" << strange_user << "' has a valid passwd entry" << std::endl;
		return 1;
	}

	std::cout << strange_user << " has no valid password entry" << std::endl;

	cosmos::GroupInfo gi{pi.getGID()};

	if (!gi.isValid()) {
		std::cerr << "failed to get group entry for " << pi.getGID() << std::endl;
		return 1;
	}

	std::cout << "Group with ID " << gi.getGID() << " is named: " << gi.getName() << std::endl;

	if (gi.getGID()  != pi.getGID()) {
		std::cerr << "got group entry for different GID?!" << std::endl;
		return 1;
	}

	cosmos::GroupInfo gi2{gi.getName()};

	if (!gi2.isValid()) {
		std::cerr << "failed to get group entry for " << gi.getName() << std::endl;
		return 1;
	}

	if (gi2.getGID() != gi.getGID()) {
		std::cerr << "group IDs for gi and gi2 don't match?!" << std::endl;
		return 1;
	}

	std::cout << "Members of " << gi2.getName() << ": " << gi2.getMembers() << std::endl;

	gi2 = cosmos::GroupInfo{"root"};

	if (!gi2.isValid()) {
		std::cerr << "failed to get group info for 'root'" << std::endl;
		return 1;
	}

	std::cout << "Members of " << gi2.getName() << ": " << gi2.getMembers() << std::endl;

	gi2 = cosmos::GroupInfo{"strangegroup"};

	if (gi2.isValid()) {
		std::cerr << "strangegroup unexpectedly exists" << std::endl;
		return 1;
	}

	return 0;
}
