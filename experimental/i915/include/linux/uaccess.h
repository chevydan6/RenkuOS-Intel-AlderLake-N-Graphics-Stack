/* SPDX-License-Identifier: MIT */
#ifndef RENKU_LINUX_UACCESS_H
#define RENKU_LINUX_UACCESS_H
#include <native_pages.h>
/* These control actual native fault allowance for registered task scopes.
 * Linux user-copy fixup/partial-copy semantics are not supplied by this header. */
#define pagefault_disable() renku_linux_pagefault_disable()
#define pagefault_enable() renku_linux_pagefault_enable()
#define pagefault_disabled() renku_linux_pagefault_disabled()
unsigned long renku_linux_copy_from_user(void *, const void *, unsigned long);
unsigned long renku_linux_copy_to_user(void *, const void *, unsigned long);
#define copy_from_user renku_linux_copy_from_user
#define copy_to_user renku_linux_copy_to_user
#endif
