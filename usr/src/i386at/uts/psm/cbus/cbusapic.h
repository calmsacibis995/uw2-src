/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/cbus/cbusapic.h	1.2"
#ident	"$Header: $"

#ifndef _CBUS1_
#define _CBUS1_

/*
 * THE CBUS1 HARDWARE ARCHITECTURE DEFINITIONS
 */

#define CBUS1_CACHE_LINE	16		/* 16 byte lines */
#define CBUS1_CACHE_SHIFT	4		/* byte size to line size */
#define CBUS1_CACHE_SIZE	0x100000	/* 1 Mb caches */

#define CBUS1_SHADOW_REGISTER	0xB0		/* offset of shadow register*/

#define DISABLE_BIOS_SHADOWING	0x0		/* disable ROM BIOS shadowing*/

/*
 * defines for the CBUS ecc control register
 */
#define	CBUS1_EDAC_SAEN		0x80
#define CBUS1_EDAC_1MB		0x40
#define	CBUS1_EDAC_WDIS		0x20
#define	CBUS1_EDAC_EN		0x10
#define	CBUS1_EDAC_SAMASK	0xf

/*
 *		THE APIC HARDWARE ARCHITECTURE DEFINITIONS
 */
#define APIC_ICR_BUSY		0x1000	/* APIC intr command reg is busy */

#define LOCAL_APIC_ENABLE	0x100	/* all APIC intrs now enabled */

#define APIC_INTR_UNMASKED	0x0	/* this interrupt now enabled */
#define APIC_INTR_MASKED	0x1	/* this interrupt now disabled */

#define APIC_INTR_FIXED		0x0	/* this interrupt is tied to a CPU */
#define APIC_INTR_LIG		0x1	/* this interrupt is lowest-in-group */
#define APIC_INTR_NMI		0x4	/* this interrupt is an NMI */

#define	APIC_EDGE		0x0	/* this interrupt is edge-triggered */
#define	APIC_LEVEL		0x1	/* this interrupt is level-triggered */
#define	APIC_LOGICAL_MODE	0x1	/* only logical mode is being used */

#define	APIC_ALL_PROCESSORS	(unsigned long)-1  /* dest format reg default */

#define APIC_SELFINTR		0x40000	/* APIC's self-interrupt code */

#define APIC_ALLINCLSELF	0x80000	/* sending a DEASSERT-RESET command */
#define APIC_TRIGGER_LEVEL	0x8000	/* generate a level interrupt */

#define APIC_TIMER_MICROSECOND	33	/* 33MHz clocking */
#define CLOCK_INTERRUPT_RATE	10000	/* specify in microseconds */
#define CLKIN_ENABLE_PERIODIC	0x20000	/* enable periodic CLKIN APIC intrs */

#define APIC_TIMER_OFFSET	0x0390	/* APIC current count register offset */

/* 
 * offset of APIC current count register in remote-read format (ie: shift 4) 
 */
#define APIC_TIMER_OFFSET_RR	0x039	

/* 
 * APIC current count reg offset 
 */
#define APIC_REMOTE_REGISTER_OFFSET	0xC0	

/*
 * last APIC remote read cmd is complete
 */
#define APIC_REMOTE_READ_COMPLETE 0x20000	

/* 
 * mask for last APIC remote read status
 */
#define APIC_REMOTE_READ_STATUS_MASK 0x30000 

#define APIC_ICR_BUSY	0x1000		/* APIC intr command reg is busy */

#define APIC_LOGICAL_MODE_IN_ULONG	0x800	/* sending a APIC-LOGICAL IPI */

#define APIC_REMOTE_READ	0x300	/* sending an APIC remote read command*/

#define APIC_DEASSERT_RESET	0x500	/* sending a DEASSERT-RESET command */

#define APIC_FULL_DRESET	(APIC_ALLINCLSELF | \
				 APIC_TRIGGER_LEVEL | \
				 APIC_LOGICAL_MODE_IN_ULONG | \
				 APIC_DEASSERT_RESET)

#define APIC_INTR_DISABLED	0x10000	/* disable this redirection entry */

/* left shift needed to convert processor_bit to Intel APIC ID - this applies */
/* to the logical destination ID and redirection entry registers only. */

/*
 * left shift needed to convert processor_bit to Intel APIC ID.
 * this applies to the:
 *
 *		I/O unit ID register
 *		I/O unit ID redirection entry destination registers
 *		local unit ID register
 *		local unit logical destination register
 */

#define APIC_BIT_TO_ID		24	/* also in cbusapic.asm */

/*
 * both LOCAL and I/O APICs must be on page-aligned boundaries
 * for the current HalpMapPhysicalMemory() calls to work.
 */

#define LOCAL_APIC_LOCATION	0xFEE00000	/* Physical addr of local APIC*/
#define IO_APIC_LOCATION	0xFEE00000	/* Physical addr of I/O APIC*/

#define LOCAL_APIC_SIZE		0x400
#define IO_APIC_SIZE		0x11

/*
 * I/O APIC registers.  note only register select & window register are
 * directly accessible in the address space.  other I/O registers are
 * reached via these two registers, similar to the CMOS access method.
 */
#define	IO_APIC_REGSEL		0x00
#define	IO_APIC_WINREG		0x10

#define	IO_APIC_ID_OFFSET	0x00
#define	IO_APIC_VERSION		0x01
#define	IO_APIC_REDIRLO		0x10
#define	IO_APIC_REDIRHI		0x11

/*
 * Each I/O APIC has one redirection entry for each interrupt it handles.
 * note that since it is not directly accessible (instead the window register
 * must be used), consecutive entries are byte aligned, not 16 byte aligned.
 */
typedef union _redirection_t {
	struct {
		unsigned long	vector : 8;
		unsigned long	delivery_mode : 3;
		unsigned long	dest_mode : 1;
		unsigned long	delivery_status : 1;		/* read-only */
		unsigned long	reserved0 : 1;
		unsigned long	remote_irr : 1;
		unsigned long	trigger : 1;
		unsigned long	mask : 1;
		unsigned long	reserved1 : 15;
		unsigned long	destination;
	} ra;
	struct {
		unsigned long	dword1;
		unsigned long	dword2;
	} rb;
} REDIRECTION_T;

/*
 * The Interrupt Command register format is used for IPIs.
 */
typedef union _intrcommand_t {
	struct {
		unsigned long		vector : 8;
		unsigned long		delivery_mode : 3;
		unsigned long		dest_mode : 1;
		unsigned long		delivery_status : 1;	/* read-only */
		unsigned long		reserved0 : 1;
		unsigned long		level : 1;
		unsigned long		trigger : 1;
		unsigned long		remote_read_status : 2;
		unsigned long		destination_shorthand : 2;
		unsigned long		reserved1 : 12;
		unsigned long		pad[3];
		unsigned long		destination;
	} ra;
	struct {
		unsigned long		dword1;
		unsigned long		dword2;
	} rb;
} INTRCOMMAND_T, *PINTRCOMMAND;

/*
    used to read/write the current task priority.  reads DO NOT
    have to be AND'ed with 0xff - this register has been
    guaranteed by both Corollary (for the CBC) and Intel
    (for the APIC) so that bits 8-31 will always read zero.
    (The Corollary guarantee is written, the Intel is verbal).
  
    note that this definition is being used both for the
    64-bit CBC and the 32-bit APIC, even though the APIC
    really only has the low 32 bits.
  
    Task Priority ranges from a low of 0 (all interrupts unmasked)
    to a high of 0xFF (all interrupts masked) on both CBC and APIC.
 */
typedef union _taskpri_t {
	struct {
		unsigned long	pri : 8;
		unsigned long	zero : 24;
		unsigned long	reserved1 : 32;
	} ra;
	struct {
		unsigned long	low_dword;
		unsigned long	high_dword;
	} rb;
} TASKPRI_T, *PTASKPRI;

/*
 * The layout of an I/O APIC register set
 */
typedef struct _ioapic_registers_t {
	unsigned long		register_select;
	unsigned char		fill0[0xC];		/* padding to 0x10*/
	unsigned long		window_register;
} APIC_IOREGISTERS_T, *PAPIC_IOREGISTERS;

typedef struct _apic_registers_t {
	unsigned char		fill0[0x20];

	unsigned long		local_unit_id;			/* 0x20 */
	unsigned char		fill1[0x80 - 0x20 - sizeof(unsigned long)];

	TASKPRI_T		apic_task_priority;		/* 0x80 */
	unsigned char		fill2[0xB0 - 0x80 - sizeof(TASKPRI_T)];

	unsigned long		apic_eoi;			/* 0xB0 */
	unsigned char		fill3[0xD0 - 0xB0 - sizeof(unsigned long)];

	unsigned long		apic_logical_destination;	/* 0xD0 */
	unsigned char		fill4[0xE0 - 0xD0 - sizeof(unsigned long)];

	unsigned long		apic_destination_format;	/* 0xE0 */
	unsigned char		fill5[0xF0 - 0xE0 - sizeof(unsigned long)];

	unsigned long		apic_spurious_vector;		/* 0xF0 */
	unsigned char		fill6[0x300 - 0xF0 - sizeof(unsigned long)];

	INTRCOMMAND_T		apic_icr;			/* 0x300 */
	unsigned char		fill7[0x320 - 0x300 - sizeof(INTRCOMMAND_T)];

	INTRCOMMAND_T		timer_vector;			/* 0x320 */
	unsigned char		fill9[0x360 - 0x320 - sizeof(INTRCOMMAND_T)];

	REDIRECTION_T		apic_local_int1;		/* 0x360 */
	unsigned char		fillA[0x380 - 0x360 - sizeof(REDIRECTION_T)];

	REDIRECTION_T		initial_count;			/* 0x380 */
	unsigned char		fillB[0x4D0 - 0x380 - sizeof(REDIRECTION_T)];

	/*
	 * Note that APMode is also the PolarityPortLow register
	 */
	unsigned char		ap_mode;			/* 0x4D0 */
	unsigned char		polarity_port_high;		/* 0x4D1 */

} APIC_REGISTERS_T, *PAPIC_REGISTERS;

#define LOWEST_APIC_PRI		0x00
#define LOWEST_APIC_DEVICE_PRI	0x40
#define HIGHEST_APIC_PRI	0xF0

/*
 *
 *		PURE SOFTWARE DEFINITIONS HERE
 *
 */

/*
 * this structure is also declared in cbusapic.asm, and is used for
 * communications between processors modifying the I/O APIC.
 */
typedef struct _redirport_t {
	unsigned long			status;
	unsigned long			apic_id;
	unsigned long			redirection_address;
	unsigned long			redirection_command;
	unsigned long			redirection_destination;
} REDIR_PORT_T;

#define REDIR_ACTIVE_REQUEST		0x01
#define REDIR_ENABLE_REQUEST		0x02
#define REDIR_DISABLE_REQUEST		0x04
#define REDIR_LASTDETACH_REQUEST	0x08
#define REDIR_FIRSTATTACH_REQUEST	0x10

/*
 * the APIC task priority register definition is shared with CBUS-2,
 * and is thus, in the shared header file.
 */

#endif	    /* _CBUS1_ */
