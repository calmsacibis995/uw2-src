/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _LIMITS_H
#define _LIMITS_H
#ident	"@(#)sgs-head:i386/head/limits.h	1.58"

#define CHAR_BIT	8		/* max # of bits in a "char" */
#define SCHAR_MIN	(-128)		/* min value of a "signed char" */
#define SCHAR_MAX	127		/* max value of a "signed char" */
#define UCHAR_MAX	255		/* max value of an "unsigned char" */

#define MB_LEN_MAX	6		/* covers full UTF2 encodings */

#ifdef __STDC__

#if defined(__USLC__) || defined(__cplusplus)
#if '\377' < 0
#define CHAR_MIN	SCHAR_MIN
#define CHAR_MAX	SCHAR_MAX
#else
#define CHAR_MIN	0
#define CHAR_MAX	255
#endif
#elif #machine(i386) || #machine(sparc)
#define CHAR_MIN	SCHAR_MIN	/* min value of a "char" */
#define CHAR_MAX	SCHAR_MAX	/* max value of a "char" */
#else
#define CHAR_MIN	0		/* min value of a "char" */
#define CHAR_MAX	255		/* max value of a "char" */
#endif

#else /*!__STDC__*/

#if i386 || sparc
#define CHAR_MIN	SCHAR_MIN	/* min value of a "char" */
#define CHAR_MAX	SCHAR_MAX	/* max value of a "char" */
#else
#define CHAR_MIN	0		/* min value of a "char" */
#define CHAR_MAX	255		/* max value of a "char" */
#endif

#endif /*__STDC__*/

#define SHRT_MIN	(-32768)	/* min value of a "short int" */
#define SHRT_MAX	32767		/* max value of a "short int" */
#define USHRT_MAX	65535		/* max value of an "unsigned short int" */
#define INT_MIN		(-2147483647-1)	/* min value of an "int" */
#define INT_MAX		2147483647	/* max value of an "int" */
#define UINT_MAX	4294967295	/* max value of an "unsigned int" */
#define LONG_MIN	(-2147483647-1)	/* min value of a "long int" */
#define LONG_MAX	2147483647	/* max value of a "long int" */
#define ULONG_MAX	4294967295 	/* max value of an "unsigned long int" */

#if __STDC__ - 0 == 0 || defined(_XOPEN_SOURCE) \
	|| defined(_POSIX_SOURCE) || defined(_POSIX_C_SOURCE)

#define	ARG_MAX		10240		/* max length of arguments to exec */
#define	LINK_MAX	1000		/* max # of links to a single file */

#ifndef MAX_CANON
#define MAX_CANON	256		/* max bytes in canonical line */
#endif

#ifndef MAX_INPUT
#define	MAX_INPUT	512		/* max size of a char input buffer */
#endif

#ifndef NGROUPS_MAX
#define NGROUPS_MAX	16		/* max number of groups for a user */
#endif

#ifndef SSIZE_MAX
#define SSIZE_MAX	INT_MAX		/* max value of an "ssize_t" */
#endif

#ifndef PATH_MAX
#define	PATH_MAX	1024		/* max characters in a path name */
#endif

#ifndef PIPE_BUF
#define	PIPE_BUF	5120		/* max bytes atomic in write to a pipe */
#endif

#ifndef TZNAME_MAX
#define	TZNAME_MAX	8		/* max bytes for time zone name */
#endif

#ifndef TMP_MAX
#define TMP_MAX		17576		/* 26 * 26 * 26 */
#endif

#define _POSIX_ARG_MAX		4096
#define _POSIX_CHILD_MAX	6
#define _POSIX_LINK_MAX		8
#define _POSIX_MAX_CANON	255
#define _POSIX_MAX_INPUT	255
#define _POSIX_NAME_MAX		14
#define _POSIX_NGROUPS_MAX	0
#define _POSIX_OPEN_MAX		16
#define _POSIX_PATH_MAX		255
#define _POSIX_PIPE_BUF		512
#define _POSIX_SSIZE_MAX	32767
#define _POSIX_STREAM_MAX	8
#define _POSIX_TZNAME_MAX	3

#define _POSIX2_BC_BASE_MAX		99
#define _POSIX2_BC_DIM_MAX		2048
#define _POSIX2_BC_SCALE_MAX		99
#define _POSIX2_BC_STRING_MAX		1000
#define _POSIX2_COLL_WEIGHTS_MAX	2
#define _POSIX2_EXPR_NEST_MAX		32
#define _POSIX2_LINE_MAX		2048
#define _POSIX2_RE_DUP_MAX	 	255

#define BC_BASE_MAX		INT_MAX
#define BC_DIM_MAX		_POSIX2_BC_DIM_MAX
#define BC_SCALE_MAX		_POSIX2_BC_SCALE_MAX
#define BC_STRING_MAX		_POSIX2_BC_STRING_MAX
#define COLL_WEIGHTS_MAX	14
#define EXPR_NEST_MAX		_POSIX2_EXPR_NEST_MAX
#define LINE_MAX		_POSIX2_LINE_MAX
#define RE_DUP_MAX		5100

#define CHARCLASS_NAME_MAX	100

#define	PASS_MAX	8		/* max characters in a password */
#define LOGNAME_MAX	32		/* max characters in a login name */
#define	UID_MAX		60002		/* max value for a user or group ID */
#define	USI_MAX		4294967295	/* max decimal value of an "unsigned" */
#define	SYSPID_MAX	1		/* max pid of system processes */
#define	PIPE_MAX	5120		/* max bytes written to a pipe */

#endif

#if defined(_XOPEN_SOURCE) || (__STDC__ - 0 == 0 \
	&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE))

#ifndef NL_MSGMAX
#define	NL_MSGMAX	32767		/* max message number */
#define	NL_ARGMAX	32767		/* max positional parameter index */
#define	NL_LANGMAX	100		/* max bytes in a LANG name */
#define	NL_NMAX		1		/* max bytes in N-to-1 mapping chars */
#define	NL_SETMAX	255		/* max set number */
#define	NL_TEXTMAX	4096		/* max size text message */
#endif

#define	NZERO		20		/* default process priority */
#define	WORD_BIT	32		/* # of bits in a "word" or "int" */
#define	LONG_BIT	32		/* # of bits in a "long" */

#define	DBL_DIG		15		/* decimal digits of precision */
#define	DBL_MAX		1.7976931348623157E+308	/* max value of a "double" */
#define	FLT_DIG		6		/* decimal digits of precision */
#define	FLT_MAX		3.40282347E+38F /* max value of a "float" */

#endif

#if __STDC__ - 0 == 0 && !defined(_XOPEN_SOURCE) \
	&& !defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE)

#define	DBL_MIN		2.2250738585072014E-308	/* min value of a "double" */
#define	FLT_MIN		1.17549435E-38F /* min value of a "float" */

#define	FCHR_MAX	1048576		/* max size of a file in bytes */
#define	PID_MAX		30000		/* max value for a process ID */
#define	CHILD_MAX	25		/* max # of processes per user id */
#define	NAME_MAX	14		/* max # of characters in a file name */

#ifndef OPEN_MAX			/* max files a process can have open */
#if i386
#define	OPEN_MAX	60
#else
#define	OPEN_MAX	20
#endif
#endif

#define	STD_BLK		1024		/* bytes in a physical I/O block */

#ifndef _STYPES
#define SYS_NMLN	257		/* 4.0 size of utsname elements */
#else
#define SYS_NMLN	9		/* old size of utsname elements */
#endif

#ifdef __cplusplus
extern "C" {
#endif
#ifdef __STDC__
extern long	_sysconf(int);
#else
extern long	_sysconf();
#endif
#ifdef __cplusplus
}
#endif

#ifndef CLK_TCK
#define CLK_TCK		_sysconf(3)	/* clocks/second (_SC_CLK_TCK is 3) */
#endif

#endif

#endif /*_LIMITS_H*/
