// Linux
#include <unistd.h>

// C++
#include <iostream>

// cosmos
#include <cosmos/private/cosmos.hxx>
#include <cosmos/error/ApiError.hxx>
#include <cosmos/error/UsageError.hxx>
#include <cosmos/error/RuntimeError.hxx>
#include <cosmos/proc/SignalFD.hxx>
#include <cosmos/utils.hxx>

namespace cosmos {

SignalFD::~SignalFD() {
	try {
		close();
	} catch (const std::exception &ex) {
		noncritical_error("Failed to close signal fd", ex);
	}
}

void SignalFD::create(const SigSet &mask) {
	close();
	auto fd = ::signalfd(-1, mask.raw(), SFD_CLOEXEC);
	if (fd == -1) {
		cosmos_throw (ApiError("signalfd()"));
	}

	m_fd.setFD(FileNum{fd});
}

void SignalFD::adjustMask(const SigSet &mask) {
	if (!valid()) {
		cosmos_throw (UsageError("no signal fd currently open"));
	}

	// NOTE: it's unclear from the man page whether flags are used when
	// modifying an existing signal fd. Let's pass on zero, hoping that no
	// flags will be taken away again through this.
	auto fd = ::signalfd(to_integral(m_fd.raw()), mask.raw(), 0);
	if (fd == -1) {
		cosmos_throw (ApiError("signalfd()"));
	}
}

void SignalFD::readEvent(Info &info) {
	auto raw = info.raw();
	constexpr auto RAW_SIZE = sizeof(*raw);

	auto res = ::read(to_integral(m_fd.raw()), raw, RAW_SIZE);

	if (res < 0) {
		cosmos_throw (ApiError("read(sigfd)"));
	}
	else if (static_cast<size_t>(res) < RAW_SIZE) {
		cosmos_throw (RuntimeError("short read from signal fd"));
	}
}

using Info = SignalFD::Info;

Info::Source Info::source() const {
	const std::initializer_list<Signal> SPECIAL_SIGS{
		signal::FPE, signal::BUS, signal::ILL, signal::SEGV,
		signal::BUS, signal::TRAP, signal::CHILD, signal::POLL,
		signal::BAD_SYS};

	// the lower level system call `rt_sigqueueinfo` allows userspace to
	// send arbitrary codes < 0, thus the signal number alone doesn't mean
	// the kernel was the source.
	if (isTrustedSource() && in_list(this->sigNr(), SPECIAL_SIGS)) {
		return Source::KERNEL;
	}

	return Source{m_raw.ssi_code};
}

std::optional<const Info::UserSigData> Info::userSigData() const {
	if (source() == SigInfo::Source::USER) {
		return UserSigData{this->procCtx()};
	}

	return std::nullopt;
}

std::optional<const Info::QueueSigData> Info::queueSigData() const {
	if (source() == SigInfo::Source::QUEUE) {
		// the signalfd_siginfo does not involve a union, but contains
		// always both the 32-bit int value and the 64-bit pointer
		// value. The pointer always contains the lower 32-bit of the
		// int, if an int was sent. Thus we place the pointer into the
		// union and constructor SigInfo::CustomData from it.
		union sigval val;
		val.sival_ptr = reinterpret_cast<void*>(m_raw.ssi_ptr);
		return QueueSigData{this->procCtx(), SigInfo::CustomData{val}};
	}

	return std::nullopt;
}

std::optional<const Info::MsgQueueData> Info::msgQueueData() const {
	if (source() == SigInfo::Source::MESGQ) {
		union sigval val;
		val.sival_ptr = reinterpret_cast<void*>(m_raw.ssi_ptr);
		return MsgQueueData{this->procCtx(), SigInfo::CustomData{val}};
	}

	return std::nullopt;
}

std::optional<const Info::TimerData> Info::timerData() const {
	if (source() == SigInfo::Source::TIMER) {
		return TimerData{
			// is unsigned here, so cast it to signed
			TimerData::TimerID{static_cast<int>(m_raw.ssi_tid)},
			// same here
			static_cast<int>(m_raw.ssi_overrun)
		};
	}

	return std::nullopt;
}

std::optional<const Info::SysData> Info::sysData() const {
	if (sigNr() == signal::BAD_SYS) {
		return SysData{
			SysData::Reason{m_raw.ssi_code},
			// is an uint64_t here, so cast it to void*
			reinterpret_cast<void*>(m_raw.ssi_call_addr),
			m_raw.ssi_syscall,
			ptrace::Arch{m_raw.ssi_arch},
			this->error()
		};
	}

	return std::nullopt;
}

std::optional<const Info::ChildData> Info::childData() const {
	if (sigNr() == signal::CHILD) {
		using Event = ChildData::Event;
		const auto event = Event{m_raw.ssi_code};

		return ChildData{{
			event,
			this->procCtx(),
			event == Event::EXITED ? std::make_optional(ExitStatus{m_raw.ssi_status}) : std::nullopt,
			event == Event::EXITED ? std::nullopt : std::make_optional(Signal{SignalNr{m_raw.ssi_status}})},
			// is an uint64_t here, so cast it to clock_t for reuse
			std::make_optional(ClockTicks{static_cast<clock_t>(m_raw.ssi_utime)}),
			std::make_optional(ClockTicks{static_cast<clock_t>(m_raw.ssi_stime)})
		};
	}

	return std::nullopt;
}

std::optional<const Info::PollData> Info::pollData() const {
	// SIGIO and SIGPOLL are the same on Linux, but just to be sure ...
	if (sigNr() == signal::IO_EVENT || sigNr() == signal::POLL) {
		return PollData{
			PollData::Reason{m_raw.ssi_code},
			FileNum{m_raw.ssi_fd},
			/* there are conflicting types, PollEvents is short
			 * but in sigaction it's a long */
			PollEvents{static_cast<short>(m_raw.ssi_band)}
		};
	}

	return std::nullopt;
}

} // end ns
