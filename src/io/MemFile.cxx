// cosmos
#include <cosmos/error/ApiError.hxx>
#include <cosmos/formatting.hxx>
#include <cosmos/io/MemFile.hxx>
#include <cosmos/private/cosmos.hxx>
#include <cosmos/utils.hxx>

namespace cosmos {

void MemFile::create(const SysString name, const OpenFlags flags, const HugePageSize tlb_ps) {

	close();

	// see `man mmap` for an explanation about this.
	const auto page_size = to_integral(tlb_ps) << MAP_HUGE_SHIFT;
	auto fd = ::memfd_create(name.raw(), flags.raw() | page_size);

	if (fd == -1) {
		throw ApiError{"memfd_create()"};
	}

	m_fd.setFD(FileNum{fd});
}

} // end ns
