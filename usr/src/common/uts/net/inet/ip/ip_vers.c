/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/inet/ip/ip_vers.c	1.5"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *	STREAMware TCP/IP Release 1.0
 *
 *	Copyrighted as an unpublished work.
 *	(c) Copyright 1990 INTERACTIVE Systems Corporation
 *	All Rights Reserved.
 *
 *	Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *	All Rights Reserved.
 *
 *	The copyright above and this notice must be preserved in all
 *	copies of this source code.  The copyright above does not
 *	evidence any actual or intended publication of this source
 *	code.
 *
 *	This is unpublished proprietary trade secret source code of
 *	Lachman Associates.  This source code may not be copied,
 *	disclosed, distributed, demonstrated or licensed except as
 *	expressly authorized by Lachman Associates.
 *
 *	System V STREAMS TCP was jointly developed by Lachman
 *	Associates and Convergent Technologies.
 */

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *		PROPRIETARY NOTICE (Combined)
 *
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 *
 *
 *
 *		Copyright Notice
 *
 * Notice of copyright on this source code product does not indicate
 * publication.
 *
 *	(c) 1986,1987,1988.1989,1990  Sun Microsystems, Inc
 *	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *		  All rights reserved.
 *
 */

#include <util/cmn_err.h>

STATIC char ip_rel[] = "Release 1.0";
STATIC char isc_ip_rel[] = "Release 4.1";

/*
 * void ipversion(void)
 *	Display networking code copyright message for everyone
 *	that ever put their hands in this thing.
 *
 * Calling/Exit State:
 *	Locking:
 *	  No locking requirements.
 */
void
ipversion(void)
{
	cmn_err(CE_CONT, "^System V STREAMS TCP %s\n", ip_rel);
	cmn_err(CE_CONT,
		"^(c) 1983,1984,1985,1986,1987,1988,1989,1990 AT&T.\n");
	cmn_err(CE_CONT, "^(c) 1986,1987,1988,1989,1990 Sun Microsystems.\n");
	cmn_err(CE_CONT,
		"^(c) 1987,1988,1989 Lachman Associates, Inc. (LAI).\n");
	cmn_err(CE_CONT, "^	 All Rights Reserved.\n");
	cmn_err(CE_CONT, "^STREAMWare TCP/IP %s\n", isc_ip_rel);
	cmn_err(CE_CONT,
	       "^Copyright 1987-1993 INTERACTIVE Systems Corporation.\n");
	cmn_err(CE_CONT,
	       "^Copyright 1987-1993 Lachman Technology, Inc..\n");
	cmn_err(CE_CONT, "^	 All Rights Reserved.\n");
}
