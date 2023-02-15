// stdlib
#include <cassert>
#include <iostream>

// cosmos
#include "cosmos/formatting.hxx"
#include "cosmos/proc/SignalFD.hxx"
#include "cosmos/proc/SigSet.hxx"
#include "cosmos/proc/Process.hxx"

using namespace cosmos;

int main() {
	cosmos::SigSet empty;
	cosmos::SigSet full(cosmos::SigSet::filled);
	cosmos::SigSet some;

	for (auto sig: {signal::INTERRUPT, signal::TERMINATE, signal::KILL, signal::IO_EVENT, signal::BUS}) {
		assert( !empty.isSet(sig) );
		assert( full.isSet(sig) );
		some.set(sig);
		full.del(sig);
		assert( some.isSet(sig) );
		assert( !full.isSet(sig) );
	}

	full.fill();
	cosmos::SigSet old;
	cosmos::signal::block(full, &old);

	std::cout << "SIGINT was " << (old.isSet(signal::INTERRUPT) ? "blocked" : "not blocked") << std::endl;
	cosmos::signal::unblock(full, &old);
	std::cout << "SIGINT was " << (old.isSet(signal::INTERRUPT) ? "blocked" : "not blocked") << std::endl;

	const auto sigint = signal::INTERRUPT;

	some.clear();
	some.set(sigint);
	cosmos::signal::set_sigmask(some, &old);

	assert( !old.isSet(sigint) );
	some = cosmos::signal::get_sigmask();
	assert( some.isSet(sigint) );

	cosmos::SignalFD sfd(sigint);

	assert( sfd.valid() );

	cosmos::SignalFD::SigInfo info;

	cosmos::signal::raise(sigint);
	sfd.readEvent(info);

	assert( info.signal() == sigint );
	std::cout << "received " << info.signal() << " from " << info.senderPID() << std::endl;

	return 0;
}
