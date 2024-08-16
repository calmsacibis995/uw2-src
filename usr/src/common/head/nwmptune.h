/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nwmptune.h	1.4"
#ident "$Header: /SRCS/esmp/usr/src/nw/head/nwmptune.h,v 1.3 1994/03/18 04:28:37 jodi Exp $"

/*
**    Copyright Novell Inc. 1991
**    (C) Unpublished Copyright of Novell, Inc. All Rights Reserved.
**
**    No part of this file may be duplicated, revised, translated, localized
**    or modified in any manner or compiled, linked or uploaded or
**    downloaded to or from any computer system without the prior written
**    consent of Novell, Inc.
**
**  Netware Unix Client
**
**	  Author: Drew Spencer
**	 Created: June 4, 1990
**
**	  MODULE: nwmptune.h
**	ABSTRACT: 	Tuneable parameters for the Netware Management Portal
**				psudo-device driver.
*/

#ifndef _NWMPTUNE_H
#define _NWMPTUNE_H

/*
**	Currently, number of opens is the same as number of device
**	minors.  Allowed Opens is the parameter that should be modified
**
*/
#define NWMP_ALLOWED_OPENS	8
#define NWMP_MINOR_DEVICES	NWMP_ALLOWED_OPENS

#endif	/* _NWMPTUNE_H */
