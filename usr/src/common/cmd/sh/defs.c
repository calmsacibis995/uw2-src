/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*	Portions Copyright(c) 1988, Sun Microsystems, Inc.	*/
/*	All Rights Reserved.					*/

#ident	"@(#)sh:common/cmd/sh/defs.c	1.10.8.4"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/sh/defs.c,v 1.1 91/02/28 20:08:17 ccs Exp $"
/*
 *	UNIX shell
 */

#include 		<setjmp.h>
#include		"mode.h"
#include		"name.h"
#include		<sys/param.h>
/* temp files and io */

int				output = 2;
int				ioset;
struct ionod	*iotemp;	/* files to be deleted sometime */
struct ionod	*fiotemp;	/* function files to be deleted sometime */
struct ionod	*iopend;	/* documents waiting to be read at NL */
struct fdsave	*fdmap=NULL;

/* substitution */
int				dolc;
unsigned char			**dolv;
struct dolnod	*argfor;
struct argnod	*gchain;


/* name tree and words */
int				wdval;
int				wdnum;
int				fndef;
int				nohash;
struct argnod	*wdarg;
int				wdset;
BOOL			reserv;

/* special names */
unsigned char			*pcsadr;
unsigned char			*pidadr;
unsigned char			*cmdadr;

/* transput */ 
unsigned char 			*tmpname;
int 			serial; 
unsigned 		peekc;
unsigned		peekn;
unsigned char 			*comdiv;
long			flags;
int				rwait;	/* flags read waiting */

/* error exits from various parts of shell */
jmp_buf			subshell;
jmp_buf			errshell;

/* fault handling */
BOOL			trapnote;

/* execflgs */
int				exitval;
int				retval;
BOOL			execbrk;
int				loopcnt;
int				breakcnt;
int 			funcnt;
int				eflag;
/*
 * The following flag is set to true if /usr/ucb is found in the path
 * before /usr/bin. This value is checked when exectuing the echo and test
 * built-in commands. If true, the command behaves as in BSD systems.
 */
int				ucb_builtins;

/* The following is set to true when read -r is called. */

int			read_rflag = 0;

/* The following stuff is from stak.h	*/

unsigned char 			*stakbas;
unsigned char			*staktop;
unsigned char			*stakbot;
struct blk			*stakbsy;
unsigned char 			*brkend;

/* Runtime timeout value	*/

unsigned int	timeout;
