/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	global_h
#define	global_h
#ident	"@(#)debugger:inc/common/global.h	1.14"

// Declare global variables which almost every module needs.

#include "List.h"
#include <sys/types.h>
#include <signal.h>
#include <stdio.h>


#ifdef DEBUG
extern int	debugflag;	// global debugging flag
#endif

extern sigset_t	interrupt;	
extern int	pathage;	
extern List	waitlist;

extern long	pagesize;

extern  sigset_t	debug_sset;
extern  sigset_t	sset_PI;	// poll and interrupt
extern  sigset_t	orig_mask;

extern const char	*follow_path;
extern const char	*thread_library_name;
extern char		*prog_name;
extern FILE		*log_file;
extern char		*original_dir;	// original working directory

extern int	quitflag;	// leave debugger

// variables used internally for support of debugger
// "%" variables
extern char	*global_path;
extern int	cmd_result;	// result status of previous command
extern int	last_error;	// last error message issued
extern int	vmode; 		// verbosity
extern int	wait_4_proc;	// background or foreground exec
extern int	follow_mode;	// follow children, threads, all, none
extern int	redir_io;	// should process I/O be redirected?
extern int	synch_mode;	// global sync/asynch flag
extern int	num_line;	// number of lines for list cmd
extern int	num_bytes;	// number of bytes for dump cmd
#ifdef DEBUG_THREADS
extern int	thr_change;  // thread change mode
#endif

#endif	/* global_h */
