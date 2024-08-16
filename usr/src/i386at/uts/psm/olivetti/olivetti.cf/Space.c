/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/olivetti/olivetti.cf/Space.c	1.1"
/*	Copyright (c) 1993 UNIX System Laboratories, Inc. 	*/
/*	  All Rights Reserved                             	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.   	            	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/
/*								*/
/*	Copyright Ing. C. Olivetti & C. S.p.A.			*/


#include <sys/types.h>
#include <sys/ipl.h>
#include <sys/pic.h>
#include <sys/olivetti.h> /* Olivetti 5050 specific defines */


/* Initialized data for Programmable Interupt Controllers */

ushort_t cmdport[NPIC]		/* command port addrs for pics */
	= { MCMD_PORT, SCMD_PORT };

ushort_t imrport[NPIC]		/* intr mask port addrs for pics */
	= { MIMR_PORT, SIMR_PORT };

uchar_t masterpic[NPIC]		/* index of this pic's master (for 82380) */
	= { 0, 0 };

uchar_t masterline[NPIC]	/* line on master this slave connected to */
	= { 0, MASTERLINE };

uchar_t curmask[NPIC];		/* current pic masks */

uchar_t iplmask[(PLHI + 1) * NPIC];	/* pic masks for intr priority levels */

uchar_t picbuffered = PICBUFFERED;	/* PICs in buffered mode */

int npic = NPIC;		/* number of pics configured */

pl_t svcpri[NPIC * PIC_NIRQ];	/* service priority level for each interrupt */

struct irqtab irqtab[NPIC * PIC_NIRQ];	/* table of per IRQ information */

/*
 * processor control port
 */
ushort_t engine_ctl_port[OLIBUS_MAXNUMCPU] = {
	ENG1_CTL_PORT,
	ENG2_CTL_PORT,
	ENG3_CTL_PORT,
	ENG4_CTL_PORT
};
