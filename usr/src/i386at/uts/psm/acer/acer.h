/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/acer/acer.h	1.3"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#include <util/types.h>

#else

#include <sys/types.h>

#endif /* _KERNEL_HEADERS */

#define	XINTR		13		/* irq no. of inter-processor intr. */
#define	CLOCKINTR	0		/* irq no. of clock intr. */

struct acerpsmops {
	void    (*acer_reboot)(int);
	struct idt_init *(*acer_idt)(int);
	void    (*acer_intr_init)(void);
	void    (*acer_intr_start)(void);
	int     (*acer_numeng)(void);
	void    (*acer_configure)(void);
	void    (*acer_online_engine)(int);
	void    (*acer_selfinit)(void);
	void    (*acer_misc_init)(void);
	void    (*acer_offline_self)(void);
	void    (*acer_clear_xintr)(int);
	void    (*acer_send_xintr)(int);
	void    (*acer_timer_init)(void);
	ulong_t (*acer_usec_time)(void);
	boolean_t (*acer_isxcall)(void);
	int	(*acer_assignvec)(int, int);
	int	(*acer_unassignvec)(int);
};

#define	RESET		0x01		/* take processor in and out of reset */
#define	FPU387PRES	0x02		/* 387 is present		      */
#define	CACHEON		0x04		/* enable caching		      */
#define	PHOLD		0x08		/* put processor in HOLD	      */
#define	CACHEFLUSH	0x10		/* flush processor cache	      */
#define	FPU387ERR	0x20		/* 387 error occurred		      */
#define	PINT		0x40		/* cause an interrupt on this cpu     */
#define	INTDIS		0x80		/* disable interrupts on this cpu     */

#define IOMODE_PORT	0xC06		/* sym/asym, cache, etc.	      */
/* IOMODE_PORT bit values. */
#define SYMINTR		0x10		/* symmetric mode interrupts	      */

#define WHOAMI_PORT	0x0C70		/* who_am_i port (tells CPU # )	      */

/*	LEDs + Port xC67 Not currently implemented		*/

/* #define	A15CACHE1	0x0C67	/* P1 Processor Cache Control Port    */
/* #define	A15CACHE2	0xFC67	/* P2 Processor Cache Control Port    */
/* #define	A15CACHE3	0xCC67	/* P3 Processor Cache Control Port    */
/* #define	A15CACHE4	0xDC67	/* P4 Processor Cache Control Port    */
/* #define A15LKMOD	 0x2		/* CPQ LKMOD magic bit		      */
/* 
/* #define	A15LEDOFF	0	/* turn off activity LED	      */
/* #define	A15LEDON	1	/* turn on activity LED		      */

/* #define	A15LED_P1	0xEF	/* P1 activity LED port	     	      */
/* #define	A15LED_P2	0xEE	/* P2 activity LED port		      */

#ifdef ECC 

#define A15K_ECCFIXED	0xC10		/* latch reg. Recovered 1 bit ecc errs*/
#define A15K_MEMFATAL	0xC14		/* latch reg. fatal N bit ecc errs    */
#define A15K_MEMINFO	0xC18		/* memory config/event register	      */

#define	A15K_ECC1ENAB()	inb(0xc38)	/* enable single-bit err service*/
#define	A15K_ECC2ENAB()	inb(0xc39)	/* enable multi-bit err service	*/

#endif /* ECC */

#ifdef ORIGINAL

/* enable/disable write alloc on write-back cache */

#define A15K_WBALLOC_ON01	outb(0xcc4, 5)	/* 2 cpu board, cpus 0,1 */
#define A15K_WBALLOC_ON23	outb(0xccc4, 5)	/* 2 cpu board, cpus 2,3 */

#define A15K_WBALLOC_OFF01	outb(0xcc4, 1)	/* 2 cpu board, cpus 0,1 */
#define A15K_WBALLOC_OFF23	outb(0xccc4, 1)	/* 2 cpu board, cpus 2,3 */

#endif /* ORIGINAL */

/*
 * BIOS ports for write-back cache
 */

#define BIOS_PORT_CPU01		0xcc4   /* write only bios setup reg. cpu 0,1 */
#define BIOS_PORT_CPU23		0xccc4  /* write only bios setup reg. cpu 2,3 */

#define WRITE_BALLOC_ON		0x04

#define BIOS_SETUP_PORT		0x070
#define BIOS_BINFO_PORT		0x071

#define RAM_ROM_MASK		0x01
#define DRAM_EISA_MASK		0x02
