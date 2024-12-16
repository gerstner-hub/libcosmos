// C++
#include <iostream>

// cosmos
#include <cosmos/formatting.hxx>
#include <cosmos/memory.hxx>
#include <cosmos/proc/process.hxx>
#include <cosmos/proc/SignalFD.hxx>
#include <cosmos/proc/SigSet.hxx>
#include <cosmos/thread/Condition.hxx>
#include <cosmos/thread/PosixThread.hxx>
#include <cosmos/thread/thread.hxx>

// Test
#include "TestBase.hxx"

using namespace cosmos;

class SignalTest :
		public cosmos::TestBase {

	void runTests() override {
		testSets();
		testSigmask();
		testIgnore();
		testPauseSuspend();
		testSigWait();
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
		const auto &orig_mask = cosmos::signal::get_sigmask();
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
		cosmos::signal::set_sigmask(orig_mask);
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

	static void handle_signal(int sig) {
		(void)sig;
	}

	void testPauseSuspend() {
		START_TEST("pause/suspend and wake thread via thread-directed signal");

		// TODO: use libcosmos API for this in the future (not yet available)
		struct sigaction action;
		cosmos::zero_object(action);
		action.sa_handler = &SignalTest::handle_signal;
		const auto res = ::sigaction(SIGUSR1, &action, nullptr);

		bool pause_over = false;
		cosmos::ConditionMutex pause_cond;

		RUN_STEP("sigaction-succeeds", res == 0);

		const auto tid_to_signal = cosmos::thread::get_tid();

		cosmos::PosixThread thread{[tid_to_signal, &pause_over, &pause_cond]() {

			cosmos::MutexGuard g{pause_cond.mutex()};

			while (!pause_over) {
				cosmos::signal::send(cosmos::proc::get_own_pid(), tid_to_signal, cosmos::signal::USR1);
				pause_cond.waitTimed(cosmos::MonotonicTime{std::chrono::milliseconds{50}});
			}

			// send a second signal to test suspend()
			cosmos::signal::send(cosmos::proc::get_own_pid(), tid_to_signal, cosmos::signal::USR1);
		}};

		cosmos::signal::pause();

		RUN_STEP("pause-returns-due-to-USR1", true);

		// now block the signal to test suspend()
		cosmos::SigSet ss;
		ss.set(cosmos::signal::USR1);
		cosmos::signal::block(ss);

		{
			cosmos::MutexGuard g{pause_cond.mutex()};
			pause_over = true;
		}

		pause_cond.signal();

		ss = cosmos::signal::get_sigmask();
		ss.del(cosmos::signal::USR1);

		// we should be able to receive the signal due to the
		// temporarily changed signal mask in suspend()
		cosmos::signal::suspend(ss);

		RUN_STEP("suspend-returns-due-to-USR1", true);

		thread.join();
	}


	void testSigWait() {
		START_TEST("testing send (sigqueue) and wait (sigwait)");

		cosmos::SigSet set{cosmos::signal::USR1};

		cosmos::signal::block(set);

		cosmos::PosixThread thread{[]() {
			cosmos::signal::send(cosmos::proc::get_own_pid(), cosmos::signal::USR1, 0);
		}};

		const auto sig = cosmos::signal::wait(set);

		RUN_STEP("sigwait-returns-USR1", sig == cosmos::signal::USR1);

		thread.join();
	}
};

int main(const int argc, const char **argv) {
	SignalTest test;
	return test.run(argc, argv);
}
