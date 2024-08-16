/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libdlist:dl_sap.h	1.1"
/*	
 * Copyright (c) 1994 Novell, Inc. All Rights Reserved.	
 *
 * THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
 * The copyright notice above does not evidence any 
 * actual or intended publication of such source code.
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

/* 
 * Public Functions to enable a sap service 
 */
extern char * enableInstallSAP(void);
extern char * enableUnixWareSAP(void);
extern char * enableRemoteAppsSAP(void);
/* 
 * Public Functions to disable a sap service 
 */
extern char * disableInstallSAP(void);
extern char * disableUnixWareSAP(void);
extern char * disableRemoteAppsSAP(void);
/* 
 * Public Functions for sap service queries
 */
extern char * isSystemSapping(void);
extern char * isSystemSappingAType(const int sapType);
extern char * getSappingServerList(const int sapType, char *** list, int * listCount);

