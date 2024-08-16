/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:proc/proc.cf/Space.c	1.9"
#ident	"$Header: $"

#include <config.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/exec.h>

char	initclass[] = INITCLASS;

extern void nudge();
extern void kpnudge();

#ifdef DEBUG
void    (*sys_nudge)() = kpnudge;         /* set default sys class nudge here */

#else

void    (*sys_nudge)() = nudge;         /* set default sys class nudge here */
#endif


int	ngroups_max = NGROUPS_MAX;

int	exec_ncargs = ARG_MAX;
long	maxcachewarm=MAXCACHEWARM;
int maximbalance=MAXIMBALANCE;
int	load_balance_freq=LOAD_BAL_FREQ;

/*
 * System default resource limits:
 */
struct rlimit sysdef_rlimits[] = {
	SCPULIM, HCPULIM,
	SFSZLIM, HFSZLIM,
	SDATLIM, HDATLIM,
	SSTKLIM, HSTKLIM,
	SCORLIM, HCORLIM,
	SFNOLIM, HFNOLIM,
	SVMMLIM, HVMMLIM
};

/* COFF shared library support */
struct shlbinfo shlbinfo = { SHLBMAX };

/* Stack overflow support */
int ovstack_size = OVSTACK_SIZE;
int ovstack_preempt = OVSTACK_PREEMPT;
