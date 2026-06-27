// C++
#include <functional>

// cosmos
#include <cosmos/proc/AuxVector.hxx>

namespace cosmos {
namespace {

	struct RawEntry {
		using enum AuxVector::Type;

		AuxVector::Type type;
		unsigned long value;

		bool isString() const {
			switch (type) {
				case BASE_PLATFORM:
				case EXECFN:
				case PLATFORM: return true;
				default: return false;
			}
		}

		bool isPointer() const {
			switch (type) {
				case BASE:
				case ENTRY:
				case PHDR:
				case SYSINFO:
				case SYSINFO_EHDR: return true;
				default: return false;
			}
		}

		bool isFileNum() const {
			return type == EXECFD;
		}

		bool isBuffer() const {
			return type == RANDOM;
		}

		bool isUID() const {
			return type == UID || type == EUID;
		}

		bool isGID() const {
			return type == GID || type == EGID;
		}

		bool isBool() const {
			return type == SECURE;
		}

		/*
		 * this special entry terminates the vector, the data returned
		 * by the kernel is a fixed buffer with excess bytes.
		 */
		bool isNull() const {
			return type == AuxVector::Type{AT_NULL};
		}
	};

	AuxVector::Data make_data(const RawEntry &entry) {
		if (entry.isString()) {
			return std::string_view{reinterpret_cast<const char*>(entry.value)};
		} else if (entry.isPointer()) {
			return reinterpret_cast<void*>(entry.value);
		} else if (entry.isUID()) {
			return cosmos::UserID{static_cast<uid_t>(entry.value)};
		} else if (entry.isGID()) {
			return cosmos::GroupID{static_cast<gid_t>(entry.value)};
		} else if (entry.isBuffer()) {
			/* currently a lex RANDOM, fixed 16 bytes */
			return std::span<const std::byte>(reinterpret_cast<const std::byte*>(entry.value), 16);
		} else if (entry.isBool()) {
			return static_cast<bool>(entry.value);
		} else if (entry.isFileNum()) {
			return cosmos::FileNum{static_cast<int>(entry.value)};
		} else {
			/* otherwise just a number */
			return entry.value;
		}
	}

	/*
	 * Iterate over all aux entries calling `cb` for each entry. When it
	 * returns `false` then the iteration stops.
	 */
	void iterate(std::span<const std::byte> buffer, std::function<bool(const RawEntry&)> cb) {
		const size_t num_entries = buffer.size() / sizeof(RawEntry);
		const struct RawEntry *entry = reinterpret_cast<const RawEntry*>(buffer.data());

		for (size_t num = 0; num < num_entries && !entry->isNull(); num++, entry++) {
			if (!cb(*entry)) {
				return;
			}
		}
	}

} // end anon ns

std::optional<AuxVector::Data> AuxVector::lookupValue(const Type type) const {
	std::optional<AuxVector::Data> ret;

	iterate(m_view, [&ret, type](auto &entry) {
		if (entry.type == type) {
			ret.emplace(make_data(entry));
			return false;
		}

		return true;
	});

	return ret;
}

std::map<AuxVector::Type, AuxVector::Data> AuxVector::asMap() const {
	std::map<Type, Data> ret;

	iterate(m_view, [&ret](auto &entry) {
		ret.insert(std::make_pair(entry.type, make_data(entry)));
		return true;
	});

	return ret;
}

} // end ns
