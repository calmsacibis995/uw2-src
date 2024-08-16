/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/pcmp/pcmp.h	1.1"
#ident	"$Header: $"

/* Definitions for Intel PC+MP Platform Specification */

/* Misc. */
#define EBDA_BASE	0x9fc00	/* base of EBDA default location 639k	*/ 
#define EBDA_PTR	0x40e	/* pointer to base of EBDA segment 	*/ 
#define BMEM_PTR	0x413	/* pointer to installed base memory in kbyte*/ 
#define CPQ_RESET_VECT  0x467	/* reset vector location */

/* PC+MP Interrupt Mode Control Registers */
#define IMCR_ADDR	0x22
#define IMCR_DATA	0x23
  
#define MP_IMCRP	0x80	/* IMCR present */
#define MP_IMCRA	0x0	/* IMCR absent */
#define MP_DEF_TYPE	6	/* default table to use */
#define MP_DEF_IMCR 	MP_IMCRA/* default IMCRP to use */
 
struct pcmp_fptr {		/* PC+MP floating pointer structure */
	char sig[4];		/* signature "_MP_"	*/
	int paddr;		/* physical address pointer to MP table */
	char len, rev;		/* table length in 16 byte; revision #	*/
	char checksum;		/* checksum				*/
	char mp_feature_byte[5];/* MP feature byte 1: default system type */
};				/* MP feature byte 2: bit 7: IMCRP	*/

struct mpchdr {
 	char sign[4];		/* signature "MPAT" */
 	short tbl_len;		/* length of table */
 	char spec_rev;		/* MP+AT specification revision no. */
 	char checksum;
 	char oem_id[8];
 	char product_id[12];
 	paddr_t oem_ptr;	/* pointer to oem table (optional) */
 	short oem_len;		/* length of above table */
 	short num_entry;	/* number of 'entry's to follow */
 	paddr_t loc_apic_adr;	/* local apic physical address */
 	char fill[4];
};
 
/* types of entries (stored in bytes[0]) */
#define MP_ET_PROC	0	/* processor */
#define MP_ET_BUS	1	/* bus */
#define MP_ET_IOAPIC	2	/* i/o apic */
#define MP_ET_I_INTR	3	/* interrupt assignment -> i/o apic */
#define MP_ET_L_INTR	4	/* interrupt assignment -> local apic */
 
struct mpe_proc {
 	char entry_type;
 	char apic_id;
 	char apic_vers;
 	char cpu_flags;
 	int  cpu_signature;	/* stepping : 4,
 	     			   model : 4,
 	     			   family : 4, */
 	int features;
 	int fill[2];
};
 
struct mpe_bus {
 	char entry_type;
 	char bus_id;
 	char fill[2];
 	char name[4];
};
 
struct mpe_ioapic {
 	char entry_type;
 	char apic_id;
 	char apic_vers;
 	char ioapic_flags;
 	paddr_t io_apic_adr;
};
 
/* flag values for intr */
#define MP_INTR_POVAL	1
#define MP_INTR_POLOW	2
#define MP_INTR_ELVAL	4
#define MP_INTR_ELLEVEL 8
 
struct mpe_intr {
 	char entry_type;
 	char intr_type;
 	char intr_flags;
 	char fill;
 	char src_bus;
 	char src_irq;
 	char dest_apicid;
 	char dest_line;
};
 
union mpcentry {
 	char bytes[20];
 	struct mpe_proc p;
 	struct mpe_bus b;
 	struct mpe_ioapic a;
 	struct mpe_intr i;
};
 
struct mpconfig {
 	struct mpchdr hdr;
 	union mpcentry entry[1];
};
