/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SVC_SYSCONFIG_H	/* wrapper symbol for kernel use */
#define _SVC_SYSCONFIG_H	/* subject to change without notice */

#ident	"@(#)kern-i386:svc/sysconfig.h	1.5"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * cmd values for _sysconfig system call. 
 * WARNING: This is an undocumented system call,
 * therefore future compatibility can not be guaranteed. 
 */ 

#define UNUSED			1
#define _CONFIG_NGROUPS		2	/* # configured supplemental groups */
#define _CONFIG_CHILD_MAX	3	/* max # of processes per uid session */
#define _CONFIG_OPEN_FILES	4	/* max # of open files per process */
#define _CONFIG_POSIX_VER	5	/* POSIX version */
#define _CONFIG_PAGESIZE	6	/* system page size */
#define _CONFIG_CLK_TCK		7	/* ticks per second */
#define _CONFIG_XOPEN_VER	8	/* XOPEN version */
#define _CONFIG_NACLS_MAX	9	/* for Enhanced Security */
#define	_CONFIG_ARG_MAX		10	/* max length of exec args */
#define _CONFIG_NPROC		11	/* # processes system is config for */
#define _CONFIG_NENGINE		12	/* # configured processors (CPUs) */
#define _CONFIG_NENGINE_ONLN	13	/* # online processors (CPUs) */

#if defined(__cplusplus)
	}
#endif

#endif /* _SVC_SYSCONFIG_H */
