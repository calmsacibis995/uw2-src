/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/compaq/syspro.h	1.6"
#ident	"$Header: $"

#ifdef _KERNEL_HEADERS

#include <util/types.h>

#else

#include <sys/types.h>

#endif /* _KERNEL_HEADERS */


#define	CPQ_SYSTEMPRO	0x01
#define	CPQ_SYSTEMPRO_COMPATIBLE	0x08

#define	ASYMINTR	0x01		/* asymmetric i/o */

#define	XINTR		13
#define	CLOCKINTR	0

struct cpqpsmops {
	void	(*cpq_reboot)(int);
	struct idt_init *(*cpq_idt)(int);
	void	(*cpq_intr_init)(void);
	void	(*cpq_intr_start)(void);
	int	(*cpq_numeng)(void);
	void	(*cpq_configure)(void);
	void	(*cpq_online_engine)(int);
	void	(*cpq_selfinit)(void);
	void	(*cpq_misc_init)(void);
	void	(*cpq_offline_self)(void);
	void	(*cpq_clear_xintr)(int);
	void	(*cpq_send_xintr)(int);
	void	(*cpq_timer_init)(void);
	ulong_t	(*cpq_usec_time)(void);
	boolean_t (*cpq_isxcall)(void);
	boolean_t (*cpq_ecc_intr)(void);
	boolean_t (*cpq_dma_intr)(void);
	int	(*cpq_assignvec)(int, int);
	int	(*cpq_unassignvec)(int);
};


/*
 * Systempro Compatible Definitions
 */

/*
 * EISA slot 0 contains the system board id. Can also use the following
 * code to get at the system board id string.
 *
 * eid[0] = inb(EISA_ID0(0));
 * eid[1] = inb(EISA_ID1(0));
 * eid[2] = inb(EISA_ID2(0));
 * eid[3] = inb(EISA_ID3(0));
 */

#define	ISSYSTEMPRO	(inb(EISA_CFG0) == 0xE && inb(EISA_CFG1) == 0x11)
#define	ISPOWERPRO	(inb(EISA_CFG0) == 0x5 && inb(EISA_CFG1) == 0x92)

/*
 * Bits of Processor control port (0x0c6a and 0xfc6a)
 *
 * Processor Control and Status Ports for Systempro compatible mode are 
 * are defined in compaq.cf/Space.c file.
 */
#define RESET		0x01		/* reset processor */
#define FPU387PRES	0x02		/* 387 is present */
#define CACHEON		0x04		/* enable caching */
#define PHOLD		0x08		/* put processor in HOLD */
#define CACHEFLUSH	0x10		/* flush processor cache */
#define FPU387ERR	0x20		/* 387 floating point unit error */
#define PINT		0x40		/* cause an interrupt on this cpu */
#define INTDIS		0x80		/* disable interrupt on this cpu */

#define WHOAMI_PORT	0x0C70          /* WHO-AM-I port */

/*
 * Extended IRQ13 Control and Status Port.
 *
 * The IRQ13 interrupt level is shared by the numerric coprocessor 
 * error interrupt, DMA chaining, and correctable error interrupt.
 * This register is active in both the Compaq Systempro compatible
 * programming mode and the symmetric mode.
 */
#define	INT13_XSTATUS_PORT	0x0CC9

/*
 * Bit description of extended irq13 control and status port (0xcc9)
 */
#define	INT13_NCP_ERROR_ACTIVE	0x01	/* numeric coprocessor error intr */
#define	INT13_NCP_ENABLE	0x02	/* status -- active and enabled */
#define	INT13_DMA_CHAIN_ACTIVE	0x04	/* DMA chaning interrupt status -- */
#define	INT13_DMA_CHAIN_ENABLE	0x08	/* active and enabled */
#define	INT13_ECC_MEMERR_ACTIVE	0x10	/* ECC correctable memory error intr */
#define	INT13_ECC_MEMERR_ENABLE	0x20	/* status -- active and enabled */

#define RAM_RELOC_REG	0x80C00000	/* RAM Relocation Register */
#define ENG1_CACHE_REG	0x80C00002	/* P1 Cache Control Register */

#define RESET_VECT	0x467		/* reset vector location */
