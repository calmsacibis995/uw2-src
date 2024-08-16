/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pkg.ihvhba:i386/pkg/ihvhba/iiop/atup.space.c	1.1"
#ident	"$Header: $"

/*
** ident @(#) Space.c 1.1 1 2/7/94 16:03:47
**
** sccs_id[] = {"@(#) 1.1 Space.c "}
*/

/*
***************************************************************************
**
**      MODULE NAME:  Space.c
**
**      PURPOSE:  Space file for PIC
**
**      DEPENDENCIES:
**
**          o Tricord Powerframe Model 30/40 & ESxxxx hardware.
**
**      REVISION HISTORY:
**      FPR/CRN     Date    Author      Description
**
**		2/4/93      M. Conner   Initial dev. TLP5/x27 rel.
**     
****************************************************************************
*/
/*
 * Initialized data for Programmable Interupt Controllers (i8259)
 */

#include <sys/types.h>
#include <sys/ipl.h>
#include <sys/pic.h>

#ifdef NPIC
#undef NPIC
#define NPIC 3
#endif
#define MAXNUMCPU 2 
#define S2CMD_PORT 0xcc0
#define S2IMR_PORT 0xcc1
#define MASTER2LINE 5 

/*
 * command port addrs of pics for self access
 */
ushort_t cmdport[NPIC] = {
	MCMD_PORT, SCMD_PORT, S2CMD_PORT
};

/*
 * interrupt mask port addrs of pics for self access
 */
ushort_t imrport[NPIC] = {
	MIMR_PORT, SIMR_PORT, S2IMR_PORT
};

uchar_t masterpic[NPIC]		/* index of this pic's master (for 82380) */
	= { 0, 0 , 0 };

/*
 * line on master this slave connected to
 */
uchar_t masterline[NPIC] = {
	0, MASTERLINE, MASTER2LINE
};

/*
 * current pic masks
 */
uchar_t curmask[MAXNUMCPU][NPIC];

/*
 * pic masks for intr priority levels
 */
uchar_t iplmask[MAXNUMCPU][(PLHI + 1) * NPIC];

uchar_t picbuffered = 1;	/* PICs in buffered mode */

int npic = NPIC;		/* number of pics configured */

/*
 * service priority level for each interrupt
 */
pl_t svcpri[NPIC * PIC_NIRQ];

/*
 * table of per IRQ information
 */ 
struct irqtab irqtab[NPIC * PIC_NIRQ];

/*
 * pointers to current masks of pics for each engine. 
 */
uchar_t *curmaskp[MAXNUMCPU] = {
	(uchar_t *)&curmask[0],
	(uchar_t *)&curmask[1],
};

/*
 * pointers to pic masks for intr priority levels for each engine. 
 */
uchar_t *iplmaskp[MAXNUMCPU] = {
	(uchar_t *)&iplmask[0],
	(uchar_t *)&iplmask[1],
};
