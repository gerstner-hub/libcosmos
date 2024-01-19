#pragma once

// cosmos
#include "cosmos/fs/filesystem.hxx"
#include "cosmos/fs/FileBase.hxx"

namespace cosmos {

/// Specialization of FileBase for managing temporary files.
/**
 * Creates a named temporary file in a template path and manages the lifetime
 * of the resulting file descriptor and of the file on file system level.
 *
 * Upon close() both the file descriptor will be closed and the file on disk
 * will be unlinked.
 *
 * \see cosmos::fs::make_tempfile() for details about the structure of the \c
 * _template path.
 **/
class COSMOS_API TempFile :
		public FileBase {
public: // functions

	TempFile() = default;

	explicit TempFile(const std::string_view _template, const OpenFlags flags = OpenFlags{OpenFlag::CLOEXEC}) {
		open(_template, flags);
	}

	~TempFile();

	TempFile& operator=(TempFile &&other) {
		m_tmp_path = other.m_tmp_path;
		other.m_tmp_path.clear();

		FileBase::operator=(std::move(other));
		return *this;
	}

	// Prevent copying due to the path deletion responsibility.

	void close() override {
		try {
			FileBase::close();
		} catch(...) {
			unlinkPath();
			throw;
		}

		unlinkPath();
	}

	void open(const std::string_view _template, const OpenFlags flags = OpenFlags{OpenFlag::CLOEXEC}) {
		close();

		auto [fd, path] = fs::make_tempfile(_template, flags);
		m_fd = fd;
		m_tmp_path = path;
	}

	/// Returns the expanded path to the temporary file.
	/**
	 * This is only valid if currently a temporary file is open. Otherwise
	 * a UsageError will be thrown.
	 **/
	const std::string& path() const;

protected: // functions

	void unlinkPath() {
		if (!m_tmp_path.empty()) {
			try {
				fs::unlink_file(m_tmp_path);
			} catch(...) {
				m_tmp_path.clear();
				throw;
			}
			m_tmp_path.clear();
		}
	}

protected: // data

	std::string m_tmp_path;
};

} // end ns
