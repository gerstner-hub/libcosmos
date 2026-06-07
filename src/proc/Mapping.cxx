// cosmos
#include <cosmos/proc/Mapping.hxx>
#include <cosmos/proc/prctl.hxx>

namespace cosmos {

void Mapping::setName(const SysString name) {
	prctl::set_anon_memory_name(m_addr, m_size, name);
}

} // end ns
