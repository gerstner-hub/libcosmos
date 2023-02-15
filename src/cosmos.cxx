// Linux
#include <unistd.h>

// C++
#include <atomic>
#include <iostream>
#include <map>

// Cosmos
#include "cosmos/cosmos.hxx"
#include "cosmos/private/cosmos.hxx"
#include "cosmos/private/Initable.hxx"

namespace cosmos {

// maintain a map with a priority value as key and the Initable as a value.
// the priority value allows to give a defined intialization order.
// lower priority value means earlier initialization.
typedef std::map<InitPrio, Initable*> InitableMap;

struct InitData {

	void add(const InitPrio prio, Initable *init_if);
	void init();
	void finish();
protected:
	InitableMap initables;
};

namespace {
	std::atomic<std::size_t> init_counter;
	InitData *init_data = nullptr;

	void freeInitData() {
		delete init_data;
		init_data = nullptr;
	}

	void createInitData() {
		if (init_data)
			return;

		/*
		 * this gives us a problem with static initialization order.
		 * we need to keep the map on the heap to make sure the object
		 * is valid at this point in time (because this function could
		 * be invoked in static initialization context).
		 *
		 * This on the other hand gives us trouble with freeing the
		 * heap memory, since we want to allow re-initialization. An
		 * atexit() handler will at least silence memory leaks
		 * reported by valgrind.
		 */
		init_data = new InitData;
		atexit(freeInitData);
	}
} // end anon ns

void InitData::add(const InitPrio prio, Initable *init_if) {

	auto ret = initables.insert(std::make_pair(prio, init_if));

	if (ret.second != true) {
		std::cerr << "Conflicting priority of Initables!\n";
		_exit(9);
	}
}

void InitData::init() {
	for (auto &[prio, initable]: initables) {
		initable->libInit();
		initable->m_lib_initialized = true;
	}
}

void InitData::finish() {
	// perform cleanup - in reverse order
	for (auto it = initables.rbegin(); it != initables.rend(); it++) {
		auto initable = it->second;
		initable->libExit();
		initable->m_lib_initialized = false;
	}
}

void Initable::registerInitable(const InitPrio prio) {
	createInitData();

	init_data->add(prio, this);
}

void init() {
	if (init_counter++ != 0)
		return;

	if (!init_data)
		// no initables have been registered
		return;

	// okay we need to perform initialization
	init_data->init();
}

void finish() {
	if (--init_counter != 0)
		return;

	if (!init_data)
		// no initables around
		return;

	init_data->finish();
}

RestartOnIntr auto_restart_syscalls{true};

void set_restart_syscall_on_interrupt(const bool auto_restart) {
	auto_restart_syscalls = RestartOnIntr{auto_restart};
}

} // end ns
