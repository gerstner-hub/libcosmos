#ifndef COSMOS_FILE_TYPES_HXX
#define COSMOS_FILE_TYPES_HXX

// Linux
#include <sys/stat.h>

// cosmos
#include "cosmos/types.hxx"

namespace cosmos {

/// Strong boolean type to enable following of symlinks in the file system
using FollowSymlinks = NamedBool<struct follow_links_t, false>;

/// Represents a file type and mode
/**
 * This is wrapper around the primitive mode_t describing file types and
 * classical UNIX file permissions and mode bits.
 **/
class FileMode {
public:
	/// Constructs a FileMode from a fully specified numerical value
	explicit FileMode(mode_t mode = 0) : m_mode(mode) {}

	bool isRegular()  const { return S_ISREG(m_mode);  }
	bool isDir()      const { return S_ISDIR(m_mode);  }
	bool isCharDev()  const { return S_ISCHR(m_mode);  }
	bool isBlockDev() const { return S_ISBLK(m_mode);  }
	bool isFIFO()     const { return S_ISFIFO(m_mode); }
	bool isLink()     const { return S_ISLNK(m_mode);  }
	bool isSocket()   const { return S_ISSOCK(m_mode); }

	bool hasSetUID() const { return (m_mode & S_ISUID) != 0; }
	bool hasSetGID() const { return (m_mode & S_ISGID) != 0; }
	bool hasSticky() const { return (m_mode & S_ISVTX) != 0; }

	bool canOwnerRead()  const { return (m_mode & S_IRUSR) != 0; }
	bool canOwnerWrite() const { return (m_mode & S_IWUSR) != 0; }
	bool canOwnerExec()  const { return (m_mode & S_IXUSR) != 0; }

	bool canGroupRead()  const { return (m_mode & S_IRGRP) != 0; }
	bool canGroupWrite() const { return (m_mode & S_IWGRP) != 0; }
	bool canGroupExec()  const { return (m_mode & S_IXGRP) != 0; }

	bool canOthersRead()  const { return (m_mode & S_IROTH) != 0; }
	bool canOthersWrite() const { return (m_mode & S_IWOTH) != 0; }
	bool canOthersExec()  const { return (m_mode & S_IXOTH) != 0; }

	bool canAnyExec() const {
		return canOwnerExec() || canGroupExec() || canOthersExec();
	}

	/// Returns the file permissions bits only (file type stripped off)
	mode_t getPermBits() const { return m_mode & (~S_IFMT); }

	mode_t raw() const { return m_mode; }
protected: // data
	/// Plain file mode value
	mode_t m_mode;
};

} // end ns

#endif // inc. guard
