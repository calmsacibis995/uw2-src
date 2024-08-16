/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)cs:cs/global.c	1.6.2.4"
#ident	"$Header: $"

#include <dial.h>
#include <netconfig.h>
#include <netdir.h>
#include <cs.h>

char	Scratch[MSGSIZE];	/* general scratch buffer */
char	msg[MSGSIZE];		/* general debugging message buffer */
CALL	Call;
CALL	*Callp=&Call;
struct	nd_hostserv	Nd_hostserv;
int	Debugging=0;
int	netfd;		/* fd into the network */
int	returnfd;	/* authenticated fd to return */
int	Pid;		/* pid of the dial request client	*/
