/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_STRLOG_H	/* wrapper symbol for kernel use */
#define _IO_STRLOG_H	/* subject to change without notice */

#ident	"@(#)kern:io/strlog.h	1.7"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Streams Log Driver Interface Definitions
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * structure of control portion of log message
 */
struct log_ctl {
	short	mid;
	short	sid;
	char 	level;		/* level of message for tracing */
	short	flags;		/* message disposition */
	clock_t	ltime;		/* time in machine ticks since boot */
	time_t	ttime;		/* time in seconds since 1970 */
	long	seq_no;		/* sequence number */
	int	pri;		/* priority = (facility|level) */
};

/* Flags for log messages */

#define SL_FATAL	0x01	/* indicates fatal error */
#define SL_NOTIFY	0x02	/* logger must notify administrator */
#define SL_ERROR	0x04	/* include on the error log */
#define SL_TRACE	0x08	/* include on the trace log */
#define SL_CONSOLE	0x10	/* log message to console */
#define SL_WARN		0x20	/* warning message */
#define SL_NOTE		0x40	/* notice message */

/*
 * Structure defining ids and levels desired by the tracer (I_TRCLOG).
 */
struct trace_ids {
	short ti_mid;
	short ti_sid;
	char  ti_level;
};

/*
 * Log Driver I_STR ioctl commands
 */

#define LOGCTL		(('L')<<8)
#define I_TRCLOG	(LOGCTL|1)	/* process is tracer */
#define I_ERRLOG	(LOGCTL|2)	/* process is error logger */
#define I_CONSLOG	(LOGCTL|3)	/* process is console logger */

/*
 * Parameter definitions for logger messages 
 */
#define LOGMSGSZ	1024
#define NLOGARGS	3

#ifdef _KERNEL
#if defined(__STDC__)
/*PRINTFLIKE5*/
extern int strlog(short, short, char, ushort_t, char *, ...);
#else
extern int strlog();
#endif
#endif	/* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif	/* _IO_STRLOG_H */
