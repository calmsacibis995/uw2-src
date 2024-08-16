/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_LOG_LOG_H	/* wrapper symbol for kernel use */
#define _IO_LOG_LOG_H	/* subject to change without notice */

#ident	"@(#)kern-i386:io/log/log.h	1.9"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Header file for the STREAMS Log Driver
 */

#ifdef _KERNEL_HEADERS

#include <io/stream.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/stream.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Minor devices for the log driver.
 * 0 through 5 are reserved for well-known entry points.
 * Minors above CLONEMIN are given out on clone opens.
 */
#define CONSWMIN	0	/* minor device to write to console log */
#define CLONEMIN	5	/* minor device of clone interface */

struct log {
	unsigned log_state;	/* driver state; see below */
	queue_t *log_rdq;	/* pointer to read queue */
	mblk_t	*log_tracemp;	/* trace_ids (list of the triplet) */
};

/*
 * Driver state values
 */
#define LOGOPEN 	0x01
#define LOGERR		0x02
#define LOGTRC		0x04
#define LOGCONS		0x08

/* 
 * Module information structure fields
 */
#define LOG_MID		44
#define LOG_NAME	"LOG"
#define LOG_MINPS	0
#define LOG_MAXPS	256
#define LOG_HIWAT	4096
#define LOG_LOWAT	256

/*
 * STRLOG(mid,sid,level,flags,fmt,args) should be used for those trace
 * calls that are only to be made during debugging.
 */
#if defined(DEBUG) || defined(lint)
#define STRLOG	strlog
#else
#define STRLOG	0 && strlog
#endif

/*
 * Utility macros for strlog.
 */
/*
 * logadjust(wp) -
 *	move a character pointer "wp" up to the next int boundary after
 *	its current value.  Assumes sizeof(int) is 2**n bytes for some
 *	integer n. 
 */
#define logadjust(wp) (char *)(((unsigned)wp + sizeof(int)) & ~(sizeof(int)-1))
/*
 * logstrcpy(dp, sp) -
 *	copies string "sp" to "dp".
 *	This is a catchall definition for those processors that have not
 *	had this coded in assembler above.
 */
#define logstrcpy(dp, sp)  for (; (*dp = *sp) != 0; dp++, sp++)

#ifdef _KERNEL
extern int canconslog(void);
#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_LOG_LOG_H */
