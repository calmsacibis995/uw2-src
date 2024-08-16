/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/cbus/cbus2.h	1.4"
#ident	"$Header: $"

/*
 * General notes:
 *
 *	- ALL reserved fields must be zero filled on writes to
 *	  ensure future compatibility.
 *
 *	- general CSR register length is 64 bits.
 *
 */

typedef struct _csr_register_t {
	unsigned long	low_dword;
	unsigned long	high_dword;
} CSR_REGISTER_T, *PCSR_REG;




typedef union _elementid_t {
	struct {
		unsigned long	element_id : 4;
		unsigned long	reserved0 : 28;
		unsigned long	reserved1 : 32;
	} ra;
	CSR_REGISTER_T rb;
} ELEMENTID_T, *PELEMENTID;




typedef union _spurious_t {
	struct {
		unsigned long	vector : 8;
		unsigned long	reserved0 : 24;
		unsigned long	reserved1 : 32;
	} ra;
	CSR_REGISTER_T rb;
} SPURIOUS_T;



/*
 * The hardware interrupt map table (16 entries) is indexed by irq.
 * Lower numerical irq lines will receive higher interrupt (IRQL) priority.
 *
 * Each CBC has its own hardware interrupt map registers .  note that
 * each processor gets his own CBC, but it need only be used in this
 * mode if there is an I/O card attached to its CBC.  each EISA bridge
 * will have a CBC, which is used to access any devices on that EISA bus.
 */
typedef union _hwintrmap_t {
	struct {
		unsigned long	vector : 8;
		unsigned long	mode : 3;
		unsigned long	reserved0 : 21;
		unsigned long	reserved1 : 32;
	} ra;
	CSR_REGISTER_T rb;
} HWINTRMAP_T, *PHWINTRMAP;

/*
 * interrupt trigger conditions
 */
#define HW_MODE_DISABLED	0x0
#define HW_EDGE_RISING		0x100		/* ie: ISA card interrupts */
#define HW_EDGE_FALLING		0x200
#define HW_LEVEL_HIGH		0x500
#define HW_LEVEL_LOW		0x600

#define CBC_REV1HWINTR_MAP_ENTRIES	0x10
#define HWINTR_MAP_ENTRIES		0x20

/*
 * move this to ../../svc/corollary.h when possible.
 */
#define CBUS2_OEM_IBM_MCA	8

/*
 * 256 intrconfig registers for vectors 0 to 255.  this determines how
 * a given processor will react to each of these vectors.  each processor
 * has his own intrconfig table in his element space.
 */
typedef union _intrconfig_t {
	struct {
		unsigned long	imode : 2;
		unsigned long	reserved0 : 30;
		unsigned long	reserved1 : 32;
	} ra;
	CSR_REGISTER_T rb;
} INTRCONFIG_T, *PINTRCONFIG;

#define HW_IMODE_DISABLED	0x0
#define HW_IMODE_ALLINGROUP	0x1
#define HW_IMODE_LIG		0x2
#define INTR_CONFIG_ENTRIES	0x100


/*
 * 256 interrupt request registers for vectors 0 to 255.
 * parallel to the interrupt config register set above.
 * This is used to send the corresponding interrupt vector.
 * which processor gets it is determined by which element's
 * address space you write to.
 *
 * The irq field should always be set when accessed by software.
 * only hardware LIG arbitration will clear it.
 */
typedef union _intrreq_t {
	struct {
		unsigned long	irq : 1;
		unsigned long	reserved0 : 31;
		unsigned long	reserved1 : 32;
	} ra;
	CSR_REGISTER_T rb;
} INTRREQ_T, *PINTRREQ;

/*
 * interrupt control register (64 bit wide) bit field definitions in use.
 */
#define	PC_INTERRUPT_ENABLE	0x40

/*
 * the CBUS-2 task priority register bit layout and
 * minimum/maximum values are defined in cbus.h, as
 * they need to be shared with the symmetric CBUS.
 * fix: merge these
 */
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
 * Offsets of various distributed interrupt registers within the CSR space.
 */
typedef struct _csr {
	char		pad0[0x28];

	ELEMENTID_T	element_id;		/* 0x0028 */

	char		pad1[0x200 - 0x28 - sizeof (ELEMENTID_T)];

	TASKPRI_T	task_priority;		/* 0x0200 */

#ifdef CBUS2_OEM
	CSR_REGISTER_T	pad2;			/* 0x208 */
	CSR_REGISTER_T	faultcontrol;		/* 0x210 */
	CSR_REGISTER_T	faultindication;	/* 0x218 */
	CSR_REGISTER_T	interruptcontrol;	/* 0x220 */
	CSR_REGISTER_T	errorvector;		/* 0x228 */
	CSR_REGISTER_T	interruptindication;	/* 0x230 */
	CSR_REGISTER_T	pendingpriority;	/* 0x238 */
#else
	char		pad2[0x220 - 0x200 - sizeof (TASKPRI_T)];

	CSR_REGISTER_T	icontrol;		/* 0x0220 */

	char		pad3[0x240 - 0x220 - sizeof (CSR_REGISTER_T)];
#endif

	SPURIOUS_T	spurious_vector;	/* 0x0240 */

	CSR_REGISTER_T	write_only;		/* 0x0248 */

	char		pad4[0x600 - 0x248 - sizeof (CSR_REGISTER_T)];

	HWINTRMAP_T	hwintrmap[HWINTR_MAP_ENTRIES];	/* 0x600 */
	CSR_REGISTER_T	hwintrmapeoi[HWINTR_MAP_ENTRIES];/* 0x700 */
	INTRCONFIG_T	intrconfig[INTR_CONFIG_ENTRIES];/* 0x800 */
	INTRREQ_T	intrreq[INTR_CONFIG_ENTRIES];	/* 0x1000 */

	char		pad6[0x2000 - 0x1000 -
				INTR_CONFIG_ENTRIES * sizeof (INTRREQ_T)];

	CSR_REGISTER_T	system_timer;				     /* 0x2000*/

	char		pad7[0x3000 - 0x2000 - sizeof(CSR_REGISTER_T)];

	CSR_REGISTER_T	ecc_clear;		/* 0x3000 */
	CSR_REGISTER_T	ecc_syndrome;		/* 0x3008 */
	CSR_REGISTER_T	ecc_write_address;	/* 0x3010 */
	CSR_REGISTER_T	ecc_read_address;	/* 0x3018 */

	char		pad8[0x8000 - 0x3018 - sizeof(CSR_REGISTER_T)];

	CSR_REGISTER_T	CbcConfiguration;      /* 0x8000 */
} CSR_T, *PCSR;


/*
 * RRD will provide an entry for every CBUS-2 element.  To avoid
 * using an exorbitant number of PTEs, this entry will specify
 * only the CSR space within each CBUS-2 element's space.  And only
 * a subset of that, as well, usually on the order of 4 pages.
 * Code wishing to access other portions of the cbus_element
 * space will need to subtract down from the RRD-specified CSR
 * address and map according to their particular needs.
 */
#define MAX_CSR_BYTES		0x10000

#define csr_register		rb.low_dword

#define interrupt_control	icontrol.low_dword

#if 0
/*
 * the full layout of a cbus_element is immaterial to the PSR and is
 * here for reference only.  Ditto the offset of the CSR within it.
 * all the information must be configurable is really passed up from RRD.
 */
struct cbus_element {
	char	device_reserved	[60 * 1024 * 1024];
	char	control_io	[0x10000];
	char	pc_bus_io	[0x10000];
	char	csr		[MAX_CSR_BYTES];
	char	reserved	[0x1D0000];
	char	ram		[0x100000];
	char	prom		[0x100000];
};

/*
 * offset of CSR within any element's I/O space.
 */
#define CBUS_CSR_OFFSET		0x3C20000
#endif

#define CBUS_MAX_BRIDGES	2

#define WRITECBUS2(addr, val)	(*(int *)(addr) = (int)(val))

#define	EISA_CONTROL_PORT	(short)0xf2
#define	DISABLE_8259		(unsigned char)0x4

/*
 * bit 7 of the CBC configuration register must be turned off to enable
 * posted writes for EISA I/O cycles.
 */
#define CBC_DISABLE_PW  	0x80

