/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/pcmp/pcmp.c	1.17"
#ident	"$Header: $"

#include <svc/bootinfo.h>
#include <io/cram/cram.h>
#include <io/i8237A.h>
#include <mem/immu.h>
#include <mem/vm_mdep.h>
#include <mem/vmparam.h>
#include <svc/intr.h>
#include <svc/pic.h>
#include <svc/pit.h>
#include <svc/psm.h>
#include <svc/uadmin.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/processor.h>
#include <util/types.h>
#include <svc/systm.h>

#include <io/ddi.h>

#include <apic.h>
#include <pcmp.h>

/*
 * Interrupt entry points. 
 */

#define SOFTINT 63              /* software interrupt */

extern void softint(void);
extern void devint0(void);
extern void devint1(void);
extern void devint2(void);
extern void devint3(void);
extern void devint4(void);
extern void devint5(void);
extern void devint6(void);
extern void devint7(void);
extern void devint8(void);
extern void devint9(void);
extern void devint10(void);
extern void devint11(void);
extern void devint12(void);
extern void devint13(void);
extern void devint14(void);
extern void devint15(void);
extern void devint16(void);
extern void devint17(void);
extern void devint18(void);
extern void devint19(void);
extern void devint20(void);
extern void devint21(void);
extern void devint22(void);
extern void devint23(void);

extern void ivct60(void);
extern void ivct61(void);
extern void ivct62(void);
extern void ivct63(void);
extern void ivct64(void);
extern void ivct65(void);
extern void ivct66(void);
extern void ivct67(void);
extern void ivct68(void);
extern void ivct69(void);
extern void ivct6A(void);
extern void ivct6B(void);
extern void ivct6C(void);
extern void ivct6D(void);
extern void ivct6E(void);
extern void ivct6F(void);
extern void ivct70(void);
extern void ivct71(void);
extern void ivct72(void);
extern void ivct73(void);
extern void ivct74(void);
extern void ivct75(void);
extern void ivct76(void);
extern void ivct77(void);
extern void ivct78(void);
extern void ivct79(void);
extern void ivct7A(void);
extern void ivct7B(void);
extern void ivct7C(void);
extern void ivct7D(void);
extern void ivct7E(void);
extern void ivct7F(void);
extern void ivct80(void);
extern void ivct81(void);
extern void ivct82(void);
extern void ivct83(void);
extern void ivct84(void);
extern void ivct85(void);
extern void ivct86(void);
extern void ivct87(void);
extern void ivct88(void);
extern void ivct89(void);
extern void ivct8A(void);
extern void ivct8B(void);
extern void ivct8C(void);
extern void ivct8D(void);
extern void ivct8E(void);
extern void ivct8F(void);
extern void ivct90(void);
extern void ivct91(void);
extern void ivct92(void);
extern void ivct93(void);
extern void ivct94(void);
extern void ivct95(void);
extern void ivct96(void);
extern void ivct97(void);
extern void ivct98(void);
extern void ivct99(void);
extern void ivct9A(void);
extern void ivct9B(void);
extern void ivct9C(void);
extern void ivct9D(void);
extern void ivct9E(void);
extern void ivct9F(void);
extern void ivctA0(void);
extern void ivctA1(void);
extern void ivctA2(void);
extern void ivctA3(void);
extern void ivctA4(void);
extern void ivctA5(void);
extern void ivctA6(void);
extern void ivctA7(void);
extern void ivctA8(void);
extern void ivctA9(void);
extern void ivctAA(void);
extern void ivctAB(void);
extern void ivctAC(void);
extern void ivctAD(void);
extern void ivctAE(void);
extern void ivctAF(void);
extern void ivctB0(void);
extern void ivctB1(void);
extern void ivctB2(void);
extern void ivctB3(void);
extern void ivctB4(void);
extern void ivctB5(void);
extern void ivctB6(void);
extern void ivctB7(void);
extern void ivctB8(void);
extern void ivctB9(void);
extern void ivctBA(void);
extern void ivctBB(void);
extern void ivctBC(void);
extern void ivctBD(void);
extern void ivctBE(void);
extern void ivctBF(void);
extern void ivctCF(void);
extern void ivctD0(void);
extern void ivctD1(void);
extern void ivctD2(void);
extern void ivctD3(void);
extern void ivctD4(void);
extern void ivctD5(void);
extern void ivctD6(void);
extern void ivctD7(void);
extern void ivctD8(void);
extern void ivctD9(void);
extern void ivctDA(void);
extern void ivctDB(void);
extern void ivctDC(void);
extern void ivctDD(void);
extern void ivctDE(void);
extern void ivctDF(void);
extern void ivctE0(void);
extern void ivctE1(void);
extern void ivctE2(void);
extern void ivctE3(void);
extern void ivctE4(void);
extern void ivctE5(void);
extern void ivctE6(void);
extern void ivctE7(void);
extern void ivctE8(void);
extern void ivctE9(void);
extern void ivctEE(void);
extern void ivctF0(void);
extern void ivctF1(void);
extern void ivctF2(void);
extern void ivctF3(void);
extern void ivctSTRAY(void);

void softreset(void);

#define	IO_ROM_INIT	0xfff0
#define	IO_ROM_SEG	0xf000
#define	RESET_FLAG	0x1234
#define	SHUT_DOWN	0x8f
#define	SHUT5		5
#define I8042_RESETCPU  0xfe
#define	STAT_8042	0x64

/*
 * Array of eventflags, one per engine, used in implementation of sendsoft
 */
STATIC volatile uint_t psm_eventflags[MAXNUMCPU];



/*
 * common IDT structure for all engines in Micronics.
 */
struct idt_init apic_idt_init[] = {
	{       31,             GATE_386INT,    ivctSTRAY,      GATE_KACC },
        {       SOFTINT,        GATE_386INT,    softint,        GATE_KACC },
        {       DEVINTS,        GATE_386INT,    devint0,        GATE_KACC },
        {       DEVINTS + 1,    GATE_386INT,    devint1,        GATE_KACC },
        {       DEVINTS + 2,    GATE_386INT,    devint2,        GATE_KACC },
        {       DEVINTS + 3,    GATE_386INT,    devint3,        GATE_KACC },
        {       DEVINTS + 4,    GATE_386INT,    devint4,        GATE_KACC },
        {       DEVINTS + 5,    GATE_386INT,    devint5,        GATE_KACC },
        {       DEVINTS + 6,    GATE_386INT,    devint6,        GATE_KACC },
        {       DEVINTS + 7,    GATE_386INT,    devint7,        GATE_KACC },
        {       DEVINTS + 8,    GATE_386INT,    devint8,        GATE_KACC },
        {       DEVINTS + 9,    GATE_386INT,    devint9,        GATE_KACC },
        {       DEVINTS + 10,   GATE_386INT,    devint10,       GATE_KACC },
        {       DEVINTS + 11,   GATE_386INT,    devint11,       GATE_KACC },
        {       DEVINTS + 12,   GATE_386INT,    devint12,       GATE_KACC },
        {       DEVINTS + 13,   GATE_386INT,    devint13,       GATE_KACC },
        {       DEVINTS + 14,   GATE_386INT,    devint14,       GATE_KACC },
        {       DEVINTS + 15,   GATE_386INT,    devint15,       GATE_KACC },
        {       DEVINTS + 16,   GATE_386INT,    devint16,       GATE_KACC },
        {       DEVINTS + 17,   GATE_386INT,    devint17,       GATE_KACC },
        {       DEVINTS + 18,   GATE_386INT,    devint18,       GATE_KACC },
        {       DEVINTS + 19,   GATE_386INT,    devint19,       GATE_KACC },
        {       DEVINTS + 20,   GATE_386INT,    devint20,       GATE_KACC },
        {       DEVINTS + 21,   GATE_386INT,    devint21,       GATE_KACC },
        {       DEVINTS + 22,   GATE_386INT,    devint22,       GATE_KACC },
        {       DEVINTS + 23,   GATE_386INT,    devint23,       GATE_KACC },
	/* IPL 1 */
        {       APICINTS + 16,  GATE_386INT,    ivct60,         GATE_KACC },
        {       APICINTS + 17,  GATE_386INT,    ivct61,         GATE_KACC },
        {       APICINTS + 18,  GATE_386INT,    ivct62,         GATE_KACC },
        {       APICINTS + 19,  GATE_386INT,    ivct63,         GATE_KACC },
        {       APICINTS + 20,  GATE_386INT,    ivct64,         GATE_KACC },
        {       APICINTS + 21,  GATE_386INT,    ivct65,         GATE_KACC },
        {       APICINTS + 22,  GATE_386INT,    ivct66,         GATE_KACC },
        {       APICINTS + 23,  GATE_386INT,    ivct67,         GATE_KACC },
        {       APICINTS + 24,  GATE_386INT,    ivct68,         GATE_KACC },
        {       APICINTS + 25,  GATE_386INT,    ivct69,         GATE_KACC },
        {       APICINTS + 26,  GATE_386INT,    ivct6A,         GATE_KACC },
        {       APICINTS + 27,  GATE_386INT,    ivct6B,         GATE_KACC },
        {       APICINTS + 28,  GATE_386INT,    ivct6C,         GATE_KACC },
        {       APICINTS + 29,  GATE_386INT,    ivct6D,         GATE_KACC },
        {       APICINTS + 30,  GATE_386INT,    ivct6E,         GATE_KACC },
        {       APICINTS + 31,  GATE_386INT,    ivct6F,         GATE_KACC },
	/* IPL 2 */
        {       APICINTS + 32,  GATE_386INT,    ivct70,         GATE_KACC },
        {       APICINTS + 33,  GATE_386INT,    ivct71,         GATE_KACC },
        {       APICINTS + 34,  GATE_386INT,    ivct72,         GATE_KACC },
        {       APICINTS + 35,  GATE_386INT,    ivct73,         GATE_KACC },
        {       APICINTS + 36,  GATE_386INT,    ivct74,         GATE_KACC },
        {       APICINTS + 37,  GATE_386INT,    ivct75,         GATE_KACC },
        {       APICINTS + 38,  GATE_386INT,    ivct76,         GATE_KACC },
        {       APICINTS + 39,  GATE_386INT,    ivct77,         GATE_KACC },
        {       APICINTS + 40,  GATE_386INT,    ivct78,         GATE_KACC },
        {       APICINTS + 41,  GATE_386INT,    ivct79,         GATE_KACC },
        {       APICINTS + 42,  GATE_386INT,    ivct7A,         GATE_KACC },
        {       APICINTS + 43,  GATE_386INT,    ivct7B,         GATE_KACC },
        {       APICINTS + 44,  GATE_386INT,    ivct7C,         GATE_KACC },
        {       APICINTS + 45,  GATE_386INT,    ivct7D,         GATE_KACC },
        {       APICINTS + 46,  GATE_386INT,    ivct7E,         GATE_KACC },
        {       APICINTS + 47,  GATE_386INT,    ivct7F,         GATE_KACC },
	/* IPL 3 */
        {       APICINTS + 48,  GATE_386INT,    ivct80,         GATE_KACC },
        {       APICINTS + 49,  GATE_386INT,    ivct81,         GATE_KACC },
        {       APICINTS + 50,  GATE_386INT,    ivct82,         GATE_KACC },
        {       APICINTS + 51,  GATE_386INT,    ivct83,         GATE_KACC },
        {       APICINTS + 52,  GATE_386INT,    ivct84,         GATE_KACC },
        {       APICINTS + 53,  GATE_386INT,    ivct85,         GATE_KACC },
        {       APICINTS + 54,  GATE_386INT,    ivct86,         GATE_KACC },
        {       APICINTS + 55,  GATE_386INT,    ivct87,         GATE_KACC },
        {       APICINTS + 56,  GATE_386INT,    ivct88,         GATE_KACC },
        {       APICINTS + 57,  GATE_386INT,    ivct89,         GATE_KACC },
        {       APICINTS + 58,  GATE_386INT,    ivct8A,         GATE_KACC },
        {       APICINTS + 59,  GATE_386INT,    ivct8B,         GATE_KACC },
        {       APICINTS + 60,  GATE_386INT,    ivct8C,         GATE_KACC },
        {       APICINTS + 61,  GATE_386INT,    ivct8D,         GATE_KACC },
        {       APICINTS + 62,  GATE_386INT,    ivct8E,         GATE_KACC },
        {       APICINTS + 63,  GATE_386INT,    ivct8F,         GATE_KACC },
	/* IPL 4 */
        {       APICINTS + 64,  GATE_386INT,    ivct90,         GATE_KACC },
        {       APICINTS + 65,  GATE_386INT,    ivct91,         GATE_KACC },
        {       APICINTS + 66,  GATE_386INT,    ivct92,         GATE_KACC },
        {       APICINTS + 67,  GATE_386INT,    ivct93,         GATE_KACC },
        {       APICINTS + 68,  GATE_386INT,    ivct94,         GATE_KACC },
        {       APICINTS + 69,  GATE_386INT,    ivct95,         GATE_KACC },
        {       APICINTS + 70,  GATE_386INT,    ivct96,         GATE_KACC },
        {       APICINTS + 71,  GATE_386INT,    ivct97,         GATE_KACC },
        {       APICINTS + 72,  GATE_386INT,    ivct98,         GATE_KACC },
        {       APICINTS + 73,  GATE_386INT,    ivct99,         GATE_KACC },
        {       APICINTS + 74,  GATE_386INT,    ivct9A,         GATE_KACC },
        {       APICINTS + 75,  GATE_386INT,    ivct9B,         GATE_KACC },
        {       APICINTS + 76,  GATE_386INT,    ivct9C,         GATE_KACC },
        {       APICINTS + 77,  GATE_386INT,    ivct9D,         GATE_KACC },
        {       APICINTS + 78,  GATE_386INT,    ivct9E,         GATE_KACC },
        {       APICINTS + 79,  GATE_386INT,    ivct9F,         GATE_KACC },
	/* IPL 5 */
        {       APICINTS + 80,  GATE_386INT,    ivctA0,         GATE_KACC },
        {       APICINTS + 81,  GATE_386INT,    ivctA1,         GATE_KACC },
        {       APICINTS + 82,  GATE_386INT,    ivctA2,         GATE_KACC },
        {       APICINTS + 83,  GATE_386INT,    ivctA3,         GATE_KACC },
        {       APICINTS + 84,  GATE_386INT,    ivctA4,         GATE_KACC },
        {       APICINTS + 85,  GATE_386INT,    ivctA5,         GATE_KACC },
        {       APICINTS + 86,  GATE_386INT,    ivctA6,         GATE_KACC },
        {       APICINTS + 87,  GATE_386INT,    ivctA7,         GATE_KACC },
        {       APICINTS + 88,  GATE_386INT,    ivctA8,         GATE_KACC },
        {       APICINTS + 89,  GATE_386INT,    ivctA9,         GATE_KACC },
        {       APICINTS + 90,  GATE_386INT,    ivctAA,         GATE_KACC },
        {       APICINTS + 91,  GATE_386INT,    ivctAB,         GATE_KACC },
        {       APICINTS + 92,  GATE_386INT,    ivctAC,         GATE_KACC },
        {       APICINTS + 93,  GATE_386INT,    ivctAD,         GATE_KACC },
        {       APICINTS + 94,  GATE_386INT,    ivctAE,         GATE_KACC },
        {       APICINTS + 95,  GATE_386INT,    ivctAF,         GATE_KACC },
	/* IPL 6 */
        {       APICINTS + 96,  GATE_386INT,    ivctB0,         GATE_KACC },
        {       APICINTS + 97,  GATE_386INT,    ivctB1,         GATE_KACC },
        {       APICINTS + 98,  GATE_386INT,    ivctB2,         GATE_KACC },
        {       APICINTS + 99,  GATE_386INT,    ivctB3,         GATE_KACC },
        {       APICINTS + 100,  GATE_386INT,   ivctB4,         GATE_KACC },
        {       APICINTS + 101,  GATE_386INT,   ivctB5,         GATE_KACC },
        {       APICINTS + 102,  GATE_386INT,   ivctB6,         GATE_KACC },
        {       APICINTS + 103,  GATE_386INT,   ivctB7,         GATE_KACC },
        {       APICINTS + 104,  GATE_386INT,   ivctB8,         GATE_KACC },
        {       APICINTS + 105,  GATE_386INT,   ivctB9,         GATE_KACC },
        {       APICINTS + 106,  GATE_386INT,   ivctBA,         GATE_KACC },
        {       APICINTS + 107,  GATE_386INT,   ivctBB,         GATE_KACC },
        {       APICINTS + 108,  GATE_386INT,   ivctBC,         GATE_KACC },
        {       APICINTS + 109,  GATE_386INT,   ivctBD,         GATE_KACC },
        {       APICINTS + 110,  GATE_386INT,   ivctBE,         GATE_KACC },
        {       APICINTS + 111,  GATE_386INT,   ivctBF,         GATE_KACC },

        {       APICINTS + 127,  GATE_386INT,   ivctCF,		GATE_KACC },

	/* IPL 8 - all IPL7s are raised to IPL 8 */
        {       APICINTS + 128,  GATE_386INT,   ivctD0,         GATE_KACC },
        {       APICINTS + 129,  GATE_386INT,   ivctD1,         GATE_KACC },
        {       APICINTS + 130,  GATE_386INT,   ivctD2,         GATE_KACC },
        {       APICINTS + 131,  GATE_386INT,   ivctD3,         GATE_KACC },
        {       APICINTS + 132,  GATE_386INT,   ivctD4,         GATE_KACC },
        {       APICINTS + 133,  GATE_386INT,   ivctD5,         GATE_KACC },
        {       APICINTS + 134,  GATE_386INT,   ivctD6,         GATE_KACC },
        {       APICINTS + 135,  GATE_386INT,   ivctD7,         GATE_KACC },
        {       APICINTS + 136,  GATE_386INT,   ivctD8,         GATE_KACC },
        {       APICINTS + 137,  GATE_386INT,   ivctD9,         GATE_KACC },
        {       APICINTS + 138,  GATE_386INT,   ivctDA,         GATE_KACC },
        {       APICINTS + 139,  GATE_386INT,   ivctDB,         GATE_KACC },
        {       APICINTS + 140,  GATE_386INT,   ivctDC,         GATE_KACC },
        {       APICINTS + 141,  GATE_386INT,   ivctDD,         GATE_KACC },
        {       APICINTS + 142,  GATE_386INT,   ivctDE,         GATE_KACC },
        {       APICINTS + 143,  GATE_386INT,   ivctDF,         GATE_KACC },
        {       APICINTS + 144,  GATE_386INT,   ivctE0,         GATE_KACC },
        {       APICINTS + 145,  GATE_386INT,   ivctE1,		GATE_KACC },
        {       APICINTS + 146,  GATE_386INT,   ivctE2,		GATE_KACC },
        {       APICINTS + 147,  GATE_386INT,   ivctE3,		GATE_KACC },
        {       APICINTS + 148,  GATE_386INT,   ivctE4,		GATE_KACC },
        {       APICINTS + 149,  GATE_386INT,   ivctE5,		GATE_KACC },
        {       APICINTS + 150,  GATE_386INT,   ivctE6,		GATE_KACC },
        {       APICINTS + 151,  GATE_386INT,   ivctE7,		GATE_KACC },
        {       APICINTS + 152,  GATE_386INT,   ivctE8,         GATE_KACC },
        {       APICINTS + 153,  GATE_386INT,   ivctE9,         GATE_KACC },
        {       238,            GATE_386INT,    ivctEE,         GATE_KACC },
        {       240,            GATE_386INT,    ivctF0,         GATE_KACC },
        {       241,            GATE_386INT,    ivctF1,         GATE_KACC },
        {       242,            GATE_386INT,    ivctF2,         GATE_KACC },
        {       243,            GATE_386INT,    ivctF3,         GATE_KACC },
        {       255,            GATE_386INT,    ivctSTRAY,      GATE_KACC },
        { 0 },
};

unsigned char mpc_default_1[] = {
	'P','C','M','P',			/* signature */
	0,0, 1, 0,		/* length, spec_rev, checksum */
	0,0,0,0, 0,0,0,0,	/* oem id string */
	0,0,0,0, 0,0,0,0,  0,0,0,0,	/* product id string */
	0,0,0,0,		/* oem table pointer */
	0,0, 2+1+1+16+2,0,		/* entry count */
	0x00,0x00,0xe0,0xfe,	/* address of local apic */
	0,0,0,0,		/* res. */

	MP_ET_PROC,	/* processor */
	0,		/* apic id == 0 */
	1,		/* apic type - 82489DX */
	1,		/* enable */
	0,0,0,0,	/* stepping, model, family, type */
	0,0,0,0,	/* feature flags - not used */
	0,0,0,0, 0,0,0,0,	/* res. */

	MP_ET_PROC,	/* processor */
	1,		/* apic id == 1 */
	1,		/* apic type - 82489DX */
	1,		/* enable */
	0,0,0,0,	/* stepping, model, family, type */
	0,0,0,0,	/* feature flags - not used */
	0,0,0,0, 0,0,0,0,	/* res. */

	MP_ET_BUS,	/* bus */
	0,		/* bus id */
	'I','S','A',' ',
	' ',' ',

	MP_ET_IOAPIC,	/* i/o apic */
	2,		/* apic id */
	1,		/* apic type - 82489DX (not used) */
	1,		/* enabled */
	0x00,0x00,0xc0,0xfe,	/* address of i/o apic */

	MP_ET_I_INTR,3,0,0,	/* PIC */
	0,0,0xff,0,		/* src(0,0) -> all i/o apics, line 0 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,1,2,1,		/* src(0,1) -> i/o apic with id=2, line 1 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,0,2,2,		/* src(0,2) -> i/o apic with id=2, line 2 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,3,2,3,		/* src(0,3) -> i/o apic with id=2, line 3 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,4,2,4,		/* src(0,4) -> i/o apic with id=2, line 4 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,5,2,5,		/* src(0,5) -> i/o apic with id=2, line 5 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,6,2,6,		/* src(0,6) -> i/o apic with id=2, line 6 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,7,2,7,		/* src(0,7) -> i/o apic with id=2, line 7 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,8,2,8,		/* src(0,8) -> i/o apic with id=2, line 8 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,9,2,9,		/* src(0,9) -> i/o apic with id=2, line 9 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,10,2,10,		/* src(0,10) -> i/o apic with id=2, line 10 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,11,2,11,		/* src(0,11) -> i/o apic with id=2, line 11 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,12,2,12,		/* src(0,12) -> i/o apic with id=2, line 12 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,13,2,13,		/* src(0,13) -> i/o apic with id=2, line 13 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,14,2,14,		/* src(0,14) -> i/o apic with id=2, line 14 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,15,2,15,		/* src(0,15) -> i/o apic with id=2, line 15 */

	MP_ET_L_INTR,3,0,0,	/* PIC */
	0,0,0xff,0,		/* src(0,0) -> all local apics, line 0 */
	MP_ET_L_INTR,1,0,0,	/* NMI */
	0,0,0xff,1,		/* src(0,0) -> all local apics, line 1 */
};

unsigned char mpc_default_2[] = {
	'P','C','M','P',			/* signature */
	0,0, 1, 0,		/* length, spec_rev, checksum */
	0,0,0,0, 0,0,0,0,	/* oem id string */
	0,0,0,0, 0,0,0,0,  0,0,0,0,	/* product id string */
	0,0,0,0,		/* oem table pointer */
	0,0, 2+1+1+14+2,0,		/* entry count */
	0x00,0x00,0xe0,0xfe,	/* address of local apic */
	0,0,0,0,		/* res. */

	MP_ET_PROC,	/* processor */
	0,		/* apic id == 0 */
	1,		/* apic type - 82489DX */
	1,		/* enable */
	0,0,0,0,	/* stepping, model, family, type */
	0,0,0,0,	/* feature flags - not used */
	0,0,0,0, 0,0,0,0,	/* res. */

	MP_ET_PROC,	/* processor */
	1,		/* apic id == 1 */
	1,		/* apic type - 82489DX */
	1,		/* enable */
	0,0,0,0,	/* stepping, model, family, type */
	0,0,0,0,	/* feature flags - not used */
	0,0,0,0, 0,0,0,0,	/* res. */

	MP_ET_BUS,	/* bus */
	0,		/* bus id */
	'E','I','S','A',
	' ',' ',

	MP_ET_IOAPIC,	/* i/o apic */
	2,		/* apic id */
	1,		/* apic type - 82489DX (not used) */
	1,		/* enabled */
	0x00,0x00,0xc0,0xfe,	/* address of i/o apic */

	MP_ET_I_INTR,3,0,0,	/* PIC */
	0,0,0xff,0,		/* src(0,0) -> all i/o apics, line 0 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,1,2,1,		/* src(0,1) -> i/o apic with id=2, line 1 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,3,2,3,		/* src(0,3) -> i/o apic with id=2, line 3 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,4,2,4,		/* src(0,4) -> i/o apic with id=2, line 4 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,5,2,5,		/* src(0,5) -> i/o apic with id=2, line 5 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,6,2,6,		/* src(0,6) -> i/o apic with id=2, line 6 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,7,2,7,		/* src(0,7) -> i/o apic with id=2, line 7 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,8,2,8,		/* src(0,8) -> i/o apic with id=2, line 8 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,9,2,9,		/* src(0,9) -> i/o apic with id=2, line 9 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,10,2,10,		/* src(0,10) -> i/o apic with id=2, line 10 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,11,2,11,		/* src(0,11) -> i/o apic with id=2, line 11 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,12,2,12,		/* src(0,12) -> i/o apic with id=2, line 12 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,14,2,14,		/* src(0,14) -> i/o apic with id=2, line 14 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,15,2,15,		/* src(0,15) -> i/o apic with id=2, line 15 */

	MP_ET_L_INTR,3,0,0,	/* PIC */
	0,0,0xff,0,		/* src(0,0) -> all local apics, line 0 */
	MP_ET_L_INTR,1,0,0,	/* NMI */
	0,0,0xff,1,		/* src(0,0) -> all local apics, line 1 */
};

unsigned char mpc_default_3[] = {
	'P','C','M','P',			/* signature */
	0,0, 1, 0,		/* length, spec_rev, checksum */
	0,0,0,0, 0,0,0,0,	/* oem id string */
	0,0,0,0, 0,0,0,0,  0,0,0,0,	/* product id string */
	0,0,0,0,		/* oem table pointer */
	0,0, 2+1+1+16+2,0,		/* entry count */
	0x00,0x00,0xe0,0xfe,	/* address of local apic */
	0,0,0,0,		/* res. */

	MP_ET_PROC,	/* processor */
	0,		/* apic id == 0 */
	1,		/* apic type - 82489DX */
	1,		/* enable */
	0,0,0,0,	/* stepping, model, family, type */
	0,0,0,0,	/* feature flags - not used */
	0,0,0,0, 0,0,0,0,	/* res. */

	MP_ET_PROC,	/* processor */
	1,		/* apic id == 1 */
	1,		/* apic type - 82489DX */
	1,		/* enable */
	0,0,0,0,	/* stepping, model, family, type */
	0,0,0,0,	/* feature flags - not used */
	0,0,0,0, 0,0,0,0,	/* res. */

	MP_ET_BUS,	/* bus */
	0,		/* bus id */
	'E','I','S','A',
	' ',' ',

	MP_ET_IOAPIC,	/* i/o apic */
	2,		/* apic id */
	1,		/* apic type - 82489DX (not used) */
	1,		/* enabled */
	0x00,0x00,0xc0,0xfe,	/* address of i/o apic */

	MP_ET_I_INTR,3,0,0,	/* PIC */
	0,0,0xff,0,		/* src(0,0) -> all i/o apics, line 0 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,1,2,1,		/* src(0,1) -> i/o apic with id=2, line 1 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,0,2,2,		/* src(0,2) -> i/o apic with id=2, line 2 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,3,2,3,		/* src(0,3) -> i/o apic with id=2, line 3 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,4,2,4,		/* src(0,4) -> i/o apic with id=2, line 4 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,5,2,5,		/* src(0,5) -> i/o apic with id=2, line 5 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,6,2,6,		/* src(0,6) -> i/o apic with id=2, line 6 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,7,2,7,		/* src(0,7) -> i/o apic with id=2, line 7 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,8,2,8,		/* src(0,8) -> i/o apic with id=2, line 8 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,9,2,9,		/* src(0,9) -> i/o apic with id=2, line 9 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,10,2,10,		/* src(0,10) -> i/o apic with id=2, line 10 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,11,2,11,		/* src(0,11) -> i/o apic with id=2, line 11 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,12,2,12,		/* src(0,12) -> i/o apic with id=2, line 12 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,13,2,13,		/* src(0,13) -> i/o apic with id=2, line 13 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,14,2,14,		/* src(0,14) -> i/o apic with id=2, line 14 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,15,2,15,		/* src(0,15) -> i/o apic with id=2, line 15 */

	MP_ET_L_INTR,3,0,0,	/* PIC */
	0,0,0xff,0,		/* src(0,0) -> all local apics, line 0 */
	MP_ET_L_INTR,1,0,0,	/* NMI */
	0,0,0xff,1,		/* src(0,0) -> all local apics, line 1 */
};

unsigned char mpc_default_4[] = {
	'P','C','M','P',			/* signature */
	0,0, 1, 0,		/* length, spec_rev, checksum */
	0,0,0,0, 0,0,0,0,	/* oem id string */
	0,0,0,0, 0,0,0,0,  0,0,0,0,	/* product id string */
	0,0,0,0,		/* oem table pointer */
	0,0, 2+1+1+16+2,0,		/* entry count */
	0x00,0x00,0xe0,0xfe,	/* address of local apic */
	0,0,0,0,		/* res. */

	MP_ET_PROC,	/* processor */
	0,		/* apic id == 0 */
	1,		/* apic type - 82489DX */
	1,		/* enable */
	0,0,0,0,	/* stepping, model, family, type */
	0,0,0,0,	/* feature flags - not used */
	0,0,0,0, 0,0,0,0,	/* res. */

	MP_ET_PROC,	/* processor */
	1,		/* apic id == 1 */
	1,		/* apic type - 82489DX */
	1,		/* enable */
	0,0,0,0,	/* stepping, model, family, type */
	0,0,0,0,	/* feature flags - not used */
	0,0,0,0, 0,0,0,0,	/* res. */

	MP_ET_BUS,	/* bus */
	0,		/* bus id */
	'M','C','A',' ',
	' ',' ',

	MP_ET_IOAPIC,	/* i/o apic */
	2,		/* apic id */
	1,		/* apic type - 82489DX (not used) */
	1,		/* enabled */
	0x00,0x00,0xc0,0xfe,	/* address of i/o apic */

	MP_ET_I_INTR,3,0,0,	/* PIC */
	0,0,0xff,0,		/* src(0,0) -> all i/o apics, line 0 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,1,2,1,		/* src(0,1) -> i/o apic with id=2, line 1 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,0,2,2,		/* src(0,2) -> i/o apic with id=2, line 2 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,3,2,3,		/* src(0,3) -> i/o apic with id=2, line 3 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,4,2,4,		/* src(0,4) -> i/o apic with id=2, line 4 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,5,2,5,		/* src(0,5) -> i/o apic with id=2, line 5 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,6,2,6,		/* src(0,6) -> i/o apic with id=2, line 6 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,7,2,7,		/* src(0,7) -> i/o apic with id=2, line 7 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,8,2,8,		/* src(0,8) -> i/o apic with id=2, line 8 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,9,2,9,		/* src(0,9) -> i/o apic with id=2, line 9 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,10,2,10,		/* src(0,10) -> i/o apic with id=2, line 10 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,11,2,11,		/* src(0,11) -> i/o apic with id=2, line 11 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,12,2,12,		/* src(0,12) -> i/o apic with id=2, line 12 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,13,2,13,		/* src(0,13) -> i/o apic with id=2, line 13 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,14,2,14,		/* src(0,14) -> i/o apic with id=2, line 14 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,15,2,15,		/* src(0,15) -> i/o apic with id=2, line 15 */

	MP_ET_L_INTR,3,0,0,	/* PIC */
	0,0,0xff,0,		/* src(0,0) -> all local apics, line 0 */
	MP_ET_L_INTR,1,0,0,	/* NMI */
	0,0,0xff,1,		/* src(0,0) -> all local apics, line 1 */
};

unsigned char mpc_default_5[] = {
	'P','C','M','P',			/* signature */
	0,0, 1, 0,		/* length, spec_rev, checksum */
	0,0,0,0, 0,0,0,0,	/* oem id string */
	0,0,0,0, 0,0,0,0,  0,0,0,0,	/* product id string */
	0,0,0,0,		/* oem table pointer */
	0,0, 2+2+1+16+2,0,		/* entry count */
	0x00,0x00,0xe0,0xfe,	/* address of local apic */
	0,0,0,0,		/* res. */

	MP_ET_PROC,	/* processor */
	0,		/* apic id == 0 */
	0x10,		/* apic type - Integrated */
	1,		/* enable */
	0,0,0,0,	/* stepping, model, family, type */
	0,0,0,0,	/* feature flags - not used */
	0,0,0,0, 0,0,0,0,	/* res. */

	MP_ET_PROC,	/* processor */
	1,		/* apic id == 1 */
	0x10,		/* apic type - Integrated */
	1,		/* enable */
	0,0x20,0,0,	/* stepping, model, family, type=CM */
	0,0,0,0,	/* feature flags - not used */
	0,0,0,0, 0,0,0,0,	/* res. */

	MP_ET_BUS,	/* bus */
	0,		/* bus id */
	'I','S','A',' ',
	' ',' ',

	MP_ET_BUS,	/* bus */
	0,		/* bus id */
	'P','C','I',' ',
	' ',' ',

	MP_ET_IOAPIC,	/* i/o apic */
	2,		/* apic id */
	0x10,		/* apic type - Integrated (not used) */
	1,		/* enabled */
	0x00,0x00,0xc0,0xfe,	/* address of i/o apic */

	MP_ET_I_INTR,3,0,0,	/* PIC */
	0,0,0xff,0,		/* src(0,0) -> all i/o apics, line 0 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,1,2,1,		/* src(0,1) -> i/o apic with id=2, line 1 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,0,2,2,		/* src(0,2) -> i/o apic with id=2, line 2 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,3,2,3,		/* src(0,3) -> i/o apic with id=2, line 3 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,4,2,4,		/* src(0,4) -> i/o apic with id=2, line 4 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,5,2,5,		/* src(0,5) -> i/o apic with id=2, line 5 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,6,2,6,		/* src(0,6) -> i/o apic with id=2, line 6 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,7,2,7,		/* src(0,7) -> i/o apic with id=2, line 7 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,8,2,8,		/* src(0,8) -> i/o apic with id=2, line 8 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,9,2,9,		/* src(0,9) -> i/o apic with id=2, line 9 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,10,2,10,		/* src(0,10) -> i/o apic with id=2, line 10 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,11,2,11,		/* src(0,11) -> i/o apic with id=2, line 11 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,12,2,12,		/* src(0,12) -> i/o apic with id=2, line 12 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,13,2,13,		/* src(0,13) -> i/o apic with id=2, line 13 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,14,2,14,		/* src(0,14) -> i/o apic with id=2, line 14 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,15,2,15,		/* src(0,15) -> i/o apic with id=2, line 15 */

	MP_ET_L_INTR,3,0,0,	/* PIC */
	0,0,0xff,0,		/* src(0,0) -> all local apics, line 0 */
	MP_ET_L_INTR,1,0,0,	/* NMI */
	0,0,0xff,1,		/* src(0,0) -> all local apics, line 1 */
};

unsigned char mpc_default_6[] = {
	'P','C','M','P',			/* signature */
	0,0, 1, 0,		/* length, spec_rev, checksum */
	0,0,0,0, 0,0,0,0,	/* oem id string */
	0,0,0,0, 0,0,0,0,  0,0,0,0,	/* product id string */
	0,0,0,0,		/* oem table pointer */
	0,0, 2+2+1+16+2,0,		/* entry count */
	0x00,0x00,0xe0,0xfe,	/* address of local apic */
	0,0,0,0,		/* res. */

	MP_ET_PROC,	/* processor */
	0,		/* apic id == 0 */
	0x10,		/* apic type - Integrated */
	1,		/* enable */
	0,0,0,0,	/* stepping, model, family, type */
	0,0,0,0,	/* feature flags - not used */
	0,0,0,0, 0,0,0,0,	/* res. */

	MP_ET_PROC,	/* processor */
	1,		/* apic id == 1 */
	0x10,		/* apic type - Integrated */
	1,		/* enable */
	0,0x20,0,0,	/* stepping, model, family, type=CM */
	0,0,0,0,	/* feature flags - not used */
	0,0,0,0, 0,0,0,0,	/* res. */

	MP_ET_BUS,	/* bus */
	0,		/* bus id */
	'E','I','S','A',
	' ',' ',

	MP_ET_BUS,	/* bus */
	0,		/* bus id */
	'P','C','I',' ',
	' ',' ',

	MP_ET_IOAPIC,	/* i/o apic */
	2,		/* apic id */
	0x10,		/* apic type - Integrated (not used) */
	1,		/* enabled */
	0x00,0x00,0xc0,0xfe,	/* address of i/o apic */

	MP_ET_I_INTR,3,0,0,	/* PIC */
	0,0,0xff,0,		/* src(0,0) -> all i/o apics, line 0 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,1,2,1,		/* src(0,1) -> i/o apic with id=2, line 1 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,0,2,2,		/* src(0,2) -> i/o apic with id=2, line 2 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,3,2,3,		/* src(0,3) -> i/o apic with id=2, line 3 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,4,2,4,		/* src(0,4) -> i/o apic with id=2, line 4 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,5,2,5,		/* src(0,5) -> i/o apic with id=2, line 5 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,6,2,6,		/* src(0,6) -> i/o apic with id=2, line 6 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,7,2,7,		/* src(0,7) -> i/o apic with id=2, line 7 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,8,2,8,		/* src(0,8) -> i/o apic with id=2, line 8 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,9,2,9,		/* src(0,9) -> i/o apic with id=2, line 9 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,10,2,10,		/* src(0,10) -> i/o apic with id=2, line 10 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,11,2,11,		/* src(0,11) -> i/o apic with id=2, line 11 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,12,2,12,		/* src(0,12) -> i/o apic with id=2, line 12 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,13,2,13,		/* src(0,13) -> i/o apic with id=2, line 13 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,14,2,14,		/* src(0,14) -> i/o apic with id=2, line 14 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,15,2,15,		/* src(0,15) -> i/o apic with id=2, line 15 */

	MP_ET_L_INTR,3,0,0,	/* PIC */
	0,0,0xff,0,		/* src(0,0) -> all local apics, line 0 */
	MP_ET_L_INTR,1,0,0,	/* NMI */
	0,0,0xff,1,		/* src(0,0) -> all local apics, line 1 */
};

unsigned char mpc_default_7[] = {
	'P','C','M','P',			/* signature */
	0,0, 1, 0,		/* length, spec_rev, checksum */
	0,0,0,0, 0,0,0,0,	/* oem id string */
	0,0,0,0, 0,0,0,0,  0,0,0,0,	/* product id string */
	0,0,0,0,		/* oem table pointer */
	0,0, 2+2+1+15+2,0,		/* entry count */
	0x00,0x00,0xe0,0xfe,	/* address of local apic */
	0,0,0,0,		/* res. */

	MP_ET_PROC,	/* processor */
	0,		/* apic id == 0 */
	0x10,		/* apic type - Integrated */
	1,		/* enable */
	0,0,0,0,	/* stepping, model, family, type */
	0,0,0,0,	/* feature flags - not used */
	0,0,0,0, 0,0,0,0,	/* res. */

	MP_ET_PROC,	/* processor */
	1,		/* apic id == 1 */
	0x10,		/* apic type - Integrated */
	1,		/* enable */
	0,0x20,0,0,	/* stepping, model, family, type */
	0,0,0,0,	/* feature flags - not used */
	0,0,0,0, 0,0,0,0,	/* res. */

	MP_ET_BUS,	/* bus */
	0,		/* bus id */
	'M','C','A',' ',
	' ',' ',

	MP_ET_BUS,	/* bus */
	0,		/* bus id */
	'P','C','I',' ',
	' ',' ',

	MP_ET_IOAPIC,	/* i/o apic */
	2,		/* apic id */
	0x10,		/* apic type - Integrated (not used) */
	1,		/* enabled */
	0x00,0x00,0xc0,0xfe,	/* address of i/o apic */

	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,1,2,1,		/* src(0,1) -> i/o apic with id=2, line 1 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,0,2,2,		/* src(0,2) -> i/o apic with id=2, line 2 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,3,2,3,		/* src(0,3) -> i/o apic with id=2, line 3 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,4,2,4,		/* src(0,4) -> i/o apic with id=2, line 4 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,5,2,5,		/* src(0,5) -> i/o apic with id=2, line 5 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,6,2,6,		/* src(0,6) -> i/o apic with id=2, line 6 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,7,2,7,		/* src(0,7) -> i/o apic with id=2, line 7 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,8,2,8,		/* src(0,8) -> i/o apic with id=2, line 8 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,9,2,9,		/* src(0,9) -> i/o apic with id=2, line 9 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,10,2,10,		/* src(0,10) -> i/o apic with id=2, line 10 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,11,2,11,		/* src(0,11) -> i/o apic with id=2, line 11 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,12,2,12,		/* src(0,12) -> i/o apic with id=2, line 12 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,13,2,13,		/* src(0,13) -> i/o apic with id=2, line 13 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,14,2,14,		/* src(0,14) -> i/o apic with id=2, line 14 */
	MP_ET_I_INTR,0,0,0,	/* INTR */
	0,15,2,15,		/* src(0,15) -> i/o apic with id=2, line 15 */

	MP_ET_L_INTR,3,0,0,	/* PIC */
	0,0,0xff,0,		/* src(0,0) -> all local apics, line 0 */
	MP_ET_L_INTR,1,0,0,	/* NMI */
	0,0,0xff,1,		/* src(0,0) -> all local apics, line 1 */
};

/*	NO_IO_APIC_OPTION	*/
unsigned char mpc_default_8[] = {
	'P','C','M','P',			/* signature */
	0,0, 1, 0,		/* length, spec_rev, checksum */
	0,0,0,0, 0,0,0,0,	/* oem id string */
	0,0,0,0, 0,0,0,0,  0,0,0,0,	/* product id string */
	0,0,0,0,		/* oem table pointer */
	0,0, 2+1+0+0+2,0,		/* entry count */
	0x00,0x00,0xe0,0xfe,	/* address of local apic */
	0,0,0,0,		/* res. */

	MP_ET_PROC,	/* processor */
	0,		/* apic id == 0 */
	0x10,		/* apic type - Integrated */
	1,		/* enable */
	0,0,0,0,	/* stepping, model, family, type */
	0,0,0,0,	/* feature flags - not used */
	0,0,0,0, 0,0,0,0,	/* res. */

	MP_ET_PROC,	/* processor */
	1,		/* apic id == 1 */
	0x10,		/* apic type - Integrated */
	1,		/* enable */
	0,0x20,0,0,	/* stepping, model, family, type=CM */
	0,0,0,0,	/* feature flags - not used */
	0,0,0,0, 0,0,0,0,	/* res. */

	MP_ET_BUS,	/* bus */
	0,		/* bus id */
	'I','S','A',' ',
	' ',' ',

	MP_ET_L_INTR,3,0,0,	/* PIC */
	0,0,0xff,0,		/* src(0,0) -> all local apics, line 0 */
	MP_ET_L_INTR,1,0,0,	/* NMI */
	0,0,0xff,1,		/* src(0,0) -> all local apics, line 1 */
};
/*	NO_IO_APIC_OPTION */


int apic_defaults[] = {
	0,
	(int)mpc_default_1,
	(int)mpc_default_2,
	(int)mpc_default_3,
	(int)mpc_default_4,
	(int)mpc_default_5,
	(int)mpc_default_6,
	(int)mpc_default_7,
	(int)mpc_default_8,
	0
};


#define str(s)  #s
#define xstr(s) str(s)

/*
 * void
 * psm_do_softint(void)
 *
 * Calling/Exit State:
 *      Called if user preemption flag is set.
 *
 * Remarks:
 *      User preemption flag is set if either of the following events
 *      are pending:
 *              - streams service procedures
 *              - local callouts
 *              - global callouts
 *              - runrun flag is set
 */
void
psm_do_softint(void)
{
        asm("int  $" xstr(SOFTINT));
}

/*
 * void
 * psm_configure(void)
 *
 *      Gather platform specific configuration info. for kernel.
 *
 * Calling/Exit State:
 *
 *      Assumes system is only running on the boot processor.
 *      No return value.
 */
void
psm_configure(void)
{
}

/*
 * array to keep track of engine number and APIC id.
 */
extern int apic_eng_id[];

/*
 * int
 * psm_numeng(void)
 *
 * Calling/Exit State:
 *      Returns number of engines available in the system
 *      It is used by conf_proc() to calloc space for engine
 *      structures.
 */
int
psm_numeng(void)
{
        int	i,found;

	/*
	 * Look for PCMP configuration table
	 */
	found = ap_table_ncpus();
        return (found);
}

/*
 * void
 * psm_intr_init(void)
 *
 * Calling/Exit State:
 *      Called when the system is being initialized.
 */
void
psm_intr_init(void)
{
        apicinit();
}

/*
 * void
 * psm_intr_start(void)
 *
 * Calling/Exit State:
 *      Called when the processor is being onlined.
 */
void
psm_intr_start(void)
{
        apicstart();
}

/*
 * void
 * psm_timer_init(void)
 *
 * Calling/Exit State:
 *      None.
 */
void
psm_timer_init(void)
{
        apic_clkstart();
	clkstart();
}

/*
 * int
 * psm_doarg(char *s) 
 *	psm specific boot argument parsing
 *
 * Calling/Exit State:
 *	This routine is called after first checking an argument
 *	against the standard set of supported arguments.  Unknown
 *	(non-standard) arguments will be passed to this routine.
 *
 *	Return Values:	 0 if argument handled
 *			-1 if argument unknown
 */	 
psm_doarg(char *s)
{
	return(-1);		/* unknown argument */
}

void
psm_do_reboot(int flag)
{
	extern int i8042_write(uchar_t, uchar_t);

	DISABLE();
	apic_reset();

	if (myengnum == 0) {
		reboot_prompt(flag);
		drv_usecwait(100);

		softreset();
		i8042_write(STAT_8042, I8042_RESETCPU); /* trigger reboot */
	}
	for(;;) {
		asm("   cli     ");
		asm("   hlt     ");
	}
}


/*
 * void
 * psm_reboot(int)
 *
 * Calling/Exit State:
 *	Intended use for flag values:
 * 		flag == 0	halt the system and wait for user interaction
 * 		flag != 0	automatic reboot, no user interaction required
 */
void
psm_reboot(int flag)
{
	emask_t resp;

	xcall_all(&resp, B_FALSE, psm_do_reboot, (void*)flag);
	psm_do_reboot(flag);
}

extern void nenableint(int, pl_t, int, int, int);
extern void ndisableint(int, pl_t, int, int);

/*
 * void
 * psm_intron(int iv, pl_t level, int intcpu, int intmp, int itype)
 *	Unmask the interrupt vector from the iplmask.
 *
 * Calling/Exit State:
 *	iv is the interrupt request no. that needs to be enabled.
 *	engnum is the engine on which the interrupt must be unmasked.
 *	level is the interrupt priority level of the iv.
 *	itype is the type of interrupt.
 *
 *	mod_iv_lock spin lock is held on entry/exit.
 */
void
psm_intron(int iv, pl_t level, int intcpu, int intmp, int itype)
{
	nenableint(iv, level, intcpu, intmp, itype);
}


/*
 * void
 * psm_introff(int iv, pl_t level, int engnum, int itype)
 *	Mask the interrupt vector in the apic of the engine currently
 *	running on or i/o apic.
 *
 * Calling/Exit State:
 *	iv is the interrupt that needs to be disabled.
 *	engnum is the engine on which the interrupt must be masked.
 *	level is the interrupt priority level of the iv.
 *	itype is the type of interrupt.
 *
 *	mod_iv_lock spin lock is held on entry/exit.
 */
void
psm_introff(int iv, pl_t level, int engnum, int itype)
{
        ndisableint(iv, level, engnum, itype);
}


/*
 * void
 * softreset(void)
 *	Indicate that the subsequent reboot is a "soft" reboot.
 *
 * Calling/Exit State:
 *	None.
 */
void
softreset(void)
{
	/* do soft reboot; only do memory check after power-on */

	*((ulong_t *)&kvpage0[0x467]) = ((ulong_t)IO_ROM_SEG << 16) |
					 (ulong_t)IO_ROM_INIT;
	*((ushort_t *)&kvpage0[0x472]) = RESET_FLAG;

	/* set shutdown flag to reset using int 19 */
	outb(CMOS_ADDR, SHUT_DOWN);
	outb(CMOS_DATA, SHUT5);
}

/*
 * struct idt_init *
 * psm_idt(int engnum)
 *
 * Calling/Exit State:
 *      Returns a pointer to an idt_init table for all the
 *      sub-platform-specific IDT entries for the given engine.
 *
 * Remarks:
 *      This routine must be called before loading the IDTR from selfinit().
 */
struct idt_init *
psm_idt(int engnum)
{
	return(apic_idt_init);
}

/*
 * STATIC void
 * pcmp_set_resetaddr()
 *	Set the starting address of the target engine
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
pcmp_set_resetaddr(reset_code)
{
	struct resetaddr {
		ushort_t  offset;
		ushort_t  segment;
	} start;
	paddr_t addr;
	char    *vector, *source;
	register      i;


	addr = (paddr_t)reset_code;

	/* get the real address seg:offset format */
	start.offset = addr & 0x0f;
	start.segment = (addr >> 4) & 0xFFFF;

	/* 
	 * there is no need to get engnum as an argument for this routine.
	 * the location at which the reset_code address is written to is 
	 * accessiable by all processors.  actually, there will be no 
	 * need to do it for the other processor once it is called.
	 */
	/* now put the address into warm reset vector (40:67) */
	vector = (char *)(KVPAGE0 + CPQ_RESET_VECT);

	/*
	 * copy byte by byte since the reset vector port is
	 * not word aligned
	 */
	source = (char *) &start;
	for (i = 0; i < sizeof(struct resetaddr); i++)
		*vector++ = *source++;

}


/*
 * void
 * psm_online_engine(int engnum, paddr_t startaddr, int flags)
 *
 * Calling/Exit State:
 *	onoff mutex lock is held on entry/exit.
 */
void
psm_online_engine(int engnum, paddr_t startaddr, int flags)
{
	uchar_t		data;
	uchar_t		p1;
	uchar_t		whoami;
	extern int apic_type;
	int		port;

	/*
	 * there is no need to do anything in case of 
  	 * WARM_ONLINE.
	 */
	if (flags == WARM_ONLINE)
		return;

	/*
	 * set the reset address and enable the engine to
	 * access the memory bus.
	 */

	/* set up the starting address of the target cpu */
	/* and write the shutdown status byte */
	/* (only  needed for older apics) */
	pcmp_set_resetaddr(startaddr);
	outb(CMOS_ADDR, 0xf);
	outb(CMOS_DATA, 0xA);

	/* check the starting address */
	/* (only needed for newer apics) */
	if ((startaddr & 0xfff) || startaddr > 0xff000) {
		cmn_err(CE_WARN, "Bad start address %x\n", startaddr);
		return;
	}

	/* hard reset OR init ipi using the APIC */
	apic_run_cpu(engnum, startaddr);
}

/*
 * void
 * psm_selfinit(void)
 *      Performs any necessay per-engine initialization.
 *
 * Calling/Exit State:
 *      Called from selfinit() when the engine is brought online
 *      for the very first time.
 *
 * Note:
 *      The internal on-chip cache is already enabled for the boot engine.
 */
void
psm_selfinit(void)
{
}

/*
 * void
 * psm_misc_init(void)
 *      Performs any necessay per-engine initialization.
 *
 * Calling/Exit State:
 *      Called at the end of selfinit() when the engine is brought online
 *      for the very first time.
 *	Also, called when coming back online.
 */
/* ARGSUSED */
void
psm_misc_init(void)
{
	extern boolean_t soft_sysdump;

	/*
	 * this is to cause a core dump to occur after PANIC.
	 */
	soft_sysdump = B_TRUE;
	apic_resu();
}

/* ARGSUSED */
void
psm_offline_self(void)
{
	pl_t s;

	/* stop taking new interrupts */
	apic_susp();

	/* allow pending interrupts to occur */
	s = spl0();
	splx(s);
}


/*
 * void
 * psm_send_xintr(int)
 *
 * Calling/Exit State:
 *	None.
 */
void
psm_send_xintr(int engnum)
{
	apic_xmsg2(0xEE,engnum);
}

/*
 * Special asm macro to enable interrupts before calling xcall_intr
 */
asm void sti(void)
{
	sti;
}

#pragma	asm partial_optimization sti

/*
 * void
 * psm_aintr(uint vec, uint oldpl, uint edx, uint ecx,
 *               uint eax, uint es, uint ds, uint eip, uint cs)
 *	Interrupt handler for cross-processor interrupt.
 *
 * Calling/Exit State:
 *	this routine is called at PLHI.
 *
 * Description:
 *	The cross-processor interrupt is used to signal soft interrupts
 *	as well as xcall interrupts.  Thus, once psm_aintr determines that
 *	the interrupt was actually a cross-processor interrupt,
 *	it does several steps:
 *		(1) Call xcall_intr to handle a pending xcall if present.
 *		(2) Copy new soft interrupt flags (if any) to engine_evtflags
 *
 * Remarks:
 *	Note that the same interrupt is used for both soft interrupts and
 *	xcall interrupts.  There are a number of races possible, but these
 *	are all handled correctly because both the soft interrupt handler
 *	and the xcall handler are implemented such that they effectively
 *	do nothing if there is nothing to do.
 */
void
psm_aintr(uint vec, uint oldpl, uint edx, uint ecx, uint eax, uint es, uint ds,
		uint eip, uint cs)
{
	extern void xcall_intr(void);


	sti();

	/*
	 * xcall_intr() is the handler for tlb flush.
	 */
	xcall_intr();  
	/*
	 * we use the same interrupt for software interrupt for now.
	 */
	if (psm_eventflags[myengnum] != 0)
		engine_evtflags |= atomic_fnc(&psm_eventflags[myengnum]);
}

/*
 * Handle dma chaining or fp error (intr 13 from old PIC)
 */
void
psm_intr(uint vec, uint oldpl, uint edx, uint ecx, uint eax, uint es, uint ds,
		uint eip, uint cs)
{
        if ((bootinfo.machflags & EISA_IO_BUS) == 0)
		return;		/* dont even do eoi! */

	if (inb(EISA_DMAIS) & 0xEF) {	/* dma chain */
		spl7();
		dma_intr(vec);
	} else {	/* will get trap 16, ignore the interrupt */
		oem_fclex();    /* Assure NDP busy latch is clear */
	}
	apic_eoi13();
}

/*
 * void
 * psm_sendsoft(int engnum, int arg)
 *	Post SW interrupt to (typically) another processor.
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	Post a software interrupt to a processor with the specified flags.
 *	The implementation uses the static array psm_eventflags; each entry
 *	in psm_eventflags corresponds to an engine and acts as a mailbox for
 *	soft interrupts to that engine.  A soft interrupt is sent by:
 *		(1) Atomically or'ing the specified flags into the
 *			psm_eventflags entry for the engine (using
 *			a bus lock prefix with an OR instruction)
 *		(2) Sending a cross-processor interrupt (directly through
 *			psm_send_xintr, not via xcall)
 *
 * Remarks:
 *	Since sendsoft is sometimes called with an fspinlock held, we
 *	can't acquire locks while doing a sendsoft.  Thus, sendsoft does
 *	not use the xcall mechanism, and it uses bus locking rather than
 *	a software lock to update psm_eventflags atomically.
 */

void
psm_sendsoft(int engnum, uint_t arg)
{
	if (engnum == myengnum)
                engine_evtflags |= arg;
        else {
                atomic_or(&psm_eventflags[engnum], arg);
                psm_send_xintr(engnum);
        }
}


/*
 * ulong_t
 * psm_time_get(psmtime_t *)
 *
 * Calling/Exit State:
 *	- save the current time stamp that is opaque to the base kernel
 *	  in psmtime_t.	  
 *
 * Currently not support.
 */

void
psm_time_get(psmtime_t *ptime)
{
	extern ulong_t pit_usec_time(void);

        ptime->pt_lo = pit_usec_time();
        ptime->pt_hi = 0;
}


/*
 * subtracts the time stamp `src' from `dst', and stores the result in `dst'.
 */
void
psm_time_sub(psmtime_t *dst, psmtime_t *src)
{
        dst->pt_lo -= src->pt_lo;
}


/*
 * add the time stamp `src' to `dst', and stores the result in `dst'.
 */
void
psm_time_add(psmtime_t *dst, psmtime_t *src)
{
        dst->pt_lo += src->pt_lo;
}

/*
 * Convert the opaque time stamp to micro second.
 */
void
psm_time_cvt(dl_t *dst, psmtime_t *src)
{
        dst->dl_lop = src->pt_lo;
        dst->dl_hop = (long)src->pt_hi;
}

/*
 * void
 * psm_ledctl(led_request_t, uint_t)
 *
 * Calling/Exit State:
 *      None.
 */
/* ARGSUSED */
void
psm_ledctl(led_request_t req, uint_t led_bits)
{
}

/*
 * boolean_t
 * psm_intrpend(pl_t pl)
 *	Check for pending interrupts above specified priority level.
 *
 * Calling/Exit State:
 *	None.
 * 
 * Description:
 *	Returns B_TRUE (B_FALSE) if there are (are not) any pending
 *	interrupts above the specified priority level.
 *
 * Remarks:
 *	Returns B_FALSE on the assumption that interrupts will be
 *	serviced on another CPU even if this one's busy.
 */
boolean_t
psm_intrpend(pl_t pl)
{

	return B_FALSE;
}

/*
 * void
 * psm_panic()
 *      Dump platform specific information to memory upon system panicing.
 *
 * Calling/Exit State:
 *      None.
 */
void
psm_panic()
{
}
