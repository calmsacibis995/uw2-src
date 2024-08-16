/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:cmd/lpadmin/ismodel.c	1.4.4.3"
#ident	"$Header: $"

#include "lp.h"
#include "lpadmin.h"

extern int		Access();

int			ismodel (name)
	char			*name;
{
	if (!name || !*name)
		return (0);

	return (Access(makepath(Lp_Model, name, (char *)0), 04) != -1);
}
