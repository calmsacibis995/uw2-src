/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nwconfig.h	1.6"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: nwconfig.h,v 1.10 1994/10/03 16:03:59 mark Exp $"
/*
 * Copyright 1991, 1992 Novell, Inc.
 * All Rights Reserved.
 *
 * This work is subject to U.S. and International copyright laws and
 * treaties.  No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 */

#ifndef __NWCONFIG_H__
#define __NWCONFIG_H__

#include <sys/nwportable.h>

/* nwconfig.h
 *
 *	Public Interface to the NetWare for UNIX Configuration Manager,
 *	from the version 1.0 specification.
 */

/*
 * Misc. Constants
 */
#define	NWCONFIG_FILE_MAGIC	3	/* this constant is not specified */
#define	NWCM_MAX_STRING_SIZE	128	/* this constant is not specified */

/*
 * Parameter Types
 */
enum NWCP {
/* 	symbolic name		parameter type	*/

	NWCP_UNDEFINED = 0,
	NWCP_BOOLEAN,		/* int		*/
	NWCP_INTEGER,		/* long		*/
	NWCP_STRING		/* char *	*/
};

/*
 * Configuration Manger Return Codes
 *
 *	The error codes are now defined by the message catalog
 *	facility so that the error codes and the message catalog
 *	entry numbers are the same.  We include the generated
 *	message table header file for those numbers.
 *
 *	See $TOP/nls/$LANG/nwcm.m4 for a list of the error codes
 *	and their meaning.
 */

#define	NWCM_SUCCESS		0		/* Successful */
#include <utilmsgtable.h>
#include <netmgtmsgtable.h>
#include <printmsgtable.h>
#include <nucmsgtable.h>
#include <nwcmmsgs.h>


/*
 * Error State Information for NWCM_SYNTAX_ERROR Return Code
 */
extern char *	NWCMConfigFilePath;
extern int	NWCMConfigFileLineNo;

/*
 * Error State Information for NWCM_SYSTEM_ERROR, NWCM_LOCK_FAILED,
 * and NWCM_UNLOCK_FAILED Return Codes
 */
extern int	NWCMSystemErrno;


/*
 * Parameter "Folders."  Don't forget to add folder names to
 * $(TOP)/nls/$(LANG)/nwcmfold.m4.
 */

#define	NWCM_PF_NONE			0		/* Do Not Display */
#define	NWCM_PF_GENERAL			1		/* General Server Parameters */
#define	NWCM_PF_NDS				2		/* Directory Services Parameters */
#define	NWCM_PF_AFP				3		/* AFP Server Parameters */
#define	NWCM_PF_SYSTUNE			4		/* System Tunable Parameters */
#define	NWCM_PF_DEFAULT			5		/* Default Folder */
#define NWCM_PF_LOCALE			6		/* Localization Parameters */
#define	NWCM_PF_IPXSPX			7		/* IPX/SPX Parameters */
#define	NWCM_PF_SAP				8		/* SAP Parameters */
#define	NWCM_PF_APPLETALK		9		/* AppleTalk Transport Parameters */
#define	NWCM_PF_ATPS			10		/* AppleTalk Print Services Parameters */
#define	NWCM_PF_NWUM			11		/* NetWare Management */
#define	NWCM_PF_PSERVER			12		/* Printing (PSERVER) */
#define	NWCM_PF_NPRINTER		13		/* Printing (NPRINTER) */
#define	NWCM_PF_NVT				14		/* NVT */
#define	NWCM_PF_TS				15		/* Time Synchronization Parameters */
#define	NWCM_PF_NUC				16		/* NetWare UNIX Client Parameters */

#define	NWCM_NUM_PARAM_FOLDERS	17


/*
 * Function Prototypes
 */
#if defined( __STDC__) || defined(__cplusplus)
#ifdef __cplusplus
extern "C" {
#endif
extern int		NWCMCleanConfig(void);
extern int		NWCMGetParam(char *, enum NWCP, void *);
extern int		NWCMGetParamDefault(char *, enum NWCP, void *);
extern int		NWCMGetParamFolder(char *, int *);
extern int		NWCMGetParamDescription(char *, char **);
extern int		NWCMGetParamHelpString(char *, char **);
extern int		NWCMSetParam(char *, enum NWCP, void *);
extern int		NWCMSetToDefault(char *);
extern int		NWCMValidateParam(char *, enum NWCP, void *);
extern int		NWCMLockConfig(void);
extern int		NWCMUnlockConfig(void);
extern int		NWCMFlush(void);
extern const char *	NWCMGetConfigFilePath(void);
extern const char *	NWCMGetConfigDirPath(void);
extern int		NWCMPerror(int, char *, ...);
#ifdef __cplusplus
}
#endif
#else /* __STDC__ */
extern int		NWCMCleanConfig();
extern int		NWCMGetParam();
extern int		NWCMGetParamDefault();
extern int		NWCMGetParamFolder();
extern int		NWCMGetParamDescription();
extern int		NWCMGetParamHelpString();
extern int		NWCMSetParam();
extern int		NWCMSetToDefault();
extern int		NWCMValidateParam();
extern int		NWCMLockConfig();
extern int		NWCMUnlockConfig();
extern int		NWCMFlush();
extern char *		NWCMGetConfigFilePath();
extern char *		NWCMGetConfigDirPath();
extern int		NWCMPerror();
#endif /* __STDC__ */

#endif /* __NWCONFIG_H__ */
