/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnwutil:common/lib/libnutil/nwconfig.c	1.3"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Header: /SRCS/esmp/usr/src/nw/lib/libnutil/nwconfig.c,v 1.9 1994/09/27 21:45:30 mark Exp $"
/*
 * Copyright 1991, 1992 Unpublished Work of Novell, Inc.
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

/* nwconfig.c
 *
 *	NetWare for UNIX Configuration Manager Public Interface Layer
 *
 *	The routines in this file perform the transition from the
 *	public interface to the private (implementation specific)
 *	interface of the configuration manager.  Based on the
 *	version 0.2 specification.
 */

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <mt.h>

#include "nwconfig.h"
#include "nwcm.h"
#include "nwmsg.h"

#ifdef _REENTRANT
#include "nwutil_mt.h"

MUTEX_T	loc_config_lock;
MUTEX_T	nwcm_config_lock;
#endif /* _REENTRANT */


#ifndef NULL
#define	NULL	0
#endif

char *		NWCMConfigFilePath = NULL;
int		NWCMConfigFileLineNo = 0;
int		NWCMSystemErrno = 0;

static int	ConfigLocked = 0;


int
NWCMGetParam(char *param, enum NWCP paramType, void *data)
{
	int	cc, ret;

	/* param must be non-null */
	if (!param)
		return NWCM_NOT_FOUND;

	/* param must not be empty */
	if (!*param)
		return NWCM_NOT_FOUND;

	/* paramType must be of the well-defined types */
	if ((paramType != NWCP_INTEGER) && (paramType != NWCP_BOOLEAN)
	    && (paramType != NWCP_STRING))
		return NWCM_INVALID_TYPE;

	/* data must be at a non-zero address */
	if (!data)
		return NWCM_INVALID_DATA;

	/*
	** Depending on how we got here, this could be called recursively
	** If we are threaded, we must ensure that we only grab and release
	** the mutex lock once, the trylock does this.
	*/
	ret = MUTEX_TRYLOCK( &loc_config_lock );
	if (!ConfigLocked) {
		if (cc = _SyncConfig())
		{
			MUTEX_UNLOCK( &loc_config_lock );
			return cc;
		}
	}
	if(ret == 0)
		MUTEX_UNLOCK( &loc_config_lock );
	
	MUTEX_LOCK( &nwcm_config_lock );
	ret = _GetParam(param, paramType, data);
	MUTEX_UNLOCK( &nwcm_config_lock );

	return( ret );
}

int
NWCMGetParamDefault(char *param, enum NWCP paramType, void *data)
{
	int	cc, ret;

	/* param must be non-null */
	if (!param)
		return NWCM_NOT_FOUND;

	/* param must not be empty */
	if (!*param)
		return NWCM_NOT_FOUND;

	/* paramType must be of the well-defined types */
	if ((paramType != NWCP_INTEGER) && (paramType != NWCP_BOOLEAN)
	    && (paramType != NWCP_STRING))
		return NWCM_INVALID_TYPE;

	/* data must be at a non-zero address */
	if (!data)
		return NWCM_INVALID_DATA;

	MUTEX_LOCK( &loc_config_lock );
	if (!ConfigLocked) {
		if (cc = _SyncConfig())
		{
			MUTEX_UNLOCK( &loc_config_lock );
			return cc;
		}
	}
	MUTEX_UNLOCK( &loc_config_lock );

	MUTEX_LOCK( &nwcm_config_lock );
	ret = _GetParamDefault(param, paramType, data);
	MUTEX_UNLOCK( &nwcm_config_lock );

	return( ret );
}


int
NWCMGetParamFolder(char *param, int *folder)
{
	int	ret;

	/* param must be non-null */
	if (!param)
		return NWCM_NOT_FOUND;

	/* param must not be empty */
	if (!*param)
		return NWCM_NOT_FOUND;

	/* data must be at a non-zero address */
	if (!folder)
		return NWCM_INVALID_DATA;

	MUTEX_LOCK( &nwcm_config_lock );
	ret = _GetParamFolder(param, folder);
	MUTEX_UNLOCK( &nwcm_config_lock );

	return( ret );
}


int
NWCMGetParamDescription(char *param, char **description)
{
	int 	ret;

	/* param must be non-null */
	if (!param)
		return NWCM_NOT_FOUND;

	/* param must not be empty */
	if (!*param)
		return NWCM_NOT_FOUND;

	/* data must be at a non-zero address */
	if (!description)
		return NWCM_INVALID_DATA;

	MUTEX_LOCK( &nwcm_config_lock );
	ret = _GetParamDescription(param, description);
	MUTEX_UNLOCK( &nwcm_config_lock );

	return( ret );
}


int
NWCMGetParamHelpString(char *param, char **helpString)
{
	int 	ret;

	/* param must be non-null */
	if (!param)
		return NWCM_NOT_FOUND;

	/* param must not be empty */
	if (!*param)
		return NWCM_NOT_FOUND;

	/* data must be at a non-zero address */
	if (!helpString)
		return NWCM_INVALID_DATA;

	MUTEX_LOCK( &nwcm_config_lock );
	ret = _GetParamHelpString(param, helpString);
	MUTEX_UNLOCK( &nwcm_config_lock );

	return( ret );
}


int
NWCMCleanConfig( void )
{
	int	cc, ret;

	MUTEX_LOCK( &loc_config_lock );
	if (!ConfigLocked) {
		if (cc = _LockAndSyncConfig())
		{
			MUTEX_UNLOCK( &loc_config_lock );
			return(cc);
		}
	}

	MUTEX_LOCK( &nwcm_config_lock );
	if((ret = _CleanConfig()) != 0) {
		MUTEX_UNLOCK( &nwcm_config_lock );
		MUTEX_UNLOCK( &loc_config_lock );
		return(ret);
	}

	ret = _UpdateAndUnlockConfig();

	MUTEX_UNLOCK( &nwcm_config_lock );
	MUTEX_UNLOCK( &loc_config_lock );

	return( ret );
}

int
NWCMSetParam(char *param, enum NWCP paramType, void *data)
{
	int	cc, ret;

	/* param must be non-null */
	if (!param)
		return NWCM_NOT_FOUND;

	/* param must not be empty */
	if (!*param)
		return NWCM_NOT_FOUND;

	/* paramType must be of the well-defined types */
	if ((paramType != NWCP_INTEGER) && (paramType != NWCP_BOOLEAN)
	    && (paramType != NWCP_STRING))
		return NWCM_INVALID_TYPE;

	/* data must be non-null */
	if (!data)
		return NWCM_INVALID_DATA;

	MUTEX_LOCK( &loc_config_lock );
	if (!ConfigLocked) {
		if (cc = _LockAndSyncConfig())
		{
			MUTEX_UNLOCK( &loc_config_lock );
			return cc;
		}
	}

	MUTEX_LOCK( &nwcm_config_lock );
	if (cc = _SetParam(param, paramType, data))
	{
		MUTEX_UNLOCK( &nwcm_config_lock );
		MUTEX_UNLOCK( &loc_config_lock );
		return cc;
	}

	ret = _UpdateAndUnlockConfig();

	MUTEX_UNLOCK( &nwcm_config_lock );
	MUTEX_UNLOCK( &loc_config_lock );

	return( ret );
}


int
NWCMSetToDefault(char *param)
{
	int	cc, ret;

	/* param must be non-null */
	if (!param)
		return NWCM_NOT_FOUND;

	/* param must not be empty */
	if (!*param)
		return NWCM_NOT_FOUND;

	if (!ConfigLocked) {
		if (cc = _LockAndSyncConfig())
			return cc;
	}
	MUTEX_LOCK( &nwcm_config_lock );
	if(cc = _SetToDefault(param))
	{
		MUTEX_UNLOCK( &nwcm_config_lock );
		return cc;
	}

	
	MUTEX_LOCK( &loc_config_lock );
	if (!ConfigLocked)
		ret = _UpdateAndUnlockConfig();

	MUTEX_UNLOCK( &loc_config_lock );
	MUTEX_UNLOCK( &nwcm_config_lock );

	return( ret );
}


int
NWCMValidateParam(char *param, enum NWCP paramType, void *data)
{
	int 	ret;

	/* param must be non-null */
	if (!param)
		return NWCM_NOT_FOUND;

	/* param must not be empty */
	if (!*param)
		return NWCM_NOT_FOUND;

	/* paramType must be of the well-defined types */
	if ((paramType != NWCP_INTEGER) && (paramType != NWCP_BOOLEAN)
	    && (paramType != NWCP_STRING))
		return NWCM_INVALID_TYPE;

	/* data must be non-null */
	if (!data)
		return NWCM_INVALID_DATA;

	MUTEX_LOCK( &nwcm_config_lock );
	ret = _ValidateParam(param, paramType, data);
	MUTEX_UNLOCK( &nwcm_config_lock );

	return( ret );
}


int
NWCMLockConfig(void)
{
	int	cc;

	MUTEX_LOCK( &loc_config_lock );
	if (ConfigLocked)
	{
		MUTEX_UNLOCK( &loc_config_lock );
		return NWCM_SUCCESS;
	}

	MUTEX_LOCK( &nwcm_config_lock );
	if (cc = _LockAndSyncConfig())
	{
		MUTEX_UNLOCK( &loc_config_lock );
		MUTEX_UNLOCK( &nwcm_config_lock );
		return cc;
	}
	MUTEX_UNLOCK( &nwcm_config_lock );
	ConfigLocked++;

	MUTEX_UNLOCK( &loc_config_lock );
	return NWCM_SUCCESS;
}


int
NWCMUnlockConfig(void)
{
	int	cc;

	MUTEX_LOCK( &loc_config_lock );
	if (!ConfigLocked)
	{
		MUTEX_UNLOCK( &loc_config_lock );
		return NWCM_SUCCESS;
	}

	MUTEX_LOCK( &nwcm_config_lock );
	if (cc = _UpdateAndUnlockConfig())
	{
		MUTEX_UNLOCK( &loc_config_lock );
		MUTEX_UNLOCK( &nwcm_config_lock );
		return cc;
	}
	MUTEX_UNLOCK( &nwcm_config_lock );
	ConfigLocked = 0;

	MUTEX_UNLOCK( &loc_config_lock );
	return NWCM_SUCCESS;
}


int
NWCMFlush(void)
{
	int 	ret;

	MUTEX_LOCK( &nwcm_config_lock );
	ret = _ConfigFlush();
	MUTEX_UNLOCK( &nwcm_config_lock );

	return( ret );
}


const char *
NWCMGetConfigFilePath(void)
{
	static char *	path = NULL;

	if (path)
		return (const char *) path;

	MUTEX_LOCK( &loc_config_lock );
	MUTEX_LOCK( &nwcm_config_lock );
	if (_ProbeConfigInit())
	{
		MUTEX_UNLOCK( &loc_config_lock );
		MUTEX_UNLOCK( &nwcm_config_lock );
		return NULL;
	}
	MUTEX_UNLOCK( &nwcm_config_lock );

	if (!*NWCMConfigFilePath)
	{
		MUTEX_UNLOCK( &loc_config_lock );
		return NULL;
	}

	path = (char *)strdup(NWCMConfigFilePath);

	MUTEX_UNLOCK( &loc_config_lock );
	return (const char *) path;
}


const char *
NWCMGetConfigDirPath(void)
{
	static char *	path = NULL;
	char *		cp;

	if (path)
		return (const char *) path;

	if ((cp = (char *) NWCMGetConfigFilePath()) == NULL)
		return NULL;

	if ((path = (char *)strdup(cp)) == NULL)
		return NULL;

	if (cp = strrchr(path, '/')) {
		*cp = 0;
	} else {
		strcpy(path, ".");
	}

	return (const char *) path;
}


int
NWCMPerror(int cmerr, char * format, ...)
{
	va_list		ap;
	int			cc;
	static int	bound = 0;

	if( bound == 0) {
		cc = MsgBindDomain(MSG_DOMAIN_NWCM, MSG_DOMAIN_UTIL_FILE, MSG_UTIL_REV_STR);
		if(cc != NWCM_SUCCESS) {
			return cc;
		}
		bound++;
	}

	va_start(ap, format);
	cc = vfprintf(stderr, format, ap);
	va_end(ap);

	cc |= fprintf(stderr, "%s\n", MsgDomainGetStr(MSG_DOMAIN_NWCM, cmerr));

	return cc;
}
