/* SPDX-License-Identifier: MIT */
#include <kpi_base.h>
#ifndef RENKU_UAPI_LINUX_LIMITS_H
#define RENKU_UAPI_LINUX_LIMITS_H
/* Select Linux UAPI limits after native declarations. No native VFS ABI is
 * implemented by these constants. */
#undef NR_OPEN
#undef NGROUPS_MAX
#undef ARG_MAX
#undef LINK_MAX
#undef MAX_CANON
#undef MAX_INPUT
#undef NAME_MAX
#undef PATH_MAX
#undef PIPE_BUF
#undef XATTR_NAME_MAX
#undef XATTR_SIZE_MAX
#undef XATTR_LIST_MAX
#undef RTSIG_MAX
#include "../../../upstream/uapi/linux/limits.h"
#endif
