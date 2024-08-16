/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ucb:common/ucbcmd/lp/lpc/lpc.h	1.2"
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
 *
 */

/*
 * Line printer control program.
 */
struct	cmd {
	char	*c_name;		/* command name */
	char	*c_help;		/* help message */
	void	(*c_handler)();		/* routine to do the work */
	int	c_priv;			/* privileged command */
};

#if defined(__STDC__)
char *	get_reason(int, char **);
int	topq_reqid(char *, char *);
int	topq_user(char *, char *);
void	_abort(int, char **);
void	clean(int, char **);
void	cleanpr(char *);
void	disable(int, char **);
void	disablepr(char *);
void	disableq(char *);
void	do_all(void (*)(char *));
void	down(int, char **);
void	downpr(char *);
void	enable(int, char **);
void	enablepr(char *);
void	enableq(char *);
void	help(int, char **);
void	quit(int, char **);
void	restart(int, char **);
void	restartpr(char *);
void	start(int, char **);
void	status(int, char **);
void	statuspr(char *);
void	stop(int, char **);
void	topq(int, char **);
void	up(int, char **);
void	uppr(char *);
#else
char *	get_reason();
int	topq_reqid();
int	topq_user();
void	_abort();
void	clean();
void	cleanpr();
void	disable();
void	disablepr();
void	disableq();
void	do_all();
void	down();
void	downpr();
void	enable();
void	enablepr();
void	enableq();
void	help();
void	quit();
void	restart();
void	restartpr();
void	start();
void	status();
void	statuspr();
void	stop();
void	topq();
void	up();
void	uppr();
#endif
