/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


# ident	"@(#)saf:global.c	1.7.7.2"
#ident  "$Header: global.c 1.2 91/07/01 $"

# include <stdio.h>
# include <sac.h>
# include <sys/types.h>
# include "misc.h"
# include "structs.h"

/*
 * possible error messages that can be generated by the sac
 */

struct errmsg Msgs[] = {
	"could not open _sactab", 1,		/* E_SACOPEN */
	"malloc failed", 2,			/* E_MALLOC */
	"_sactab file is corrupt", 3,		/* E_BADFILE */
	"_sactab version # is incorrect", 4,	/* E_BADVER */
	"can not chdir to home directory", 5,	/* E_CHDIR */
	"could not open _sacpipe", 6,		/* E_NOPIPE */
	"internal error - bad state", 7,	/* E_BADSTATE */
	"read of _sacpipe failed", 8,		/* E_BADREAD */
	"fattach failed", 9,		 	/* E_FATTACH */
	"I_SETSIG failed", 10,		 	/* E_SETSIG */
	"read failed", 11,			/* E_READ */
	"poll failed", 12,			/* E_POLL */
	"system error in _sysconfig", 13,	/* E_SYSCONF */
	"error interpreting _sysconfig", 14,	/* E_BADSYSCONF */
	"pipe failed", 15,			/* E_PIPE */
	"could not create _cmdpipe", 16,	/* E_CMDPIPE */
	"could not set ownership", 17,		/* E_CHOWN */
	"could not set level of process", 18,	/* E_SETPROC */
	"could not get level of process", 19,	/* E_GETPROC */
	"could not get level identifiers", 20,	/* E_LVLIN */	
};

int	N_msgs = sizeof(Msgs) / sizeof(struct errmsg);	/* number of valid messages */
int	Stime;			/* sanity timer interval */
struct	sactab	*Sactab;	/* linked list head of PM info */
char	Scratch[SIZE];		/* general scratch buffer */
int	Sfd;			/* _sacpipe file descriptor */
int	Cfd;			/* command pipe file descriptor */
int	Nentries;		/* # of entries in internal version of _sactab */
