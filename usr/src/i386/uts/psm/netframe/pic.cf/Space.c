/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:psm/netframe/pic.cf/Space.c	1.1"
#include <sys/types.h>
#include <sys/ipl.h>
#include <sys/nf_pic.h>


/* Initialized data for Programmable Interupt Controllers */

ushort_t cmdport[NPIC]		/* command port addrs for pics */
	= { MCMD_PORT, S1CMD_PORT, S2CMD_PORT, S3CMD_PORT };
ushort_t imrport[NPIC]		/* intr mask port addrs for pics */
	= { MIMR_PORT, S1IMR_PORT, S2IMR_PORT, S3IMR_PORT };
uchar_t masterpic[NPIC]		/* index of this pic's master (for 82380) */
	= { 0, 0, 0, 0 };
uchar_t masterline[NPIC]	/* line on master this slave connected to */
	= { 0, MASTERLINE1, MASTERLINE2, MASTERLINE3};

uchar_t curmask[NPIC];		/* current pic masks */

uchar_t iplmask[(PLHI + 1) * NPIC];	/* pic masks for intr priority levels */

uchar_t picbuffered = PICBUFFERED;	/* PICs in buffered mode */

int npic = NPIC;		/* number of pics configured */

pl_t svcpri[256];		/* service priority level for each interrupt */

struct irqtab irqtab[NPIC * PIC_NIRQ];	/* table of per IRQ information */
