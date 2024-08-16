/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)truss:i386/cmd/truss/prt.c	1.1.2.1"
#ident	"$Header: prt.c 1.1 91/07/09 $"

#include <stdio.h>

#include "pcontrol.h"
#include "ramdata.h"
#include "systable.h"
#include "proto.h"
#include "machdep.h"

/* print sysi86 code */
void
prt_si86(int val, int raw)
{
	register CONST char * s = raw? NULL : si86name(val);

	if (s == NULL)
		prt_dec(val, 0);
	else
		outstring(s);
}
