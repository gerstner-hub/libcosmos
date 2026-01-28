// cosmos
#include <cosmos/error/ApiError.hxx>
#include <cosmos/memory.hxx>
#include <cosmos/proc/ResourceUsage.hxx>
#include <cosmos/utils.hxx>

namespace cosmos {

void ResourceUsage::clear() {
	zero_object(m_ru);
}

void ResourceUsage::fetch(const Who who) {
	if (::getrusage(to_integral(who), &m_ru) != 0 ) {
		throw ApiError{"getrusage()"};
	}
}

} // end ns
