/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/* Copyright (c) 1981 Regents of the University of California */
#ident	"@(#)vi:port/ex_re.h	1.6.1.5"
#ident  "$Header: ex_re.h 1.2 91/06/27 $"

#include <regex.h>

struct	regexp {
	regex_t	re;
	int	status;	/* 0:unset; otherwise <0:is ^-anchored */
};

var struct	regexp	*curre;			/* current re to use */
var struct	regexp	scanre;			/* last scanning re */
var struct	regexp	subre;			/* last substitution re */
var unsigned char	rhsbuf[RHSSIZE];	/* rhs of last substitute */

var char	*loc1, *loc2;	/* start and just after end of match */
