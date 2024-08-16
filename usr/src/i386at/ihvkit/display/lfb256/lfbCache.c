/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ihvkit:display/lfb256/lfbCache.c	1.1"

/*
 * Copyright (c) 1990, 1991, 1992, 1993 UNIX System Laboratories, Inc.
 *	Copyright (c) 1988, 1989, 1990 AT&T
 *	Copyright (c) 1993  Intel Corporation
 *	  All Rights Reserved
 */

#include <lfb.h>

/*	SDD MEMORY CACHING CONTROL	*/
/*		OPTIONAL		*/

SIBool lfbAllocCache(buf, type)
SIbitmapP buf;
SIint32 type;
{
    return(SI_FAIL);
}

SIBool lfbFreeCache(buf)
SIbitmapP buf;
{
    return(SI_FAIL);
}

SIBool lfbLockCache(buf)
SIbitmapP buf;
{
    return(SI_FAIL);
}

SIBool lfbUnlockCache(buf)
SIbitmapP buf;
{
    return(SI_FAIL);
}
