// C++
#include <iostream>

// cosmos
#include "cosmos/formatting.hxx"
#include "cosmos/GroupInfo.hxx"
#include "cosmos/PasswdInfo.hxx"
#include "cosmos/proc/Process.hxx"

// Test
#include "TestBase.hxx"

class UserTest :
		public cosmos::TestBase {

	void runTests() override {
		testUser();
		testGroup();
	}

	void testUser() {
		START_TEST("user");
		const auto our_uid = cosmos::proc::get_real_user_id();
		cosmos::PasswdInfo pi{our_uid};

		RUN_STEP("own-uid-passwd-valid", pi.valid());

		std::cout << "uid " << our_uid << " has username " << pi.name() << std::endl;

		cosmos::PasswdInfo pi2(pi.name());

		RUN_STEP("own-username-passwd-valid", pi.valid());

		std::cout << pi.name() << " has uid " << pi.uid() << std::endl;

		RUN_STEP("uids-match", our_uid == pi.uid());

		const auto strange_user = "some_strange_user";
		cosmos::PasswdInfo pi3(strange_user);

		RUN_STEP("strange-user-unknown", !pi3.valid());

		std::cout << strange_user << " has no valid password entry" << std::endl;
	}

	void testGroup() {
		const auto our_gid = cosmos::proc::get_real_group_id();
		cosmos::GroupInfo gi{our_gid};

		RUN_STEP("own-gid-group-valid", gi.valid());

		std::cout << "Group with ID " << gi.gid() << " is named: " << gi.name() << std::endl;

		RUN_STEP("gids-match", gi.gid() == our_gid);

		cosmos::GroupInfo gi2{gi.name()};

		RUN_STEP("own-groupname-group-valid", gi2.valid());

		RUN_STEP("gid-infos-match", gi2.gid() == gi.gid());

		std::cout << "Members of " << gi2.name() << ": " << gi2.members() << std::endl;

		gi2 = cosmos::GroupInfo{"root"};

		RUN_STEP("root-gid-valid", gi2.valid());

		std::cout << "Members of " << gi2.name() << ": " << gi2.members() << std::endl;

		gi2 = cosmos::GroupInfo{"strangegroup"};

		RUN_STEP("strange-group-unknown", !gi2.valid());
	}
};

int main(const int argc, const char **argv) {
	UserTest test;
	return test.run(argc, argv);
}
