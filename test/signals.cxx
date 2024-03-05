// C++
#include <iostream>

// cosmos
#include <cosmos/formatting.hxx>
#include <cosmos/proc/SignalFD.hxx>
#include <cosmos/proc/SigSet.hxx>
#include <cosmos/proc/process.hxx>

// Test
#include "TestBase.hxx"

using namespace cosmos;

class SignalTest :
		public cosmos::TestBase {

	void runTests() override {
		testSets();
		testSigmask();
		testIgnore();
	}

	void testSets() {
		START_TEST("SigSet");
		cosmos::SigSet empty;
		cosmos::SigSet full{cosmos::SigSet::filled};
		cosmos::SigSet some;

		for (auto sig: {signal::INTERRUPT, signal::TERMINATE, signal::KILL, signal::IO_EVENT, signal::BUS}) {
			RUN_STEP("not-in-empty", !empty.isSet(sig));
			RUN_STEP("is-in-full", full.isSet(sig));
			some.set(sig);
			full.del(sig);
			RUN_STEP("in-set-after-add", some.isSet(sig));
			RUN_STEP("not-there-after-del", !full.isSet(sig));
		}
	}

	void testSigmask() {
		START_TEST("Sigmask");
		cosmos::SigSet full{cosmos::SigSet::filled};
		cosmos::SigSet old;
		cosmos::signal::block(full, &old);

		std::cout << "SIGINT was " << (old.isSet(signal::INTERRUPT) ? "blocked" : "not blocked") << std::endl;
		cosmos::signal::unblock(full, &old);
		std::cout << "SIGINT was " << (old.isSet(signal::INTERRUPT) ? "blocked" : "not blocked") << std::endl;

		const auto sigint = signal::INTERRUPT;

		cosmos::SigSet set;
		set.set(sigint);
		cosmos::signal::set_sigmask(set, &old);

		RUN_STEP("old-mask-correct", !old.isSet(sigint));
		set = cosmos::signal::get_sigmask();
		RUN_STEP("get-mask-correct", set.isSet(sigint));

		cosmos::SignalFD sfd{sigint};

		RUN_STEP("signalfd-validity", sfd.valid());

		cosmos::SignalFD::SigInfo info;

		cosmos::signal::raise(sigint);
		sfd.readEvent(info);

		RUN_STEP("received-sig-correct", info.signal() == sigint);
		std::cout << "received " << info.signal() << " from " << info.senderPID() << std::endl;
	}

	void testIgnore() {
		START_TEST("ignore signal");

		SigSet set;
		set.set(signal::TERMINATE);
		signal::unblock(set);
		signal::ignore(signal::TERMINATE);
		signal::raise(signal::TERMINATE);

		RUN_STEP("did-not-terminate", true);
	}
};

int main(const int argc, const char **argv) {
	SignalTest test;
	return test.run(argc, argv);
}
