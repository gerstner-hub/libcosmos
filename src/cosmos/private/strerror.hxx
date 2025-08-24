#pragma once

namespace cosmos {

/// wrapper around the XSI commpliant version of strerror_r, compatible e.g. with MUSL libc.
int xsi_strerror_r(int errnum, char *buf, size_t buflen);

}
