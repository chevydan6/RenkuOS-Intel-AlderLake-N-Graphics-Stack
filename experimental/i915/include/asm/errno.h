/* SPDX-License-Identifier: MIT */
#ifndef RENKU_LINUX_ERRNO_H
#define RENKU_LINUX_ERRNO_H
#include <kpi_base.h>
#ifdef RENKU_NATIVE
/* These native status macros expand through errno names. Freeze their native
 * values before installing the Linux errno namespace in this translation unit. */
enum {
	RENKU_NATIVE_BUFFER_OVERFLOW = B_BUFFER_OVERFLOW,
	RENKU_NATIVE_TOO_MANY_ARGS = B_TOO_MANY_ARGS,
	RENKU_NATIVE_FILE_TOO_LARGE = B_FILE_TOO_LARGE,
	RENKU_NATIVE_RESULT_NOT_REPRESENTABLE = B_RESULT_NOT_REPRESENTABLE,
	RENKU_NATIVE_DEVICE_NOT_FOUND = B_DEVICE_NOT_FOUND,
	RENKU_NATIVE_NOT_SUPPORTED = B_NOT_SUPPORTED
};
#undef B_BUFFER_OVERFLOW
#undef B_TOO_MANY_ARGS
#undef B_FILE_TOO_LARGE
#undef B_RESULT_NOT_REPRESENTABLE
#undef B_DEVICE_NOT_FOUND
#undef B_NOT_SUPPORTED
#define B_BUFFER_OVERFLOW RENKU_NATIVE_BUFFER_OVERFLOW
#define B_TOO_MANY_ARGS RENKU_NATIVE_TOO_MANY_ARGS
#define B_FILE_TOO_LARGE RENKU_NATIVE_FILE_TOO_LARGE
#define B_RESULT_NOT_REPRESENTABLE RENKU_NATIVE_RESULT_NOT_REPRESENTABLE
#define B_DEVICE_NOT_FOUND RENKU_NATIVE_DEVICE_NOT_FOUND
#define B_NOT_SUPPORTED RENKU_NATIVE_NOT_SUPPORTED
#endif
/* Linux-source namespace only; native entry points must translate status_t. */
#undef E2BIG
#undef EACCES
#undef EADDRINUSE
#undef EADDRNOTAVAIL
#undef EADV
#undef EAFNOSUPPORT
#undef EAGAIN
#undef EALREADY
#undef EBADE
#undef EBADF
#undef EBADFD
#undef EBADMSG
#undef EBADR
#undef EBADRQC
#undef EBADSLT
#undef EBFONT
#undef EBUSY
#undef ECANCELED
#undef ECHILD
#undef ECHRNG
#undef ECOMM
#undef ECONNABORTED
#undef ECONNREFUSED
#undef ECONNRESET
#undef EDEADLK
#undef EDEADLOCK
#undef EDESTADDRREQ
#undef EDOM
#undef EDOTDOT
#undef EDQUOT
#undef EEXIST
#undef EFAULT
#undef EFBIG
#undef EHOSTDOWN
#undef EHOSTUNREACH
#undef EHWPOISON
#undef EIDRM
#undef EILSEQ
#undef EINPROGRESS
#undef EINTR
#undef EINVAL
#undef EIO
#undef EISCONN
#undef EISDIR
#undef EISNAM
#undef EKEYEXPIRED
#undef EKEYREJECTED
#undef EKEYREVOKED
#undef EL2HLT
#undef EL2NSYNC
#undef EL3HLT
#undef EL3RST
#undef ELIBACC
#undef ELIBBAD
#undef ELIBEXEC
#undef ELIBMAX
#undef ELIBSCN
#undef ELNRNG
#undef ELOOP
#undef EMEDIUMTYPE
#undef EMFILE
#undef EMLINK
#undef EMSGSIZE
#undef EMULTIHOP
#undef ENAMETOOLONG
#undef ENAVAIL
#undef ENETDOWN
#undef ENETRESET
#undef ENETUNREACH
#undef ENFILE
#undef ENOANO
#undef ENOBUFS
#undef ENOCSI
#undef ENODATA
#undef ENODEV
#undef ENOENT
#undef ENOEXEC
#undef ENOKEY
#undef ENOLCK
#undef ENOLINK
#undef ENOMEDIUM
#undef ENOMEM
#undef ENOMSG
#undef ENONET
#undef ENOPKG
#undef ENOPROTOOPT
#undef ENOSPC
#undef ENOSR
#undef ENOSTR
#undef ENOSYS
#undef ENOTBLK
#undef ENOTCONN
#undef ENOTDIR
#undef ENOTEMPTY
#undef ENOTNAM
#undef ENOTRECOVERABLE
#undef ENOTSOCK
#undef ENOTTY
#undef ENOTUNIQ
#undef ENXIO
#undef EOPNOTSUPP
#undef EOVERFLOW
#undef EOWNERDEAD
#undef EPERM
#undef EPFNOSUPPORT
#undef EPIPE
#undef EPROTO
#undef EPROTONOSUPPORT
#undef EPROTOTYPE
#undef ERANGE
#undef EREMCHG
#undef EREMOTE
#undef EREMOTEIO
#undef ERESTART
#undef ERFKILL
#undef EROFS
#undef ESHUTDOWN
#undef ESOCKTNOSUPPORT
#undef ESPIPE
#undef ESRCH
#undef ESRMNT
#undef ESTALE
#undef ESTRPIPE
#undef ETIME
#undef ETIMEDOUT
#undef ETOOMANYREFS
#undef ETXTBSY
#undef EUCLEAN
#undef EUNATCH
#undef EUSERS
#undef EWOULDBLOCK
#undef EXDEV
#undef EXFULL
#include <asm-generic/errno.h>
#endif
