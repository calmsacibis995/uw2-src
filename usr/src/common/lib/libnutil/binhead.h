/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: binhead.h,v 1.6.2.1 1994/10/19 18:23:37 vtag Exp $"
#ident	"@(#)libnwutil:common/lib/libnutil/binhead.h	1.6"
/*
 * Copyright 1994 Unpublished Work of Novell, Inc.
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

#include "nwmsg.h"

/*
**	Binary File format structures
*/

typedef struct binHeader {
	unsigned long	checksum;
	char		libName[15];	/* Max. System V name length */
	int 		messDomain;
	char		domainFile[15];
	char		domainRevStr[NWCM_MAX_STRING_SIZE];
	int 		numIntParams;
	int 		numBoolParams;
	int 		numStringParams;
} binHeader_t;

typedef struct binRecord {
	char			paramName[NWCM_MAX_STRING_SIZE];
	int 			folder;
	int 			description;
	int 			helpString;
	int 			dataType;
	char			validName[NWCM_MAX_STRING_SIZE];
	unsigned long	lowLimit;
	unsigned long	highLimit;
	int 			displayFormat;
	char			stringDefault[NWCM_MAX_STRING_SIZE];
	int 			boolDefault;
	unsigned long	intDefault;
} binRecord_t;

typedef struct configFile {
	void	*dlHandle;
} cFTab;
