/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnwutil:common/lib/libnutil/nwcm.c	1.6"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Header: /SRCS/esmp/usr/src/nw/lib/libnutil/nwcm.c,v 1.13.4.1 1994/12/21 18:29:30 mark Exp $"
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
 * nwcm.c
 *
 *	NetWare for UNIX Configuration Manager
 *
 *	The public routines in this module should only be called by
 *	the configuration manager public interface layer.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include "nwenv.h"
#include "nwmsg.h"
#include <util_proto.h>

extern int	errno;

#include "nwconfig.h"

#define	NWCM_SCHEMA_FRIEND
#include "nwcm.h"

#define	NWCONFIG_MODES	0644		/* creat modes */

static int	ConfigFd = -1;
static int	ConfigFileLocked = 0;
static int	ConfigReadOnly = 0;
static time_t	ConfigLastSync = 0;
int		ConfigDirty = 0;	/* global because used in parser */

static int	_InitConfigManager(void);
static int	_ParseConfigFile(void);
static int	_WriteConfigFile(void);
static int	_GetDomainNumber(int);

extern int	InitNWCM(char *);
extern int	BindInstalledDomains(void);

static struct flock	lck = { 0, SEEK_SET, 0, 0 };

static int	NWCMINIT=FALSE;

#define	CHECKINIT()	(ConfigFd != -1)
#define	INIT()		{ \
				int _cc; \
				if (!CHECKINIT()) { \
					if (_cc = _InitConfigManager()) \
						return _cc; \
				} \
			}

/*
 * int strncmpi(s1, s2)
 *
 *	Like strncmp(), but case insensitive.
 */

int
strncmpi(char * s1, char * s2, int len)
{
	int	r;

	while (*s1 && *s2 && len--)
		if ((r = (int) (toupper(*s1++) - toupper(*s2++))) != 0)
			return r;
	return (int) (toupper(*s1) - toupper(*s2));
}

int
_GetParam(char *name, enum NWCP type, void *data)
{
	struct cp_s	param;
	int		ccode;

	if (!CHECKINIT())
		return NWCM_NOT_INITIALIZED;

	if (ccode = _LookUpParameter(name, type, &param))
		return ccode;

	switch (param.type) {
	case NWCP_INTEGER:
		*((unsigned long *) data) = *((unsigned long *) param.cur_val);
		break;

	case NWCP_BOOLEAN:
		*((int *) data) = *((int *) param.cur_val);
		break;

	case NWCP_STRING:
		(void) strncpy((char *) data, (char *) param.cur_val,
		    NWCM_MAX_STRING_SIZE);
		break;

	default:
		return NWCM_INVALID_TYPE;
	}

	return NWCM_SUCCESS;
}

int
_GetParamDefault(char *name, enum NWCP type, void *data)
{
	struct cp_s	param;
	int		ccode;

	if (!CHECKINIT())
		return NWCM_NOT_INITIALIZED;

	if (ccode = _LookUpParameter(name, type, &param))
		return ccode;

	switch (param.type) {
	case NWCP_INTEGER:
		*((unsigned long *) data) = *((unsigned long *) param.def_val);
		break;

	case NWCP_BOOLEAN:
		*((int *) data) = *((int *) param.def_val);
		break;

	case NWCP_STRING:
		(void) strncpy((char *) data, (char *) param.def_val,
		    NWCM_MAX_STRING_SIZE);
		break;

	default:
		return NWCM_INVALID_TYPE;
	}

	return NWCM_SUCCESS;
}


int
_GetParamFolder(char *name, int *folder)
{
	struct cp_s	param;
	int		ccode;
	static  int foldBound = 0;

	*folder = NWCM_PF_NONE;

	if (!CHECKINIT())
		return NWCM_NOT_INITIALIZED;

	if (ccode = _LookUpParameter(name, NWCP_UNDEFINED, &param))
		return ccode;

	if( foldBound == 0) {
		(void) MsgBindDomain(MSG_DOMAIN_NWCM_FOLD, MSG_DOMAIN_NWCM_FILE,
			MSG_NWCM_REV_STR);
		foldBound++;
	}

	*folder = param.folder;

	return NWCM_SUCCESS;
}

int
_GetParamDescription(char *name, char **description)
{
	struct cp_s	param;
	int		ccode;
	int 	domain;

	*description = NULL;

	if (!CHECKINIT())
		return NWCM_NOT_INITIALIZED;

	if (ccode = _LookUpParameter(name, NWCP_UNDEFINED, &param))
		return ccode;

	if((domain = _GetDomainNumber(param.folder)) < 0)
		return(NWCM_INVALID_TYPE);

	*description = MsgDomainGetStr(domain, param.description);

	return NWCM_SUCCESS;
}

int
_GetParamHelpString(char *name, char **helpString)
{
	struct cp_s	param;
	int		ccode;
	int 	domain;

	*helpString = NULL;

	if (!CHECKINIT())
		return NWCM_NOT_INITIALIZED;

	if (ccode = _LookUpParameter(name, NWCP_UNDEFINED, &param))
		return ccode;

	if((domain = _GetDomainNumber(param.folder)) < 0)
		return(NWCM_INVALID_TYPE);

	*helpString = MsgDomainGetStr(domain, param.helpString);

	return NWCM_SUCCESS;
}

static int
_GetDomainNumber(int folder)
{
	switch(folder)
	{
	case NWCM_PF_NONE:
	case NWCM_PF_IPXSPX:
	case NWCM_PF_SAP:
	case NWCM_PF_NVT:
	case NWCM_PF_DEFAULT:
		return(MSG_DOMAIN_NWCM_DH);

	case NWCM_PF_NUC:
		return(MSG_DOMAIN_NUC_DH);

	case NWCM_PF_NPRINTER:
		return(MSG_DOMAIN_NPRINT_DH);

	case NWCM_PF_NWUM:
		return(MSG_DOMAIN_NWUMD_DH);

	case NWCM_PF_PSERVER:
	case NWCM_PF_ATPS:
		return(MSG_DOMAIN_PSERVER_DH);

	case NWCM_PF_NDS:
	case NWCM_PF_TS:
		return(MSG_DOMAIN_NDS_DH);

	case NWCM_PF_GENERAL:
	case NWCM_PF_AFP:
	case NWCM_PF_APPLETALK:
	case NWCM_PF_SYSTUNE:
	case NWCM_PF_LOCALE:
		return(MSG_DOMAIN_NWU_DH);

	default:
		return(-1);
	}
}

int
_CleanConfig(void)
{
	if (ConfigReadOnly)
		return NWCM_CONFIG_READ_ONLY;

	ConfigDirty++;
	return(0);
}

int
_SetParam(char *name, enum NWCP type, void *data)
{
	struct cp_s	param;
	int		ccode;
	unsigned long	ldata;
	int		bdata;
	char		sbuf[NWCM_MAX_STRING_SIZE];
	char *		sdata;

	if (!CHECKINIT())
		return NWCM_NOT_INITIALIZED;

	if (ConfigReadOnly)
		return NWCM_CONFIG_READ_ONLY;

	if (!ConfigFileLocked)
		return NWCM_CONFIG_NOT_LOCKED;

	if (ccode = _LookUpParameter(name, type, &param))
		return ccode;

	switch (param.type) {
	case NWCP_INTEGER:
		ldata = *((unsigned long *) data);
		if (param.validation.data) {
			if (param.validation.data->func) {
				if (!param.validation.data->func(ldata)) {
					return NWCM_INVALID_DATA;
				}
			} else {
				if ((ldata < param.validation.data->min) ||
				    (ldata > param.validation.data->max)) {
					return NWCM_INVALID_DATA;
				}
			}
		}
		if (ldata != *((unsigned long *) param.cur_val)) {
			*((unsigned long *) param.cur_val) = ldata;
			ConfigDirty++;
		}
		break;

	case NWCP_BOOLEAN:
		bdata = *((int *) data);
		if (bdata != *((int *) param.cur_val)) {
			*((int *) param.cur_val) = bdata;
			ConfigDirty++;
		}
		break;

	case NWCP_STRING:
		if (param.format == df_uppercase) {
			/*
			 * Source string may be in a text (read-only)
			 * segment, so copy into a buffer before munging.
			 */
			(void) strncpy(sbuf, (char *) data,
			    NWCM_MAX_STRING_SIZE);
			sdata = sbuf;
			ConvertToUpper(sdata);
		} else {
			sdata = (char *) data;
		}
		if (param.validation.func) {
			if (!param.validation.func(sdata)) {
				return NWCM_INVALID_DATA;
			}
		}
		if (strcmp(sdata, (char *) param.cur_val)) {
			(void) strncpy((char *) param.cur_val, sdata,
			    NWCM_MAX_STRING_SIZE);
			ConfigDirty++;
		}
		break;

	default:
		return NWCM_INVALID_TYPE;
	}

	return NWCM_SUCCESS;
}

int
_SetToDefault(char *name)
{
	struct cp_s	param;
	int		ccode;
	int 	bdata;

	if (!CHECKINIT())
		return NWCM_NOT_INITIALIZED;

	if (ConfigReadOnly)
		return NWCM_CONFIG_READ_ONLY;

	if (!ConfigFileLocked)
		return NWCM_CONFIG_NOT_LOCKED;

	if (ccode = _LookUpParameter(name, NWCP_UNDEFINED, &param))
		return ccode;

	switch (param.type) {
	case NWCP_INTEGER:
		*((unsigned long *) param.cur_val) =
		    *((unsigned long *) param.def_val);
		ConfigDirty++;
		break;

	case NWCP_BOOLEAN:
		bdata = *((int *)param.cur_val);
		if(bdata != *((int *) param.def_val))
		{
			*((int *) param.cur_val) = *((int *) param.def_val);
			ConfigDirty++;
		}
		break;

	case NWCP_STRING:
		(void) strncpy((char *) param.cur_val, (char *) param.def_val,
		    NWCM_MAX_STRING_SIZE);
		if (param.format == df_uppercase)
			ConvertToUpper((char *) param.cur_val);
		ConfigDirty++;
		break;

	default:
		return NWCM_INVALID_TYPE;
	}

	return NWCM_SUCCESS;
}


int
_ValidateParam(char *name, enum NWCP type, void *data)
{
	struct cp_s	param;
	int		ccode;
	unsigned long	ldata;
	char		sbuf[NWCM_MAX_STRING_SIZE];
	char *		sdata;

	if (!CHECKINIT())
		return NWCM_NOT_INITIALIZED;

	if (ccode = _LookUpParameter(name, type, &param))
		return ccode;

	switch (param.type) {
	case NWCP_INTEGER:
		ldata = *((unsigned long *) data);
		if (param.validation.data) {
			if (param.validation.data->func) {
				if (!param.validation.data->func(ldata)) {
					return NWCM_INVALID_DATA;
				}
			} else {
				if ((ldata < param.validation.data->min) ||
				    (ldata > param.validation.data->max)) {
					return NWCM_INVALID_DATA;
				}
			}
		}
		break;

	case NWCP_BOOLEAN:
		break;

	case NWCP_STRING:
		if (param.format == df_uppercase) {
			/*
			 * Source string may be in a text (read-only)
			 * segment, so copy into a buffer before munging.
			 */
			(void) strncpy(sbuf, (char *) data,
			    NWCM_MAX_STRING_SIZE);
			sdata = sbuf;
			ConvertToUpper(sdata);
		} else {
			sdata = (char *) data;
		}
		if (param.validation.func) {
			if (!param.validation.func(sdata)) {
				return NWCM_INVALID_DATA;
			}
		}
		break;

	default:
		return NWCM_INVALID_TYPE;
	}

	return NWCM_SUCCESS;
}

int
_LockAndSyncConfig(void)
{
	if (ConfigFileLocked)
		return NWCM_SUCCESS;

	INIT();

	if (ConfigReadOnly)
		return NWCM_CONFIG_READ_ONLY;

	lck.l_type = F_WRLCK;
	if (fcntl(ConfigFd, F_SETLKW, &lck) < 0) {
		NWCMSystemErrno = errno;
		return NWCM_LOCK_FAILED;
	}

	ConfigFileLocked++;

	return _SyncConfig();
}

int
_SyncConfig(void)
{
	int		i, ccode;
	struct stat	st;


	INIT();

	if (fstat(ConfigFd, &st) < 0) {
		NWCMSystemErrno = errno;
		return NWCM_SYSTEM_ERROR;
	}

	if (st.st_mtime <= ConfigLastSync)
		return NWCM_SUCCESS;

	if (!ConfigFileLocked) {
		lck.l_type = F_RDLCK;
		(void) fcntl(ConfigFd, F_SETLK, &lck);
	}

/*
**	New code to set to an initial default state all parameters
**	before syncing.
*/
	for(i=0; i<ConfigurationParameterCount; i++)
		(void)_SetToDefault(ConfigurationParameters[i].name);

	ConfigDirty=0;

	if (ccode = _ParseConfigFile())
		return ccode;

	if (!ConfigFileLocked) {
		lck.l_type = F_UNLCK;
		(void) fcntl(ConfigFd, F_SETLK, &lck);
	}

	return NWCM_SUCCESS;
}

int
_UpdateAndUnlockConfig(void)
{
	int	ccode = NWCM_SUCCESS;

	if (!CHECKINIT())
		return NWCM_NOT_INITIALIZED;

	if (ConfigReadOnly)
		return NWCM_CONFIG_READ_ONLY;

	if (!ConfigFileLocked)
		return NWCM_CONFIG_NOT_LOCKED;

	if (ConfigDirty)
		ccode = _WriteConfigFile();

	lck.l_type = F_UNLCK;
	if (fcntl(ConfigFd, F_SETLK, &lck) < 0) {
		NWCMSystemErrno = errno;
		return NWCM_UNLOCK_FAILED;
	}

	ConfigFileLocked = 0;

	return ccode;
}

int
_ConfigFlush(void)
{
	if (CHECKINIT()) {
		(void) close(ConfigFd);
		ConfigFd = -1;
	}

	return NWCM_SUCCESS;
}

int
_ProbeConfigInit(void)
{
	INIT();

	return NWCM_SUCCESS;
}

static int
_InitConfigManager(void)
{
	char *pathPtr, *cp;
	char *NWCMSchemaPath;
	int err;
	int tmpFd1, tmpFd2, tmpFd3;

	/*
	 * Path to standard configuration file.
	 */
	if (((NWCMConfigFilePath = getenv(NWCONFIG_ENVIRONMENT_VARIABLE))
	    == NULL) || (!*NWCMConfigFilePath))
		NWCMConfigFilePath = DEFAULT_NWCONFIG_PATH;

	pathPtr = (char *)strdup(NWCMConfigFilePath);
	if(cp = strrchr(pathPtr, '/')) {
		*cp = 0;
	} else {
		strcpy(pathPtr, ".");
	}

	/*
	** If there is a NWU_SCHEMA_FILES env variable, use it to resolve location
	** of the .bin files, otherwise, cat /conf to pathPtr and use it.
	*/
	if (((NWCMSchemaPath = getenv(SCHEMA_ENVIRONMENT_VARIABLE)) == NULL)
			|| (!*NWCMSchemaPath))
		NWCMSchemaPath = DEFAULT_SCHEMA_PATH;

	strcat(NWCMSchemaPath, "/");

	/*
	 * Initialize NWCM configuration
	 */
	if(!NWCMINIT) {
		if(InitNWCM(NWCMSchemaPath))
			return NWCM_INIT_FAILED;
		else
			NWCMINIT = TRUE;
	}

	/*
	 * Open standard configuration file and determine open mode.
	 * To ensure we are not getting file descriptor 0, 1, or 2, we will
	 * open 3 tmpFds before opening nwconfig, and close them after.
	 */
	tmpFd1=open("/dev/null", O_RDWR);
	tmpFd2=open("/dev/null", O_RDWR);
	tmpFd3=open("/dev/null", O_RDWR);

	if ((ConfigFd = open(NWCMConfigFilePath, O_RDWR|O_CREAT,
	    NWCONFIG_MODES)) < 0) {
		if ((ConfigFd = open(NWCMConfigFilePath, O_RDONLY)) < 0) {
			close(tmpFd1);
			close(tmpFd2);
			close(tmpFd3);
			return NWCM_INIT_FAILED;
		}
		ConfigReadOnly++;
	} else {
		ConfigReadOnly = 0;
	}
	close(tmpFd1);
	close(tmpFd2);
	close(tmpFd3);

	/*
	 * Load contents of configuration file.
	 */
	if(err = _ParseConfigFile())
		return(err);

	/*
	 * Bind message domains for installed packages
	 */
	if(BindInstalledDomains())
		return NWCM_INIT_FAILED;

	return(err);
}

/*
 * _LookUpParameter(name, type, buffer)
 *
 *	Find the parameter definition structure, if it exists.  Match
 *	by parameter name, and optionally by type.  Return a copy of the
 *	parameter description through the area indicated by buffer.
 *	Use a simple binary search, because the schema compiler was
 *	kind enough to lexicographically order the parameter names, and
 *	there is no facility for adding/deleting on the fly anyway.
 *
 * NOTE:  _LookUpParameter is global so the command line utilities
 * can use it for a dirty hook.
 */

int
_LookUpParameter(char *name, enum NWCP type, struct cp_s *buffer)
{
	int	low, high, middle, cmpres;

	low = 0;
	high = ConfigurationParameterCount;
	middle = (low + high) / 2;

	for (;;) {
		if ((cmpres = strncmpi(name,
		    ConfigurationParameters[middle].name,
		    NWCM_MAX_STRING_SIZE)) == 0) {
			if (type != NWCP_UNDEFINED) {
				if (type !=
				    ConfigurationParameters[middle].type) {
					return NWCM_INVALID_TYPE;
				}
			}
			(void) memcpy((void *) buffer,
			    (void *) &ConfigurationParameters[middle],
			    sizeof(struct cp_s));
			return NWCM_SUCCESS;
		}
		if (cmpres < 0) {
			if (high == middle)
				break;
			high = middle;
		} else {
			if (low == middle)
				break;
			low = middle; 
		}
		middle = (low + high) / 2;
	}

	return NWCM_NOT_FOUND;
}

static int
_ParseConfigFile()
{
	int		dupfd;
	extern FILE *	yyin;
	extern int	yyparse(void);
#ifdef NEVER
	int 		debugerr;
	char		buf[21];
#endif

	if ((dupfd = dup(ConfigFd)) < 0) {
		NWCMSystemErrno = errno;
		return NWCM_SYSTEM_ERROR;
	}
	lseek(dupfd, 0, SEEK_SET);	/* voodoo */

	errno = 0; /* fdopen() may not fail during syscall */
	if ((yyin = fdopen(dupfd, "r")) == NULL) {
		NWCMSystemErrno = errno;
		return NWCM_SYSTEM_ERROR;
	}

	NWCMConfigFileLineNo = 1;

	if (yyparse()) {
		NWCMConfigFileLineNo--;
		return NWCM_SYNTAX_ERROR;
	}

	(void) fclose(yyin);
	yyin = NULL;

	if ((int) time(&ConfigLastSync) < 0) {
		NWCMSystemErrno = errno;
		return NWCM_SYSTEM_ERROR;
	}

	return NWCM_SUCCESS;
}

static int
_WriteConfigFile()
{
	FILE *		outfp;
	struct cp_s *	cpp;
	struct cp_s *	cpe;
	int 		bdata, err;

	errno = 0; /* fopen() may not fail in syscall */
	if ((outfp = fopen(NWCMConfigFilePath, "w")) == NULL) {
		NWCMSystemErrno = errno;
		return NWCM_SYSTEM_ERROR;
	}

	cpe = &ConfigurationParameters[ConfigurationParameterCount];

	for (cpp = ConfigurationParameters; cpp < cpe; cpp++) {
		switch (cpp->type) {
		case NWCP_INTEGER:
			if (*((unsigned long *) cpp->cur_val) !=
			    *((unsigned long *) cpp->def_val)) {
				switch (cpp->format) {
				case df_hexadecimal:
					fprintf(outfp, "%s = 0x%lX\n",
					    cpp->name,
					    *((unsigned long *) cpp->cur_val));
					break;
				case df_octal:
					if (*((unsigned long *)cpp->cur_val))
						fprintf(outfp, "%s = 0%lo\n",
						    cpp->name, *((unsigned long *) cpp->cur_val));
					else
						fprintf(outfp, "%s = 0\n",
						    cpp->name);
					break;
				default:
					fprintf(outfp, "%s = %lu\n", cpp->name,
					    *((unsigned long *) cpp->cur_val));
				}
			}
			break;
		case NWCP_BOOLEAN:
			bdata = *((int *) cpp->cur_val);
			if (cpp->validation.action) {
				if ((err = cpp->validation.action(bdata)) != 0) {
					if(bdata == FALSE)
						*((int *) cpp->cur_val) = TRUE;
					else
						*((int *) cpp->cur_val) = FALSE;
				}
			}
			if (*((int *) cpp->cur_val) !=
			    *((int *) cpp->def_val))
			{
				fprintf(outfp, "%s = %s\n", cpp->name,
				    *((int *) cpp->cur_val) ?
				    "on" : "off");
			}
			break;
		case NWCP_STRING:
			if (strncmp((char *) cpp->cur_val,
			    (char *) cpp->def_val, NWCM_MAX_STRING_SIZE))
			{
				fprintf(outfp, "%s = \"%s\"\n", cpp->name,
				    (char *) cpp->cur_val);
			}
			break;
		}
	}

	(void) fclose(outfp);

	if ((int) time(&ConfigLastSync) < 0) {
		NWCMSystemErrno = errno;
		return NWCM_SYSTEM_ERROR;
	}

	return NWCM_SUCCESS;
}

