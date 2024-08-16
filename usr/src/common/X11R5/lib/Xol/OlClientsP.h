/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)olmisc:OlClientsP.h	1.4"

#ifndef _OlClientsP_h
#define _OlClientsP_h

#include <Xol/OlClients.h>

#ifdef DELIMITER
#undef DELIMITER
#endif

#define DELIMITER	0x1f

#define DEF_STRING(s,d)	        (s == NULL ? d : s)
#define NULL_DEF_STRING(s)      DEF_STRING(s,"")

#endif
