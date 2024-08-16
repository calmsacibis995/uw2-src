/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:acc/mac/mac.cf/Space.c	1.4"
#include <config.h>
#include <sys/covert.h>
#include <sys/types.h>

int mac_cachel = MAC_CACHEL;
int mac_mru_lvls = MAC_MRU_LVLS;

int mac_installed;


/* Covert Channel stuff */

ulong_t ct_delay = CT_DELAY;	/* delay threshold in ticks */
ulong_t ct_audit = CT_AUDIT;	/* audit threshold in ticks */
ulong_t ct_cycle = CT_CYCLE;	/* cycling period in tick */
ulong_t cc_psearchmin = CC_PSEARCH;	/* minimal process search */
