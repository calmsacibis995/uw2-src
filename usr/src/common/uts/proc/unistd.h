/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PROC_UNISTD_H	/* wrapper symbol for kernel use */
#define _PROC_UNISTD_H	/* subject to change without notice */

#ident	"@(#)kern:proc/unistd.h	1.16"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * WARNING: This is an implementation-specific header,
 * its contents are not guaranteed. Applications
 * should include <unistd.h> and not this header.
 */

/* Symbolic constants for the 'access' routine: */
#define R_OK    004       /* Test for Read permission */
#define W_OK    002       /* Test for Write permission */
#define X_OK    001       /* Test for eXecute permission */
#define F_OK    000       /* Test for existence of File */

#if !defined(_POSIX_SOURCE) && !defined(_XOPEN_SOURCE) || defined(_KERNEL)
#define EFF_ONLY_OK	010	/* Test using effective ids */
#define EX_OK		020	/* Test for Regular, executable file */
#endif

/* Symbolic constants for the "lseek" routine: */
#ifndef SEEK_SET
#define SEEK_SET        0       /* Set file pointer to "offset" */
#define SEEK_CUR        1       /* Set file pointer to current plus "offset" */
#define SEEK_END        2       /* Set file pointer to EOF plus "offset" */
#endif  /* SEEK_SET */


/* command names for POSIX sysconf */

#define _SC_ARG_MAX	1
#define _SC_CHILD_MAX	2
#define _SC_CLK_TCK	3
#define _SC_NGROUPS_MAX 4
#define _SC_OPEN_MAX	5
#define _SC_JOB_CONTROL 6
#define _SC_SAVED_IDS	7
#define _SC_VERSION	8
#define _SC_PASS_MAX	9
#define _SC_LOGNAME_MAX	10
#define _SC_PAGESIZE	11
#define _SC_PAGE_SIZE	11
#define _SC_XOPEN_VERSION 12
#define _SC_NACLS_MAX   13
#define _SC_NPROCESSORS_CONF	14
#define _SC_NPROCESSORS_ONLN	15
#define _SC_NPROCESSES	39
#define _SC_TZNAME_MAX	320
#define _SC_STREAM_MAX	321
#define _SC_XOPEN_CRYPT	323
#define _SC_XOPEN_ENH_I18N	324
#define _SC_XOPEN_SHM	325
#define _SC_XOPEN_XCU_VERSION	327
#define _SC_AES_OS_VERSION	330
#define _SC_ATEXIT_MAX	331
#define _SC_2_C_BIND	350
#define _SC_2_C_DEV	351
#define _SC_2_C_VERSION	352
#define _SC_2_CHAR_TERM	353
#define _SC_2_FORT_DEV	354
#define _SC_2_FORT_RUN	355
#define _SC_2_LOCALEDEF	356
#define _SC_2_SW_DEV	357
#define _SC_2_UPE	358
#define _SC_2_VERSION	359
#define _SC_BC_BASE_MAX	370
#define _SC_BC_DIM_MAX	371
#define _SC_BC_SCALE_MAX	372
#define _SC_BC_STRING_MAX	373
#define _SC_COLL_WEIGHTS_MAX	380
#define _SC_EXPR_NEST_MAX	381
#define _SC_LINE_MAX	382
#define _SC_RE_DUP_MAX	383
#define _SC_IOV_MAX		390	/* 4.0MP COMPAT */
#define _SC_NPROC_CONF		391	/* 4.0MP COMPAT */
#define _SC_NPROC_ONLN		392	/* 4.0MP COMPAT */

/* command names for XPG4 confstr */
#define _CS_PATH		1
#define _CS_HOSTNAME		2	/* name of node */
#define _CS_RELEASE 		3	/* release of operating system */
#define _CS_VERSION		4	/* version field of utsname */
#define _CS_MACHINE		5	/* kind of machine */
#define _CS_ARCHITECTURE	6	/* instruction set arch */
#define _CS_HW_SERIAL		7	/* hardware serial number */
#define _CS_HW_PROVIDER		8	/* hardware manufacturer */
#define _CS_SRPC_DOMAIN		9	/* secure RPC domain */
#define _CS_INITTAB_NAME	10	/* name of inittab file used */
#define _CS_SYSNAME		11	/* name of operating system */

/* command names for POSIX pathconf */

#define _PC_LINK_MAX	1
#define _PC_MAX_CANON	2
#define _PC_MAX_INPUT	3
#define _PC_NAME_MAX	4
#define _PC_PATH_MAX	5
#define _PC_PIPE_BUF	6
#define _PC_NO_TRUNC	7
#define _PC_VDISABLE	8
#define _PC_CHOWN_RESTRICTED	9

#ifndef _POSIX_VERSION
#define _POSIX_VERSION	199009L
#endif

#ifndef _XOPEN_VERSION
#define _XOPEN_VERSION 4
#endif

#if defined(__cplusplus)
        }
#endif
#endif /* _PROC_UNISTD_H */
