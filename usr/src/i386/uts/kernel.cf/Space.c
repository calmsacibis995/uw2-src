/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:kernel.cf/Space.c	1.14"
#ident	"$Header: $"

#include <config.h>	/* to collect tunable parameters */

#include <sys/types.h>
#include <sys/var.h>

/* Enhanced Application Binary Compatibility */

#include <sys/sockmod.h>
#include <sys/osocket.h>

/* Enhanced Application Binary Compatibility */

#ifndef NUMSXT
#define NUMSXT	0
#endif
#ifndef XSDSEGS
#define XSDSEGS	0
#endif
#ifndef XSDSLOTS
#define XSDSLOTS 0
#endif

struct var	v = {
	NBUF,
	NCALL,
	NPROC,
	MAXCLSYSPRI,
	MAXUP,
	NHBUF,
	NHBUF-1,
	NPBUF,
	MAXPMEM,
	NAUTOUP,
	BUFHWM,
	/* XENIX Support */
	NSCRN,
	NEMAP,
	NUMSXT,
	XSDSEGS,
	XSDSLOTS,
	/* End XENIX Support */
	MAXULWP,
	/* XXX - make into real tunables: */
	1,	/* v_nonexclusive */
	100,	/* v_max_proc_exbind: a guess only */
	128,	/* v_static_sq */
};

boolean_t nullptr_default = NULLPTR;
#if NULLPTR == 2
boolean_t nullptr_log = B_TRUE;
#else
boolean_t nullptr_log = B_FALSE;
#endif

/*
 * This array lists all kernel routines which return structures.
 * This is needed to properly handle stack traces for the i386 family,
 * since the calling convention for such routines involves a net change
 * of the stack pointer.
 */
#include <sys/dl.h>
vaddr_t structret_funcs[] = {
	(vaddr_t)ladd,
	(vaddr_t)lsub,
	(vaddr_t)lmul,
	(vaddr_t)ldivide,
	(vaddr_t)lmod,
	(vaddr_t)lshiftl,
	(vaddr_t)lsign
};
size_t structret_funcs_size = sizeof structret_funcs;

/*
 * Misc parameter variables.
 */

#ifndef	CPURATE			
#define	CPURATE		16	/* minimum cpu rate in Mhz */
#endif

#ifndef	CPUSPEED		/* approximate speed of cpu in VAX MIPS */
#define	CPUSPEED	25	/* used for spin loops in various places */
				/* normalised to 100 Mhz                 */
#endif

#ifndef	I486_CPUSPEED		/* ditto for 25 Mhz 486 */
#define	I486_CPUSPEED	50
#endif

int	cpurate		= CPURATE;
int	lcpuspeed	= CPUSPEED;
int	i486_lcpuspeed	= I486_CPUSPEED;


/* Enhanced Application Binary Compatibility */

/* 
 * SCO socket emulation protocol translation structure.
 * It is used by osocket, a loadable driver.
 * This is here because it is initialized once only by /usr/eac/bin/initsock,
 * when the system goes multi-user.
 */
struct odomain *osoc_family = 0; 

/* end Enhanced Application Binary Compatibility */


/*
 * USER_RDTSC controls permission to use the RDTSC instruction to read
 * the Time Stamp Counter from user mode.  If non-zero, the user may
 * execute RDTSC on processors which support it (e.g. Pentium).
 */
boolean_t user_rdtsc = USER_RDTSC;

#if	PHYSTOKVMEM

asm(".section .kvphystokv, \"w\", \"nobits\"; .align 0x1000 ; .zero 268435456");
asm(".globl kvavbase; .align  0x1000; .set kvavbase, . ; .zero 4194304");

#else

asm(".section .kvphystokv, \"w\", \"nobits\"; .align 0x1000 ; .zero 4194304");
asm(".globl kvavbase; .align  0x1000; .set kvavbase, . ; .zero 4194304");

#endif

