/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma	ident	"@(#)dtadmin:dtamlib/owner.c	1.7"
#endif
/*
 *	owner.c:	validate owner privileges
 *
 */
#include <stdio.h>
#include <string.h>
#include <X11/Intrinsic.h>
#include "owner.h"


Boolean	_DtamIsOwner(char *adm_name)
{
	char	buf[BUFSIZ];

	sprintf(buf, "/sbin/tfadmin -t %s 2>/dev/null", adm_name);
	return (system(buf)==0);
}
