/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)ttymon:common/cmd/ttymon/ttymon.notp/stty.h	1.1"

#define ASYNC	1
#define FLOW	2
#define WINDOW	4
#define TERMIOS 8

struct	speeds {
	const char	*string;
	int	speed;
};

struct mds {
	const char	*string;
	long	set;
	long	reset;
};

