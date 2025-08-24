#ifdef _GNU_SOURCE
#	undef _GNU_SOURCE
#endif
#define _POSIX_C_SOURCE 20200405L
#include <string.h>

namespace cosmos {

/*
 * strerror_r comes in a GNU and an XSI variant. Non GNU-libc like musl libc
 * only provide the XSI variant. The signatures differ but the names are
 * the same. To export the XSI variant on GNU libc we need to #undef
 * _GNU_SOURCE. However, libstdc++ doesn't work without _GNU_SOURCE.
 * Therefore use this isolated compilation unit to wrap the XSI strerror_r to
 * stay portable.
 *
 * Even on musl libc _GNU_SOURCE seems to get defined under some circumstances
 * (c++ lib again?).
 */

int xsi_strerror_r(int errnum, char *buf, size_t buflen) {
	return strerror_r(errnum, buf, buflen);
}

} // end ns
