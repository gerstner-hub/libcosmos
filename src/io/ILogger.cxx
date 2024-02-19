// C++
#include <iostream>

// Cosmos
#include "cosmos/fs/FileDescriptor.hxx"
#include "cosmos/io/ILogger.hxx"
#include "cosmos/io/Terminal.hxx"

namespace cosmos {

using namespace cosmos::term;

ILogger::ILogger() :
	m_err{  "Error:   ", FrontColor{TermColor::RED}},
	m_warn{ "Warning: ", FrontColor{TermColor::YELLOW}},
	m_info{ "Info:    ", FrontColor{TermColor::WHITE}},
	m_debug{"Debug:   ", FrontColor{TermColor::CYAN}}
{}

bool ILogger::isTTY(const std::ostream &o) {
	/*
	 * there's no elegant, portable way to get the file descriptor from an
	 * ostream thus we have to use some heuristics ...
	 */

	FileDescriptor fd_to_check;

	if (auto thisbuf = o.rdbuf(); thisbuf == std::cout.rdbuf()) {
		fd_to_check.setFD(FileNum::STDOUT);
	} else if (thisbuf == std::cerr.rdbuf()) {
		fd_to_check.setFD(FileNum::STDERR);
	} else {
		return false;
	}

	return Terminal{fd_to_check}.isTTY();
}

void ILogger::setStream(std::ostream &s, StreamState &state) {
	state.stream = &s;
	state.is_tty = isTTY(s);
	state.enabled = (&state == &m_debug) ? false : true;
}

void ILogger::setStreams(
		std::ostream &debug, std::ostream &info,
		std::ostream &warn, std::ostream &err) {
	setStream(debug, m_debug);
	setStream(err, m_err);
	setStream(info, m_info);
	setStream(warn, m_warn);
}

} // end ns
