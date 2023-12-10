#ifndef COSMOS_IO_TYPES_HXX
#define COSMOS_IO_TYPES_HXX

// Linux
#include <sys/uio.h>

// C++
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace cosmos {

/// const variant of the struct iovec from system headers.
/**
 * `struct iovec` is used for both, input and output operations.  For being
 * able to satisfy const semantics when writing data using an IO vector we
 * need a variant of struct iovec that takes a const pointer. To do this we
 * need a data structure that is equal to the system header's struct iovec
 * with the exception of the const modifier for `iov_base`. It is casted to
 * the original `struct iovec` internally when being passed to system calls,
 * using the asIovec() helper.
 **/
struct iovec_const {
	const void *iov_base = nullptr;
	size_t iov_len = 0;
};

static_assert(sizeof(struct iovec_const) == sizeof(struct iovec),
		"size mismatch between iovec_const vs. struct iovec in system headers");

/// I/O memory region specification used with scatter/gather I/O in StreamIO API.
/**
 * This structure defines a single memory region to be used for vector
 * read/write on file descriptors. It is used a base class for read/write
 * specializations due to different constness requirements of the underlying
 * data structure.
 *
 * IOVEC is either the system defined `struct iovec` or our remodelled `struct
 * iovec_const`.
 **/
template <typename IOVEC>
struct IOMemoryRegion :
		protected IOVEC {
public: // types

	// only allow the specialized IOVector to access th raw data fields.
	template <typename ENTRY_TYPE>
	friend class IOVector;

	using PtrType = decltype(IOVEC::iov_base);

public: // functions
	auto getBase()             { return this->iov_base; }
	auto getBase() const       { return this->iov_base; }
	void setBase(PtrType base) {        this->iov_base = base; }

	size_t getLength() const      { return this->iov_len; }
	void setLength(size_t length) {        this->iov_len = length; }

	bool finished() const { return getLength() == 0; }

	/// Update the current memory region to accommodate the given number of processed bytes.
	/**
	 * Once a system call processes part or all of an IOVector, use this
	 * function to adjust the remaining length and advance the base
	 * pointer.
	 *
	 * This function returns the number of bytes that account to this
	 * IOVector entry. If more bytes have been processed than this entry
	 * holds then the next IOVector entry needs to be updated, too.
	 **/
	size_t update(const size_t processed_bytes) {
		auto to_reduce = std::min(processed_bytes, getLength());

		if constexpr (std::is_same_v<IOVEC, struct iovec_const>) {
			auto data = reinterpret_cast<const char*>(getBase());
			setBase(reinterpret_cast<PtrType>(data + to_reduce));
		} else {
			auto data = reinterpret_cast<char*>(getBase());
			setBase(reinterpret_cast<PtrType>(data + to_reduce));
		}

		setLength(getLength() - to_reduce);
		return to_reduce;
	}
};

/// IOMemoryRegion for input (read) operations.
/**
 * This variant of an IOVector entry holds a non-const base pointer for
 * writing into.
 **/
struct InputMemoryRegion :
		public IOMemoryRegion<struct ::iovec> {

	InputMemoryRegion() : InputMemoryRegion{nullptr, 0} {}

	InputMemoryRegion(void *base, size_t length) {
		setBase(base);
		setLength(length);
	}

	explicit InputMemoryRegion(std::string &s) {
		setBase(s.data());
		setLength(s.size());
	}

	static auto asIovec(InputMemoryRegion *entry) {
		return static_cast<struct iovec*>(entry);
	}
};

/// IOMemoryRegion for output (write) operations.
/**
 * This variant of an IOVector entry holds a const base pointer for reading
 * from.
 **/
struct OutputMemoryRegion :
		public IOMemoryRegion<struct iovec_const> {

	OutputMemoryRegion() : OutputMemoryRegion{nullptr, 0} {}

	OutputMemoryRegion(const void *base, size_t length) {
		setBase(base);
		setLength(length);
	}

	explicit OutputMemoryRegion(const std::string_view sv) {
		setBase(sv.data());
		setLength(sv.size());
	}

	static auto asIovec(OutputMemoryRegion *entry) {
		return reinterpret_cast<struct iovec*>(entry);
	}
};

/// A sequence of IOMemoryRegion specifications for scatter/gather I/O in the StreamIO API.
template <typename MEMORY_REGION>
class IOVector :
		public std::vector<MEMORY_REGION> {
	// only let StreamIO access the raw data
	friend class StreamIO;

public: // functions

	/// Returns the accumulated number of unprocessed bytes over the complete vector.
	size_t leftBytes() const {
		size_t ret = 0;
		for (const auto &entry: *this) {
			ret += entry.getLength();
		}

		return ret;
	}

protected: // functions

	auto raw() { return MEMORY_REGION::asIovec(this->data()); };

	/// Update the vector given the number of bytes processed by a system call.
	bool update(size_t processed_bytes);
};

using ReadIOVector = IOVector<InputMemoryRegion>;
using WriteIOVector = IOVector<OutputMemoryRegion>;

} // end ns

#endif // inc. guard
