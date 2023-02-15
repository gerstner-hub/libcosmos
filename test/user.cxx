#include <iostream>

#include "cosmos/formatting.hxx"
#include "cosmos/GroupInfo.hxx"
#include "cosmos/PasswdInfo.hxx"
#include "cosmos/proc/Process.hxx"


int main() {
	const auto our_uid = cosmos::proc::get_real_user_id();

	cosmos::PasswdInfo pi(our_uid);

	if (!pi.valid()) {
		std::cerr << "failed to get passwd entry for " << our_uid << std::endl;
		return 1;
	}

	std::cout << "uid " << our_uid << " has username " << pi.name() << std::endl;

	cosmos::PasswdInfo pi2(pi.name());

	if (!pi.valid()) {
		std::cerr << "failed to get passwd entry for " << pi.name() << std::endl;
		return 1;
	}

	std::cout << pi.name() << " has uid " << pi.uid() << std::endl;

	if (our_uid != pi.uid()) {
		std::cerr << "mismatch in UIDs!" << std::endl;
		return 1;
	}

	const auto strange_user = "some_strange_user";
	cosmos::PasswdInfo pi3(strange_user);

	if (pi3.valid()) {
		std::cerr << "unexpectedly '" << strange_user << "' has a valid passwd entry" << std::endl;
		return 1;
	}

	std::cout << strange_user << " has no valid password entry" << std::endl;

	cosmos::GroupInfo gi{pi.gid()};

	if (!gi.valid()) {
		std::cerr << "failed to get group entry for " << pi.gid() << std::endl;
		return 1;
	}

	std::cout << "Group with ID " << gi.gid() << " is named: " << gi.name() << std::endl;

	if (gi.gid()  != pi.gid()) {
		std::cerr << "got group entry for different GID?!" << std::endl;
		return 1;
	}

	cosmos::GroupInfo gi2{gi.name()};

	if (!gi2.valid()) {
		std::cerr << "failed to get group entry for " << gi.name() << std::endl;
		return 1;
	}

	if (gi2.gid() != gi.gid()) {
		std::cerr << "group IDs for gi and gi2 don't match?!" << std::endl;
		return 1;
	}

	std::cout << "Members of " << gi2.name() << ": " << gi2.members() << std::endl;

	gi2 = cosmos::GroupInfo{"root"};

	if (!gi2.valid()) {
		std::cerr << "failed to get group info for 'root'" << std::endl;
		return 1;
	}

	std::cout << "Members of " << gi2.name() << ": " << gi2.members() << std::endl;

	gi2 = cosmos::GroupInfo{"strangegroup"};

	if (gi2.valid()) {
		std::cerr << "strangegroup unexpectedly exists" << std::endl;
		return 1;
	}

	return 0;
}
