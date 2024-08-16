/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:psm/netframe/apic.cf/Space.c	1.1"
#ident	"$Header: $"
#include <sys/types.h>
#include <sys/ipl.h>
#include <sys/nf_apic.h>

long io_apic[] = {0x8400, 0x8800};
int nioapics = (sizeof io_apic)/(sizeof (long));

struct apic_int apic_int[] = {
	{0,	PROC_APIC,	AP_LVT_TIMER,		0},	/* clock */
	{15,	IO_APIC,	0x8400,			15},	/* async */
	{16,	IO_APIC,	0x8800,			0},	/* iop 1 */
	{18,	IO_APIC,	0x8800,			1},	/* iop 2 */
	{20,	IO_APIC,	0x8800,			2},	/* iop 3 */
	{22,	IO_APIC,	0x8800,			3},	/* iop 4 */
	{24,	IO_APIC,	0x8800,			4},	/* iop 5 */
	{26,	IO_APIC,	0x8800,			5},	/* iop 6 */
	{28,	IO_APIC,	0x8800,			6},	/* iop 7 */
	{30,	IO_APIC,	0x8800,			7},	/* iop 8 */
};

int napicints = ((sizeof apic_int)/(sizeof (struct apic_int)));

/* 
 * interrupts assignments in groups per spl level
 * Each group consists of 16 interrupts.
 * Total groups should not exceed 9.
 */
int apic_groups_per_spl[PLHI+1] = {
	0,		/* spl 0 */
	1,		/* spl 1 */
	1,		/* spl 2 */
	1,		/* spl 3 */
	1,		/* spl 4 */
	1,		/* spl 5 */
	1,		/* spl 6 */
	1,		/* spl 7 */
	1,		/* spl 8 */
};
