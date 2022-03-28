// stdlib
#include <atomic>
#include <iostream>
#include <map>

// Linux
#include <unistd.h>

// Cosmos
#include "cosmos/Init.hxx"
#include "cosmos/private/Initable.hxx"

namespace cosmos {

static std::atomic<std::size_t> g_init_counter;
// maintain a map with a priority value as key and the Initable as a value.
// the priority value allows to give a defined intialization order.
// lower priority value means earlier initialization.
typedef std::map<InitPrio, Initable*> InitableMap;
static InitableMap *g_init_map = nullptr;

void freeInitMap() {
	delete g_init_map;
	g_init_map = nullptr;
}

void Initable::registerInitable(const InitPrio prio) {
	if (!g_init_map) {
		/*
		 * this gives us a problem with static initialization order.
		 * we need to keep the map on the heap to make sure the object
		 * is valid at this point in time.
		 *
		 * this on the other hand gives us trouble with freeing the
		 * heap memory. an atexit() handler will at least silence
		 * memory leaks reported by valgrind.
		 */
		g_init_map = new InitableMap;
		atexit(freeInitMap);
	}

	auto ret = g_init_map->insert(std::make_pair(prio, this));

	if (ret.second != true) {
		std::cerr << "Conflicting priority of Initables!" << std::endl;
		_exit(9);
	}
}

void initLibCosmos() {
	if (g_init_counter++ != 0)
		return;

	if (!g_init_map)
		return;

	// okay we need to perform initialization
	for (auto &pair: *g_init_map) {
		auto initable = pair.second;
		initable->libInit();
		initable->m_lib_initialized = true;
	}
}

void finishLibCosmos() {

	if (--g_init_counter != 0)
		return;

	if (!g_init_map)
		return;

	// okay we need to perform cleanup
	for (auto it = g_init_map->rbegin(); it != g_init_map->rend(); it++) {
		auto initable = it->second;
		initable->libExit();
		initable->m_lib_initialized = false;
	}
}

} // end ns
