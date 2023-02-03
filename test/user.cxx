#include <iostream>

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

	return 0;
}
