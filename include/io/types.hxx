#ifndef COSMOS_IO_TYPES_HXX
#define COSMOS_IO_TYPES_HXX

// cosmos
#include "cosmos/dso_export.h"

// Linux
#include <sys/uio.h>

// C++
#include <string>
#include <vector>

namespace cosmos {

// TODO: the iovec structure is problematic when trying to pass const data
// into it. There's no way to make it const without a cast. We could remodel
// `struct iovec` as `struct iovec_const` with a const pointer, using
// `static_assert` to make sure no suprises happen regarding the struct layout
// and size. Then we could provide a ReadIOVector and a WriteIOVector, the
// latter using the const structure.
// In the implementation the remodelled `struct iovect_const` would need to be
// casted into the expected `struct iovec`.

/// I/O data specification used with scatter/gather I/O in StreamIO API.
/**
 * This structure defines a single memory region to be used for vector
 * read/write on file descriptors. Use IOVector for a sequence of
 * IOVectorEntry to read into or write from.
 **/
struct IOVectorEntry :
		protected ::iovec {

	// only allow the specialized IOVector to access th raw data fields.
	friend class IOVector;

	IOVectorEntry() : IOVectorEntry{nullptr, 0} {}

	IOVectorEntry(void *base, size_t length) {
		setBase(base);
		setLength(length);
	}

	explicit IOVectorEntry(std::string &s) {
		setBase(s.data());
		setLength(s.size());
	}

	void* getBase() { return iov_base; }
	const void* getBase() const { return iov_base; }
	void setBase(void *base) { iov_base = base; }

	size_t getLength() const { return iov_len; }
	void setLength(size_t length) { iov_len = length; }

	bool empty() const { return getLength() == 0; }

	/// Update the current memory region to accommodate the given number of processed bytes.
	/**
	 * Once a system call processes part of all of an IOVector, use this
	 * update() function to adjust the remaining length and advance the
	 * base pointer.
	 *
	 * This function return the number of bytes that account to this
	 * IOVectorEntry. If more bytes have been processed than this entry
	 * holds then the next IOVector entry needs to be updated, too.
	 **/
	size_t update(const size_t processed_bytes) {
		auto to_reduce = std::min(processed_bytes, getLength());

		auto data = reinterpret_cast<char*>(getBase());
		setBase(reinterpret_cast<void*>(data + to_reduce));
		setLength(getLength() - to_reduce);

		return to_reduce;
	}
};

/// A sequence of IOVectorEntry specifications for scatter/gather I/O in the StreamIO API.
class COSMOS_API IOVector :
		public std::vector<IOVectorEntry> {
	// only let StreamIO access the raw data
	friend class StreamIO;

public: // functions	

	size_t leftBytes() const {
		size_t ret = 0;
		for (const auto &entry: *this) {
			ret += entry.getLength();
		}

		return ret;
	}

protected: // functions
	auto raw() { return static_cast<struct iovec*>(data()); };
	auto raw() const { return static_cast<const struct iovec*>(data()); };

	/// Update the vector given the number of bytes processed by a system call.
	bool update(size_t processed_bytes);
};

} // end ns

#endif // inc. guard
