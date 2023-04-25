#ifndef COSMOS_TEMPDIR_HXX
#define COSMOS_TEMPDIR_HXX

// cosmos
#include "cosmos/fs/filesystem.hxx"

namespace cosmos {

/// Creation and lifetime management for temporary directories.
/**
 * Create a temporary directory based on a name template. See
 * fs::make_tempdir() for details on the template requirements.
 *
 * Upon close() the temporary directory will be recursively removed.
 **/
class COSMOS_API TempDir {
public: // functions
	
	TempDir() = default;
	
	explicit TempDir(const std::string_view _template) {
		create(_template);
	}

	~TempDir();

	// Prevent copying due to the path deletion responsibility.
	TempDir(const TempDir&) = delete;
	TempDir& operator=(const TempDir&) = delete;

	TempDir(TempDir &&other) {
		*this = std::move(other);
	}

	TempDir& operator=(TempDir &&other) {
		m_tmp_path = other.m_tmp_path;
		other.m_tmp_path.clear();
		return *this;
	}

	void close() {
		if (!m_tmp_path.empty()) {
			fs::remove_tree(m_tmp_path);
			m_tmp_path.clear();
		}
	}

	void create(const std::string_view _template);

	/// Returns the expanded path to the temporary dir.
	/**
	 * This is only valid if currently a temporary file is open. Otherwise
	 * a UsageError will be thrown.
	 **/
	const std::string& path() const;

protected: // data

	std::string m_tmp_path;
};

} // end ns

#endif // inc. guard
