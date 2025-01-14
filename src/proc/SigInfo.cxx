// cosmos
#include <cosmos/error/UsageError.hxx>
#include <cosmos/proc/SigInfo.hxx>
#include <cosmos/proc/signal.hxx>

namespace cosmos {

SigInfo::Source SigInfo::source() const {
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

	return Source{m_raw.si_code};
}

bool SigInfo::isFaultSignal() const {
	/*
	 * should we require isTrustedSource() as well here?
	 *
	 * I guess, formally yes, but there could be some situations
	 * like in emulation or test environments, where other user
	 * space processes send such signals.
	 *
	 * The kernel checks for privileges before it allows user
	 * space to send a signal to another process, so this is less
	 * of a security concern, more a concern of maintaining
	 * integrity. If user space sends these signals then we cannot
	 * know for sure that the data found in the siginfo_t is sane.
	 */
	return in_list(sigNr(), {
			signal::ILL, signal::FPE, signal::SEGV,
			signal::BUS, signal::TRAP});
}

std::optional<const SigInfo::UserSigData> SigInfo::userSigData() const {
	if (source() == SigInfo::Source::USER) {
		return UserSigData{this->procCtx()};
	}

	return std::nullopt;
}

std::optional<const SigInfo::QueueSigData> SigInfo::queueSigData() const {
	if (source() == SigInfo::Source::QUEUE) {
		return QueueSigData{this->procCtx(), CustomData{m_raw.si_value}};
	}

	return std::nullopt;
}

std::optional<const SigInfo::MsgQueueData> SigInfo::msgQueueData() const {
	if (source() == SigInfo::Source::MESGQ) {
		return MsgQueueData{this->procCtx(), CustomData{m_raw.si_value}};
	}

	return std::nullopt;
}

std::optional<const SigInfo::TimerData> SigInfo::timerData() const {
	if (source() == SigInfo::Source::TIMER) {
		return TimerData{
			TimerData::TimerID{m_raw.si_timerid},
			m_raw.si_overrun
		};
	}

	return std::nullopt;
}

std::optional<const SigInfo::ChildData> SigInfo::childData() const {
	if (sigNr() == signal::CHILD) {
		using Event = ChildData::Event;
		const auto event = Event{m_raw.si_code};
		return ChildData{
			event,
			this->procCtx(),
			event == Event::EXITED ? std::make_optional(ExitStatus{m_raw.si_status}) : std::nullopt,
			event == Event::EXITED ? std::nullopt : std::make_optional(Signal{SignalNr{m_raw.si_status}}),
			ClockTicks{m_raw.si_utime},
			ClockTicks{m_raw.si_stime}
		};
	}

	return std::nullopt;
}

std::optional<const SigInfo::SysData> SigInfo::sysData() const {
	if (sigNr() == signal::BAD_SYS) {
		return SysData{
			SysData::Reason{m_raw.si_code},
			m_raw.si_call_addr,
			m_raw.si_syscall,
			ptrace::Arch{m_raw.si_arch},
			this->error()
		};
	}

	return std::nullopt;
}

std::optional<const SigInfo::PollData> SigInfo::pollData() const {
	// SIGIO and SIGPOLL are the same on Linux, but just to be sure ...
	if (sigNr() == signal::IO_EVENT || sigNr() == signal::POLL) {
		return PollData{
			PollData::Reason{m_raw.si_code},
			FileNum{m_raw.si_fd},
			/* there are conflicting types, PollEvents is short
			 * but in sigaction it's a long */
			PollEvents{static_cast<short>(m_raw.si_band)}
		};
	}

	return std::nullopt;
}

std::optional<const SigInfo::IllData> SigInfo::illData() const {
	if (sigNr() == signal::ILL) {
		return IllData{
			{m_raw.si_addr},
			IllData::Reason{m_raw.si_code}
		};
	}

	return std::nullopt;
}

std::optional<const SigInfo::FPEData> SigInfo::fpeData() const {
	if (sigNr() == signal::FPE) {
		return FPEData{
			{m_raw.si_addr},
			FPEData::Reason{m_raw.si_code}
		};
	}

	return std::nullopt;
}

std::optional<const SigInfo::SegfaultData> SigInfo::segfaultData() const {
	if (sigNr() == signal::SEGV) {
		using Reason = SegfaultData::Reason;
		const auto reason = Reason{m_raw.si_code};
		return SegfaultData{
			{m_raw.si_addr},
			reason,
			reason == Reason::BOUND_ERROR ?
				std::make_optional(SegfaultData::Bound{m_raw.si_lower, m_raw.si_upper}) :
				std::nullopt,
			reason == Reason::PROT_KEY_ERROR ?
				std::make_optional(SegfaultData::ProtectionKey{m_raw.si_pkey}) :
				std::nullopt
		};
	}

	return std::nullopt;
}

std::optional<const SigInfo::BusData> SigInfo::busData() const {
	if (sigNr() == signal::BUS) {
		using Reason = BusData::Reason;
		const auto reason = Reason{m_raw.si_code};
		return BusData{
			{m_raw.si_addr},
			reason,
			in_list(reason, {Reason::MCE_ACTION_REQUIRED, Reason::MCE_ACTION_OPTIONAL}) ?
				std::make_optional(m_raw.si_addr_lsb) :
				std::nullopt
		};
	}

	return std::nullopt;
}

} // end ns
