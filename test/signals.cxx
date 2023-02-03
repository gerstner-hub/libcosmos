// stdlib
#include <cassert>
#include <iostream>

// cosmos
#include "cosmos/proc/SignalFD.hxx"
#include "cosmos/proc/SigSet.hxx"
#include "cosmos/proc/Process.hxx"

int main() {
	cosmos::SigSet empty;
	cosmos::SigSet full(cosmos::SigSet::filled);
	cosmos::SigSet some;

	for (auto sigraw: {SIGINT, SIGTERM, SIGIO, SIGKILL, SIGBUS}) {
		auto sig = cosmos::Signal(sigraw);
		assert( !empty.isSet(sig) );
		assert( full.isSet(sig) );
		some.set(sig);
		full.del(sig);
		assert( some.isSet(sig) );
		assert( !full.isSet(sig) );
	}

	full.fill();
	cosmos::SigSet old;
	cosmos::g_process.blockSignals(full, &old);

	std::cout << "SIGINT was " << (old.isSet(cosmos::Signal(SIGINT)) ? "blocked" : "not blocked") << std::endl;
	cosmos::g_process.unblockSignals(full, &old);
	std::cout << "SIGINT was " << (old.isSet(cosmos::Signal(SIGINT)) ? "blocked" : "not blocked") << std::endl;

	const auto sigint = cosmos::Signal(SIGINT);

	some.clear();
	some.set(sigint);
	cosmos::g_process.setSigMask(some, &old);

	assert( !old.isSet(sigint) );
	some = cosmos::g_process.getSigMask();
	assert( some.isSet(sigint) );

	cosmos::SignalFD sfd(sigint);

	assert( sfd.valid() );

	cosmos::SignalFD::SigInfo info;

	cosmos::signal::raise(sigint);
	sfd.readEvent(info);

	assert( info.getSignal() == sigint );
	std::cout << "received " << info.getSignal() << " from " << info.getSenderPID() << std::endl;

	return 0;
}
