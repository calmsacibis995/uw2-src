/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma	ident	"@(#)xidlelock:xidlelock.h	1.2"


#ifndef MAIN_H
#define MAIN_H

#define APPNAME "xidlelock"

#ifdef DEBUG
#define PRINT_DEBUG_MSG(x,a,b)	fprintf(stderr, x,a,b); fflush(stderr)
#else
#define PRINT_DEBUG_MSG(x,a,b)
#endif

/*
 *	Xidlelock Preference Resources data save area
 */
typedef struct {
	Boolean notify;
	char *locker;
	int timeout;
} XidlelockApplicationData, *XidlelockApplicationDataPtr;

extern XidlelockApplicationData	Xidlelockdata;

/*
 *
 */
extern char * GetStr (char *);

#ifndef FS
#define FS	"\001"
#define FS_CHR	'\001'
#endif

#ifdef HELP
#define HELP_FILE		"/usr/X/lib/locale/C/help/xidleprefs/xidleprefs.hlp"
#endif



/*
 * Usage message strings
 */
#define TXT_usage	"xidlelock:1" FS "Usage: xidlelock [-timeout minutes][-locker locker] [-notify]"
#define TXT_timeout	"xidlelock:2" FS "-timeout minutes : time to lock screen [min = %d, max = %d]\n"
#define TXT_locker	"xidlelock:3" FS "-locker locker   : program used to lock"
#define TXT_notify	"xidlelock:4" FS "-notify          : warns screen is about to be locked"
#define TXT_defaults	"xidlelock:5" FS "Defaults :"
#define TXT_def_timeout	"xidlelock:6" FS "      timeout   : OFF\n"
#define TXT_def_locker	"xidlelock:7" FS "      locker    : %s\n"
#define TXT_def_notify	"xidlelock:8" FS "      notify    : FALSE"

/*
 * General messages
 */
  
#define TXT_already	"xidlelock:9" FS "xidlelock is already running (PID %d)"
#define TXT_exclusive	"xidlelock:10" FS "xidlelock can not determine if another xidlelock already running"
#define TXT_malloc	"xidlelock:11" FS "xidlelock:Error during argument malloc: EXITING"
#define TXT_ext	"xidlelock:12" FS "Server doesn't support XIdle Extension"
#define TXT_exec	"xidlelock:13" FS "Unable to execute .... Aborting"
#define TXT_child	"xidlelock:14" FS "Child disinherit failed"
#define TXT_fork	"xidlelock:15" FS "ERROR:  xidlelock CAN'T fork, EXITING"
#define TXT_min	"xidlelock:16" FS "Warning: time is set to minimum value of %d minute(s)"
#define TXT_max	"xidlelock:17" FS "Warning: time is set to maximum time of %d minute(s)"

#endif
