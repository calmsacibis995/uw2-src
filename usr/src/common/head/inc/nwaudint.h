/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/nwaudint.h	1.1"
#ifndef AUDIT_KEY
#define AUDIT_KEY 0xFFFFFFDCL

#include <npackon.h>

#define ERROR_PASSWORD_INVALID 222
#define FLAGS 3
#define PASSWORD_LENGTH 16

typedef struct NWAduditKey
{
   nuint32 signature;
   nuint8  flags[4];
   nuint8  cryptedPassword[16];
   nuint8  cryptedPassword2[16];
} KEY;

#include <npackoff.h>

#endif

