/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nwutil_mt.h	1.3"
#ifndef _NET_NW_UTIL_MT_H  /* wrapper symbol for kernel use */
#define _NET_NW_UTIL_MT_H  /* subject to change without notice */

#ident  "$Id: nwutil_mt.h,v 1.4 1994/05/02 21:07:53 vtag Exp $"

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

/*
 * novell_mt.h
 */

#ifndef NW_UP

#ifdef _REENTRANT

#include "nwmsg.h"
#include <thread.h>
#include <synch.h>

/* thread-specific data structure definition */
struct sap_tsd {
	void *sap_listp;
};

struct msg_tsd {
	void *curDomain;
	unsigned char	*buf;
};

#define SAP_TSD_KEYTBL_SIZE \
			( sizeof(struct sap_tsd) / sizeof(void *) )
#define MSG_TSD_KEYTBL_SIZE \
			( sizeof(struct msg_tsd) / sizeof(void *) )

#ifdef __STDC__

extern void FreeSapTSD(void *);
extern void FreeMsgTSD(void *);
	
#else /* ! __STDC__ */

extern void FreeSapTSD();
extern void FreeMsgTSD();

#endif /* __STDC__ */

/* key for thread specific data */
extern THREAD_KEY_T	listHeadKey;
extern THREAD_KEY_T	domainKey;

/* locks for libnwutil */
extern MUTEX_T	sap_list_lock;
extern MUTEX_T	mem_map_lock;
extern MUTEX_T	head_list_lock;

#endif /* _REENTRANT */

#endif /* ! NW_UP */
#endif /* _NET_NW_SAP_MT_H  */
