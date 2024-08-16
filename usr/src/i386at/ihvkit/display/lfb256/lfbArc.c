/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ihvkit:display/lfb256/lfbArc.c	1.1"

/*
 * Copyright (c) 1990, 1991, 1992, 1993 UNIX System Laboratories, Inc.
 *	Copyright (c) 1988, 1989, 1990 AT&T
 *	Copyright (c) 1993  Intel Corporation
 *	  All Rights Reserved
 */

#include <lfb.h>

/*	HARDWARE DRAW ARC ROUTINE	*/
/*		OPTIONAL		*/

SIvoid lfbDrawArcClip(x1, y1, x2, y2)
SIint32 x1, y1, x2, y2;
{
}

SIBool lfbDrawArc(x, y, w, h, arc1, arc2)
SIint32 x, y, w, h, arc1, arc2;
{
    return(SI_FAIL);
}


/*	HARDWARE FILL ARC ROUTINE	*/
/*		OPTIONAL		*/

SIvoid lfbFillArcClip(x1, y1, x2, y2)
SIint32 x1, y1, x2, y2;
{
}

SIBool lfbFillArc(x, y, w, h, arc1, arc2)
SIint32 x, y, w, h, arc1, arc2;
{
    return(SI_FAIL);
}
