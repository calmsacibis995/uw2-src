/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnwutil:common/lib/libnutil/nwcm/nwnet/arch_val.c	1.1"

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
 *	NetWare for UNIX Configuration Manager
 *
 *	Architecture specific validation functions for the configuration manager.
 */


#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/nwportable.h>

/*
 * Validate a router_type
*/
NWCMValidateRouterType(char * value)
{
	ConvertToUpper(value);

	/* has to be FULL or CLIENT */
	if( ( strcmp("FULL", value) != 0) && 
		( strcmp("CLIENT", value) != 0))
		return 0;

	return 1;
}

int
NWCMValidateIFName(char * value)
{
	/*XXX*/
	return 1;
}

int
NWCMValidateDevice(char * value)
{
	/*XXX*/
	return 1;
}

int
NWCMValidateDeviceType(char * value)
{
	if( strcmp("ETHERNET_DLPI", value) == 0)
		return 1;
	if( strcmp("TOKEN-RING_DLPI", value) == 0)
		return 1;
	return 0;
}

int
NWCMValidateFrameType(char * value)
{
	if( strcmp("ETHERNET_II", value) == 0)
		return 1;
	if( strcmp("ETHERNET_802.3", value) == 0)
		return 1;
	if( strcmp("ETHERNET_802.2", value) == 0)
		return 1;
	if( strcmp("ETHERNET_SNAP", value) == 0)
		return 1;
	if( strcmp("TOKEN-RING_SNAP", value) == 0)
		return 1;
	if( strcmp("TOKEN-RING", value) == 0)
		return 1;
	return 0;
}
