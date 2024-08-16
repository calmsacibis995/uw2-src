/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nwcm.h	1.4"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Header: /SRCS/esmp/usr/src/nw/head/nwcm.h,v 1.6 1994/08/30 15:36:43 mark Exp $"
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

#ifndef __NWCM_H__
#define __NWCM_H__

/* nwcm.h
 *
 *	Private Interface to the NetWare for UNIX Configuration Manager
 *
 *	This file should only be included by the files containing the
 *	implemetation of the configuration manager, and by the public
 *	interface module, which performs the transition from the public
 *	to the private interface.
 */

#ifdef NWCM_FRIEND

#include <nwconfig.h>

#ifdef __STDC__
extern int	_CleanConfig(void);
extern int	_GetParam(char *, enum NWCP, void *);
extern int 	_GetParamDefault(char *, enum NWCP, void *);
extern int	_GetParamFolder(char *, int *);
extern int	_GetParamDescription(char *, char **);
extern int	_GetParamHelpString(char *, char **);
extern int	_SetParam(char *, enum NWCP, void *);
extern int	_SetToDefault(char *);
extern int	_ValidateParam(char *, enum NWCP, void *);
extern int	_LockAndSyncConfig(void);
extern int	_SyncConfig(void);
extern int	_UpdateAndUnlockConfig(void);
extern int	_ConfigFlush(void);
extern int	_ProbeConfigInit(void);
#else /* __STDC__ */
extern int	_CleanConfig();
extern int	_GetParam();
extern int	_GetParamDefault();
extern int	_GetParamFolder();
extern int	_GetParamDescription();
extern int	_GetParamHelpString();
extern int	_SetParam();
extern int	_SetToDefault();
extern int	_ValidateParam();
extern int	_LockAndSyncConfig();
extern int	_SyncConfig();
extern int	_UpdateAndUnlockConfig();
extern int	_ConfigFlush();
extern int	_ProbeConfigInit();
#endif /* __STDC__ */

#ifdef	NWCM_SCHEMA_FRIEND

#include "schemadef.h"

#ifdef __STDC__
extern int	_LookUpParameter(char *, enum NWCP, struct cp_s *);
#else /* __STDC__ */
extern int	_LookUpParameter();
#endif /* __STDC__ */

#endif /* NWCM_SCHEMA_FRIEND */

#endif /* NWCM_FRIEND */

#endif /* __NWCM_H__ */
