// Cosmos
#include <cosmos/error/RuntimeError.hxx>
#include <cosmos/io/iovector.hxx>

namespace cosmos {

static_assert(sizeof(struct iovec_const) == sizeof(struct iovec),
		"size mismatch between iovec_const vs. struct iovec in system headers");

template <typename MEMORY_REGION>
bool IOVector<MEMORY_REGION>::update(size_t processed_bytes) {
	// there's two approaches to update an io vector after partial
	// read/write operations:
	// a) removing completely processed entry from the begin of the
	//    vector and update partially processed ones
	// b) only updating pointer and length information but keeping
	//    the entry in the vector.
	// For b) the erase operation on the front of the vector is
	// somewhat expensive. For a) the re-entry into the kernel is
	// somewhat expensive, since the first entries processed will
	// potentially be finished already. For b) the advantage is that even
	// a fixed size std::array would be possible to use.  Currently we
	// follow b).
	bool vec_finished = true;

	for (auto &entry: *this) {
		processed_bytes -= entry.update(processed_bytes);

		if (!entry.finished()) {
			vec_finished = false;
			break;
		}
	}

	if (processed_bytes != 0) {
		throw RuntimeError{"inconsistency while updating IOVector"};
	}

	return vec_finished;
}

template class IOVector<InputMemoryRegion>;
template class IOVector<OutputMemoryRegion>;

} // end ns
