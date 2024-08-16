/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnwutil:common/lib/libnutil/nwcm/nuc/nuc_val.c	1.1"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:lib/libnwcm/global_val.c	1.2"
#ident	"$Id: nuc_val.c,v 1.1 1994/08/30 16:21:34 mark Exp $"

/*
 * Copyright 1992 Unpublished Work of Novell, Inc.
 * All Rights Reserved.
 *
 * This work is an unpublished work and contains confidential,
 * proprietary and trade secret information of Novell, Inc. Access
 * to this work is restricted to (I) Novell employees who have a
 * need to know to perform tasks within the scope of their
 * assignments and (II) entities other than Novell who have
 * entered into appropriate agreements.
 *
 * No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 */

/*
 * nuc_val.c
 *
 *	NetWare for UNIX Configuration Manager
 *
 *	NUC validation functions for the configuration manager.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <string.h>
#include <errno.h>

#include <sys/nwportable.h>

/*
**	login uses /etc/.slogin to determine if single login is enabled.  It
**	probably should use the nwcm parameter, but at the present time does not.
**	Until the time comes that we have a single indicator of the state of
**	single login, we must create and delete the file as well as set the nwcm
**	parameter.  Since a boolean does not have a validation function, we
**	must use type string and fake a boolean, using value active or inactive.
**/
NWCMValidateSingleLogin(int value)
{
	struct stat file_buf;
	int	code;

	if( value ) {
		/* Create file, turn on single login */
		if( stat ("/etc/.slogin", &file_buf) == (-1)) {
			code = system ("/usr/bin/touch /etc/.slogin 2>/dev/null");
		} else {
			code = 0;
		}
	} else {
		/* remove file, turn off single login */
		if( stat ("/etc/.slogin", &file_buf) > (-1)) {
			code = system ("/usr/bin/rm -f /etc/.slogin 2>/dev/null");
		} else {
			code = 0;
		}
	}

	if( code != 0)
		return 1;
	return 0;
}
