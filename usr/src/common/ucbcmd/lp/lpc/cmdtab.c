/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ucb:common/ucbcmd/lp/lpc/cmdtab.c	1.2"
#ident	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * lpc -- command tables
 */

#include "lpc.h"

char	aborthelp[] =	"terminate a spooling daemon immediately and disable printing";
char	cleanhelp[] =	"remove cruft files from a queue";
char	enablehelp[] =	"turn a spooling queue on";
char	disablehelp[] =	"turn a spooling queue off";
char	downhelp[] =	"do a 'stop' followed by 'disable' and put a message in status";
char	helphelp[] =	"get help on commands";
char	quithelp[] =	"exit lpc";
char	restarthelp[] =	"kill (if possible) and restart a spooling daemon";
char	starthelp[] =	"enable printing and start a spooling daemon";
char	statushelp[] =	"show status of daemon and queue";
char	stophelp[] =	"stop a spooling daemon after current job completes and disable printing";
char	topqhelp[] =	"put job at top of printer queue";
char	uphelp[] =	"enable everything and restart spooling daemon";

struct cmd cmdtab[] = {
	{ "abort",	aborthelp,	_abort,		1 },
	{ "clean",	cleanhelp,	clean,		1 },
	{ "enable",	enablehelp,	enable,		1 },
	{ "exit",	quithelp,	quit,		0 },
	{ "disable",	disablehelp,	disable,	1 },
	{ "down",	downhelp,	down,		1 },
	{ "help",	helphelp,	help,		0 },
	{ "quit",	quithelp,	quit,		0 },
	{ "restart",	restarthelp,	restart,	0 },
	{ "start",	starthelp,	start,		1 },
	{ "status",	statushelp,	status,		0 },
	{ "stop",	stophelp,	stop,		1 },
	{ "topq",	topqhelp,	topq,		1 },
	{ "up",		uphelp,		up,		1 },
	{ "?",		helphelp,	help,		0 },
	{ 0 },
};

int	NCMDS = sizeof (cmdtab) / sizeof (cmdtab[0]) - 1;
