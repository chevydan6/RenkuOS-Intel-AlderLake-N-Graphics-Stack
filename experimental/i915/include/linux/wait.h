/* SPDX-License-Identifier: MIT */
#include <linux/init.h>
/* libc declarations are already parsed. Select the original Linux wait-flag
 * spelling in this source namespace before its UAPI header is included. */
#undef WNOHANG
#undef WUNTRACED
#undef WSTOPPED
#undef WEXITED
#undef WCONTINUED
#undef WNOWAIT
#undef __WNOTHREAD
#undef __WALL
#undef __WCLONE
#include "../../upstream/linux/wait.h"
