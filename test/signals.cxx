// C++
#include <atomic>
#include <iostream>

// cosmos
#include <cosmos/formatting.hxx>
#include <cosmos/memory.hxx>
#include <cosmos/proc/process.hxx>
#include <cosmos/proc/SigAction.hxx>
#include <cosmos/proc/SigInfo.hxx>
#include <cosmos/proc/SignalFD.hxx>
#include <cosmos/proc/SigSet.hxx>
#include <cosmos/thread/Condition.hxx>
#include <cosmos/thread/PosixThread.hxx>
#include <cosmos/thread/thread.hxx>
#include <cosmos/time/Clock.hxx>
#include <cosmos/time/time.hxx>
#include <cosmos/utils.hxx>

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
		testSigWaitInfo();
		testSigAction();
		testAsyncSignals();
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

	static void handle_signal(const cosmos::Signal) {
	}

	void testPauseSuspend() {
		START_TEST("pause/suspend and wake thread via thread-directed signal");

		cosmos::SigAction action;
		action.setHandler(&handle_signal);
		cosmos::signal::set_action(cosmos::signal::USR1, action, &action);

		bool pause_over = false;
		cosmos::ConditionMutex pause_cond;

		const auto tid_to_signal = cosmos::thread::get_tid();

		cosmos::PosixThread thread{[tid_to_signal, &pause_over, &pause_cond]() {

			cosmos::MutexGuard g{pause_cond.mutex()};

			while (!pause_over) {
				cosmos::signal::send(cosmos::proc::get_own_pid(), tid_to_signal, cosmos::signal::USR1);
				pause_cond.waitTimed(cosmos::MonotonicClock{}.now() + cosmos::MonotonicTime{std::chrono::milliseconds{50}});
			}

			// send a second signal to test suspend()
			cosmos::signal::send(cosmos::proc::get_own_pid(), tid_to_signal, cosmos::signal::USR1);
		}};

		cosmos::signal::pause();

		RUN_STEP("pause-returns-due-to-USR1", true);

		// now block the signal to test suspend()
		cosmos::SigSet ss{cosmos::signal::USR1};
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

		// restore the original action
		cosmos::signal::set_action(cosmos::signal::USR1, action);
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

	void testSigWaitInfo() {
		START_TEST("testing send (sigqueue) and wait_info (sigwaitinfo)");

		cosmos::SigSet set{cosmos::signal::USR1};

		cosmos::signal::block(set);

		cosmos::PosixThread thread{[]() {
			cosmos::signal::send(cosmos::proc::get_own_pid(), cosmos::signal::USR1, 0x1234);
		}};

		cosmos::SigInfo info{cosmos::no_init};

		cosmos::signal::wait_info(set, info);

		thread.join();

		RUN_STEP("sigwaitinfo-sig-matches", info.sigNr() == cosmos::signal::USR1);
		RUN_STEP("sigwaitinfo-source-matches", info.source() == SigInfo::Source::QUEUE);
		RUN_STEP("sigwaitinfo-untrusted-source", !info.isTrustedSource());
		RUN_STEP("sigwaitinfo-no-fault-sig", !info.isFaultSignal());
		RUN_STEP("sigwaitinfo-no-user-sig-data", !info.userSigData());
		RUN_STEP("sigwaitinfo-no-msg-queue-data", !info.msgQueueData());
		RUN_STEP("sigwaitinfo-no-timer-data", !info.timerData());
		RUN_STEP("sigwaitinfo-no-sys-data", !info.sysData());
		RUN_STEP("sigwaitinfo-no-child-data", !info.childData());
		RUN_STEP("sigwaitinfo-no-poll-data", !info.pollData());
		RUN_STEP("sigwaitinfo-no-ill-data", !info.illData());
		RUN_STEP("sigwaitinfo-no-fpe-data", !info.fpeData());
		RUN_STEP("sigwaitinfo-no-segv-data", !info.segfaultData());
		RUN_STEP("sigwaitinfo-no-bus-data", !info.busData());

		auto data = info.queueSigData();

		RUN_STEP("sigwaitinfo-has-queue-sig-data", data != std::nullopt);
		RUN_STEP("sigwaitinfo-sender-pid-is-us", data->sender.pid == cosmos::proc::get_own_pid());
		RUN_STEP("sigwaitinfo-sender-uid-is-us", data->sender.uid == cosmos::proc::get_real_user_id());
		RUN_STEP("sigwaitinfo-data-matches", data->data.asInt() == 0x1234);


		auto res = cosmos::signal::timed_wait(set, info, cosmos::IntervalTime{std::chrono::milliseconds{50}});

		RUN_STEP("sigtimedwait-returns-nothing", res == cosmos::signal::WaitRes::NO_RESULT);

		res = cosmos::signal::poll_info(set, info);

		RUN_STEP("poll-info-returns-nothing", res == cosmos::signal::WaitRes::NO_RESULT);
	}

	static std::atomic_bool info_handler_running;
	static std::atomic_bool simple_handler_running;
	static std::atomic_int info_handler_int;
	static std::atomic_int async_signal_seen;

	static void info_handler(const cosmos::SigInfo &info) {
		info_handler_int.store(info.queueSigData()->data.asInt());
		async_signal_seen.store(cosmos::to_integral(info.sigNr().raw()));
		info_handler_running.store(true);
	}

	static void simple_handler(const cosmos::Signal sig) {
		async_signal_seen.store(cosmos::to_integral(sig.raw()));
		simple_handler_running.store(true);
	}

	static void plain_handler(int) {
	}

	void testSigAction() {
		START_TEST("basic sigaction test");

		using cosmos::SigAction;
		constexpr auto TESTSIG = cosmos::signal::POWER;

		SigAction act;
		act.setFlags(SigAction::Flags{SigAction::Flag::RESET_HANDLER});
		act.mask().set(cosmos::signal::ILL);
		act.setHandler(&info_handler);

		SigAction orig;

		// NOTE: using SIGSEGV or similar fault signals is problematic
		// when running with ASAN sanitizer. The glue code obviously
		// installed their own signal handlers, so non-default values
		// will be observed in these cases.
		cosmos::signal::set_action(TESTSIG, act, &orig);

		if (orig.getSimpleHandler() == SigAction::IGNORE) {
			std::cerr << "IGNORE\n";
		} else if (orig.getSimpleHandler() == SigAction::UNKNOWN) {
			std::cerr << "UNKNOWN\n";
		}

		RUN_STEP("orig-action-is-default", orig.getSimpleHandler() == SigAction::DEFAULT);

		SigAction act2;
		act2.clear();

		cosmos::signal::get_action(TESTSIG, act2);

		for (auto _: cosmos::Twice{}) {
			// comparing signal sets is non-trivial, so just check whether SIGILL is present in both.
			RUN_STEP("new-act-mask-matches", act2.mask().isSet(cosmos::signal::ILL) == act.mask().isSet(cosmos::signal::ILL));
			// we need to mask out the SA_RESTORER flag which is implicitly set by libc
			RUN_STEP("new-act-flags-match", act2.getFlags().reset(SigAction::Flag::RESTORER) == act.getFlags());

			RUN_STEP("new-act-handler-matches", act2.getInfoHandler() == info_handler);

			// restore the original signal setting, check again
			// whether the old data is correct.
			cosmos::signal::set_action(TESTSIG, orig, &act2);
		}

		/*
		 * now test setting a regular C signal handler, overriding and
		 * restoring that via libcosmos without losing information.
		 */

		{
			struct sigaction sa;
			cosmos::zero_object(sa);
			sa.sa_handler = plain_handler;
			sigaddset(&sa.sa_mask, SIGBUS);
			sa.sa_flags = SA_NODEFER;
			const auto res = ::sigaction(SIGPWR, &sa, nullptr);
			RUN_STEP("sigaction-succeeds", res == 0);
		}

		cosmos::signal::set_action(TESTSIG, act, &act2);

		RUN_STEP("legacy-act-mask-matches", act2.mask().isSet(cosmos::signal::BUS));
		RUN_STEP("legacy-flags-match", act2.getFlags().reset(SigAction::Flag::RESTORER) == SigAction::Flags{SigAction::Flag::NO_DEFER});
		RUN_STEP("legacy-handler-matches", act2.getSimpleHandler() == SigAction::UNKNOWN);

		// restore default to avoid side effects for other tests
		cosmos::signal::set_action(TESTSIG, orig);
	}

	void testAsyncSignals() {
		START_TEST("async signals test");

		constexpr auto TESTSIG = cosmos::signal::USR1;

		// the signal we want to use for async signal handling
		cosmos::signal::unblock(cosmos::SigSet{TESTSIG});

		cosmos::SigAction old{cosmos::no_init};
		cosmos::SigAction action;
		// first test the simple handler
		action.setHandler(&simple_handler);

		cosmos::signal::set_action(TESTSIG, action, &old);
		cosmos::signal::raise(TESTSIG);

		while (!simple_handler_running.load()) {
			cosmos::time::sleep(std::chrono::milliseconds{50});
		}

		RUN_STEP("simple-handler-signal-matches", async_signal_seen.load() == SIGUSR1);

		async_signal_seen.store(0);

		action.setHandler(&info_handler);
		cosmos::signal::set_action(TESTSIG, action);

		cosmos::signal::send(cosmos::proc::get_own_pid(), TESTSIG, 0x4321);

		while (!info_handler_running.load()) {
			cosmos::time::sleep(std::chrono::milliseconds{50});
		}

		RUN_STEP("info-handler-signal-matches", async_signal_seen.load() == SIGUSR1);
		RUN_STEP("info-handler-data-matches", info_handler_int.load() == 0x4321);

		// restore original handler
		cosmos::signal::set_action(TESTSIG, old);
	}
};

std::atomic_bool SignalTest::info_handler_running;
std::atomic_bool SignalTest::simple_handler_running;
std::atomic_int SignalTest::info_handler_int;
std::atomic_int SignalTest::async_signal_seen;

int main(const int argc, const char **argv) {
	SignalTest test;
	return test.run(argc, argv);
}
