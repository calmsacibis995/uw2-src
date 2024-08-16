/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwClnt:headers/nwapidef.h	1.2"
/*
** Copyright 1989, 1991 Novell, Inc. All Rights Reserved.
**
** THIS WORK IS SUBJECT TO U.S. AND INTERNATIONAL COPYRIGHT LAWS AND
** TREATIES.  NO PART OF THIS WORK MAY BE USED, PRACTICED, PERFORMED,
** COPIED, DISTRIBUTED, REVISED, MODIFIED, TRANSLATED, ABRIDGED,
** CONDENSED, EXPANDED, COLLECTED, COMPILED, LINKED, RECAST,
** TRANSFORMED OR ADAPTED WITHOUT THE PRIOR WRITTEN CONSENT
** OF NOVELL.  ANY USE OR EXPLOITATION OF THIS WORK WITHOUT
** AUTHORIZATION COULD SUBJECT THE PERPETRATOR TO CRIMINAL AND
** CIVIL LIABILITY.
**
**    @(#)$Header: /SRCS/esmp/usr/src/nw/lib/libnwClnt/headers/nwapidef.h,v 1.2 1994/04/20 15:30:07 jodi Exp $
**/

/* 
** This header file contains the definitions used throughout the API's
*/
#ifndef __NWAPIDEF_H__
#define __NWAPIDEF_H__

#ifndef NULL
#define		NULL		0
#endif

#ifndef 	TRUE
#define		TRUE		1
#define		FALSE		0
#endif

#define	NWMAX_OBJECT_NAME_LENGTH		48
#define NWMAX_SERVER_NAME_LENGTH		48
#define NWMAX_PASSWORD_LENGTH			128
#define NWMAX_INTERNET_ADDRESS_LENGTH	12 

#endif /* __NWAPIDEF_H__ */
