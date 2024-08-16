/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nwmsg.h	1.19"
#ident	"$Id: nwmsg.h,v 1.35 1994/10/10 17:07:23 mark Exp $"

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
#if !defined(__MSG_H__)
#define __MSG_H__

#include <msgmaxlen.h>
#include <sys/nwportable.h>

/*
** Each message belongs to a message domain.  The domain is
** a message catalog file and a the set within the catalog file.
** Currently there are many domains within 4 message files.  The
** domain number is the filenumber << 8 plus the set number
** within the file.  Associated with each domain is
** a revision string and catalog descriptor.
** The revision strings are currently set 1 message number 1.
** Text messages should start with set 2.
*/

#define MSG_REV_SET 1				/* Default rev set and msg number...	*/
#define MSG_REV_NUMBER 1			/* currently Set 1 and message 1.		*/
#define MSG_MAX_DOMAINS_FILES 11	/* One for each file + 1.				*/

/*
** Message fileNumbers.  These are used as an index into fileStruct 
** to get the correct filename and catalog descriptor.
** Files are numbered starting with 0 upto MSG_MAX_DOMAINS_FILES - 2.
** The source file listed is the master m4 file in directory nls/English/
** It may include other m4 files as part of the domain.  If the messages
** need to be edited or additional messages added, first look in the  
** master m4 file for the message and then the related included files.
*/
#define MSG_NW_FILE_NUMBER 0		/*File 0 =  source is nwumsg.m4 */
#define MSG_DOMAIN_NW_FILE "nwumsgs"

#define MSG_DS_FILE_NUMBER 1		/*File 1 =  source is dsmsg.m4 DS utils */
#define MSG_DOMAIN_DS_FILE "dsmsgs"

#define MSG_NETMGT_FILE_NUMBER 2	/*File 2 =  source is nwummsg.m4 mgmt  */
#define MSG_DOMAIN_NETMGT_FILE "nmmsgs"

#define MSG_PRINT_FILE_NUMBER 3		/*File 3 =  source is printmsg.m4 pserver */
#define MSG_DOMAIN_PRINT_FILE "prntmsgs"

#define	MSG_NWCM_FILE_NUMBER	4	/* File 4 = source is nwcmmsg.m4 (NWCM) */
#define MSG_DOMAIN_NWCM_FILE "nwcmmsgs"

#define	MSG_NPS_FILE_NUMBER	5		/* File 5 = source is npsmsg.m4 (NPS) */
#define MSG_DOMAIN_NPS_FILE "npsmsgs"

#define	MSG_UTIL_FILE_NUMBER	6		/* File 6 = source is utilmsgs.m4 */
#define MSG_DOMAIN_UTIL_FILE "utilmsgs"

#define	MSG_NPFS_UTIL_FILE_NUMBER	7	/* File 7 = source is npfsutil.m4 */
#define MSG_DOMAIN_NPFS_UTIL_FILE "npfsutil"

#define	MSG_NUC_FILE_NUMBER	8	/* File 8 = source is nucmsg.m4 (NUC) */
#define MSG_DOMAIN_NUC_FILE "nucmsgs"

#define MSG_PSERV_FILE_NUMBER 9		/*File 9 =  source is printmsg.m4 pserver */
#define MSG_DOMAIN_PSERV_FILE "psrvmsgs"

#define	MSG_NWUTIL_FILE_NUMBER	10		/* File 6 = source is nwutil.m4 */
#define MSG_DOMAIN_NWUTIL_FILE "nwutil"

	/* Add additional files here (be sure to increase MSG_MAX_DOMAINS_FILES
	** for each new file added.)  Place the m4 source file in directory
	** nls/English and update the Imakefile there for the new catalog 
	** generation.
	*/


/*
** Message Domains
*/

/* Netware */
#define MSG_DOMAIN_NW ((MSG_NW_FILE_NUMBER<<8)+2)			/* File 0, set 2 */

/* Volume Management Services */
#define	MSG_DOMAIN_NWVM	((MSG_NW_FILE_NUMBER<<8)+3)			/* File 0, set 3 */

/* AFP Services */
#define MSG_DOMAIN_AFP ((MSG_NW_FILE_NUMBER<<8)+4)			/* File 0, set 4 */

/* NWU NWCM Parameter Help and Descriptions */
#define MSG_DOMAIN_NWU_DH ((MSG_NW_FILE_NUMBER<<8)+5)		/* File 0, set 5 */


/* Directory Services: dsview utility */
#define MSG_DOMAIN_DSVIEW ((MSG_DS_FILE_NUMBER<<8)+2)		/* File 1, set 2 */

/* Directory Services: dsrepair utility */
#define MSG_DOMAIN_DSREPAIR ((MSG_DS_FILE_NUMBER<<8)+3)		/* File 1, set 3 */

/* TimeSync administration utility */
#define MSG_DOMAIN_TSADMIN ((MSG_DS_FILE_NUMBER<<8)+4)		/* File 1, set 4 */

/* Directory Services: main library - libnds */
#define MSG_DOMAIN_LIBNDS ((MSG_DS_FILE_NUMBER<<8)+5)		/* File 1, set 5 */

/* Directory Services: install library - libdsi */
#define MSG_DOMAIN_LIBDSI ((MSG_DS_FILE_NUMBER<<8)+6)		/* File 1, set 6 */

/* Directory Services: install library - libdsi (code from INSTALL.NLM) */
#define MSG_DOMAIN_INSTALL ((MSG_DS_FILE_NUMBER<<8)+7)		/* File 1, set 7 */

/* Directory Services: timesync library code */
#define MSG_DOMAIN_TIMESYNC ((MSG_DS_FILE_NUMBER<<8)+8)		/* File 1, set 8 */
#define MSG_DOMAIN_NDS_DH ((MSG_DS_FILE_NUMBER<<8)+9)		/* File 1, set 9 */


/* Netware Managment */
#define MSG_DOMAIN_NWUMPSD ((MSG_NETMGT_FILE_NUMBER<<8)+2)	/* File 2, set 2 */
#define MSG_DOMAIN_NWUMD ((MSG_NETMGT_FILE_NUMBER<<8)+3)	/* File 2, set 3 */
#define MSG_DOMAIN_NWUMD_DH ((MSG_NETMGT_FILE_NUMBER<<8)+4)	/* File 2, set 4 */

/* NPRINTER */
#define MSG_DOMAIN_NPRINT ((MSG_PRINT_FILE_NUMBER<<8)+3)	/* File 3, set 3 */
#define MSG_DOMAIN_NPRINT_DH ((MSG_PRINT_FILE_NUMBER<<8)+4)	/* File 3, set 4 */

/* NetWare Configuration Manager */
#define MSG_DOMAIN_NWCM_FOLD ((MSG_NWCM_FILE_NUMBER<<8)+3)	/* File 4, set 3 */
#define MSG_DOMAIN_NWCM_DH ((MSG_NWCM_FILE_NUMBER<<8)+4)	/* File 4, set 4 */

/* Novell Protocol Stack Daemon Messages */
#define MSG_DOMAIN_NPS ((MSG_NPS_FILE_NUMBER<<8)+2)		/* File 5, set 2 */
/* Diagnostics Daemon */
#define MSG_DOMAIN_DIAG ((MSG_NPS_FILE_NUMBER<<8)+3)	/* File 5, set 3 */
/* Service Advertising Daemon */
#define MSG_DOMAIN_SAPD ((MSG_NPS_FILE_NUMBER<<8)+4)	/* File 5, set 4 */
/* Novell Virtual Terminal */
#define MSG_DOMAIN_NVTD ((MSG_NPS_FILE_NUMBER<<8)+5)	/* File 5, set 5 */
/* Stack info processes */
#define MSG_DOMAIN_IPXI ((MSG_NPS_FILE_NUMBER<<8)+6)	/* File 5, set 6 */
#define MSG_DOMAIN_RIPI ((MSG_NPS_FILE_NUMBER<<8)+7)	/* File 5, set 7 */
#define MSG_DOMAIN_SAPI ((MSG_NPS_FILE_NUMBER<<8)+8)	/* File 5, set 8 */
#define MSG_DOMAIN_SPXI ((MSG_NPS_FILE_NUMBER<<8)+9)	/* File 5, set 9 */
#define MSG_DOMAIN_DSCVR ((MSG_NPS_FILE_NUMBER<<8)+10)	/* File 5, set 10 */
#define MSG_DOMAIN_SUTIL ((MSG_NPS_FILE_NUMBER<<8)+11)	/* File 5, set 11 */
#define MSG_DOMAIN_RROUT ((MSG_NPS_FILE_NUMBER<<8)+12)	/* File 5, set 12 */
#define MSG_DOMAIN_DROUT ((MSG_NPS_FILE_NUMBER<<8)+13)	/* File 5, set 13 */
#define MSG_DOMAIN_NSTRT ((MSG_NPS_FILE_NUMBER<<8)+14)	/* File 5, set 14 */
#define MSG_DOMAIN_NSTAT ((MSG_NPS_FILE_NUMBER<<8)+15)	/* File 5, set 15 */
#define MSG_DOMAIN_NSTOP ((MSG_NPS_FILE_NUMBER<<8)+16)	/* File 5, set 16 */

#define MSG_DOMAIN_SAPL ((MSG_UTIL_FILE_NUMBER<<8)+2)	/* File 6, set 2 */
#define MSG_DOMAIN_NWCM ((MSG_UTIL_FILE_NUMBER<<8)+3)	/* File 6, set 3 */

/* NPFS utility programs */
#define MSG_DOMAIN_NPFSVM ((MSG_NPFS_UTIL_FILE_NUMBER<<8)+2) /* File 7, set 2 */

/* NUC Services */
#define MSG_DOMAIN_NUC_DH ((MSG_NUC_FILE_NUMBER<<8)+2)		/* File 8, set 2 */

/* PSERVER */
#define MSG_DOMAIN_PSERVER ((MSG_PSERV_FILE_NUMBER<<8)+2)	/* File 9, set 2 */
#define MSG_DOMAIN_PSERVER_DH ((MSG_PSERV_FILE_NUMBER<<8)+3)	/* File 9, set 3 */
#define MSG_DOMAIN_ATPS ((MSG_PSERV_FILE_NUMBER<<8)+4)	/* File 9, set 4 */

/* NWU utility programs */
#define MSG_DOMAIN_ETCINFO ((MSG_NWUTIL_FILE_NUMBER<<8)+2) /* File 10, set 2 */
#define MSG_DOMAIN_NWDUMP ((MSG_NWUTIL_FILE_NUMBER<<8)+3) /* File 10, set 3 */
#define MSG_DOMAIN_NWENGINE ((MSG_NWUTIL_FILE_NUMBER<<8)+4) /* File 10, set 4 */
#define MSG_DOMAIN_NXINFO ((MSG_NWUTIL_FILE_NUMBER<<8)+5) /* File 10, set 5 */

	/*
	** Added new domains here
	*/

#if defined (__STDC__) || defined(__cplusplus)
#ifdef __cplusplus
extern "C" {
#endif
/* Prototypes (DON'T EDIT) */
extern void	MsgScanOffsetAmountStr(char **, long *);
extern LONG	MsgScanOffsetDateStr(BYTE **, LONG *, LONG *, LONG *);
extern LONG	MsgScanOffsetTimeStr(BYTE **, LONG *, LONG *, LONG *);
extern int MsgBindDomain(int domain, char *msgFileName, char *msgRevStr);
extern int MsgGetDomain(void);
extern int MsgChangeDomain(int Doamin);
extern int MsgSetlocale(void);
extern void MsgGetDateStr(char *);
extern void MsgGetTimeStr(char *);
extern char * MsgGetStr(int msgNumber);
extern char * MsgDomainGetStr(int domain, int msgNumber);
extern void ConvertToUpper(char *);
#ifdef __cplusplus
}
#endif
#else
extern void	MsgScanOffsetAmountStr();
extern LONG	MsgScanOffsetDateStr();
extern LONG	MsgScanOffsetTimeStr();
extern int MsgBindDomain();
extern int MsgGetDomain();
extern int MsgChangeDomain();
extern int MsgSetlocale();
extern void MsgGetDateStr();
extern void MsgGetTimeStr();
extern char * MsgGetStr();
extern char * MsgDomainGetStr();
extern void ConvertToUpper();
#endif

#endif /* __MSG_H__ */
