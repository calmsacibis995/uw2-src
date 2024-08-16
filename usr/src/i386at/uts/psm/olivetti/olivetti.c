/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/olivetti/olivetti.c	1.9"
/*	Copyright (c) 1993 UNIX System Laboratories, Inc. 	*/
/*	  All Rights Reserved                             	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.   	            	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/
/*								*/
/*	Copyright Ing. C. Olivetti & C. S.p.A.			*/


#ifdef _KERNEL_HEADERS
#include <svc/bootinfo.h>
#include <psm/olivetti/olivetti.h>
#include <psm/olivetti/oliapic.h>
#include <io/cram/cram.h>
#include <mem/immu.h>
#include <mem/vm_mdep.h>
#include <mem/vmparam.h>
#include <svc/intr.h>
#include <svc/pic.h>
#include <svc/pit.h>
#include <svc/psm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/processor.h>
#include <util/types.h>
#include <svc/systm.h>

#elif defined(_KERNEL) || defined(_KMEMUSER)

/* this is used in PSK environment */

#include <sys/bootinfo.h>
#include "olivetti.h"
#include "oliapic.h"
#include <sys/cram.h>
#include <sys/immu.h>
#include <sys/vmparam.h>
#include "intr.h"
#include <sys/pic.h>
#include <sys/pit.h>
#include <sys/psm.h>
#include <sys/cmn_err.h>
#include <sys/debug.h>
#include <sys/engine.h>
#include <sys/inline.h>
#include <sys/param.h>
#include <sys/plocal.h>
#include <sys/processor.h>
#include <sys/types.h>
#include <sys/systm.h>
#endif /* _KERNEL_HEADERS */

/*
 * Interrupt entry points. 
 */

#ifdef __STDC__

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
extern void devint24(void);
extern void devint25(void);
extern void devint26(void);
extern void devint27(void);
extern void devint28(void);
extern void devint29(void);
extern void devint30(void);
extern void devint31(void);

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
extern void ivctC0(void);
extern void ivctC1(void);
extern void ivctC2(void);
extern void ivctC3(void);
extern void ivctC4(void);
extern void ivctC5(void);
extern void ivctC6(void);
extern void ivctC7(void);
extern void ivctC8(void);
extern void ivctC9(void);
extern void ivctCA(void);
extern void ivctCB(void);
extern void ivctCC(void);
extern void ivctCD(void);
extern void ivctCE(void);
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
extern void ivctEE(void);
extern void ivctF0(void);
extern void ivctF1(void);
extern void ivctF2(void);
extern void ivctF3(void);
extern void ivctSTRAY(void);

#endif /* __STDC__ */
extern caddr_t physmap(paddr_t, ulong_t, uint_t);

void softreset(void);

#define	IO_ROM_INIT	0xfff0
#define	IO_ROM_SEG	0xf000
#define	RESET_FLAG	0x1234
#define	SHUT_DOWN	0x8f
#define	SHUT5		5
#define	STAT_8042	0x64
#define	TR_REBOOT	0xfe

/*
 * Array of eventflags, one per engine, used in implementation of sendsoft
 */
STATIC volatile uint_t psm_eventflags[MAXNUMCPU];

extern int sanity_clk;


/*
 * common IDT structure for all engines in Micronics.
 */
struct idt_init apic_idt_init[] = {
	{       31,             GATE_386INT,    ivctSTRAY,      GATE_KACC },
/* 63 */{       SOFTINT,        GATE_386INT,    softint,        GATE_KACC },
/* 64 */{       DEVINTS,        GATE_386INT,    devint0,        GATE_KACC },
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
        {       DEVINTS + 24,   GATE_386INT,    devint24,       GATE_KACC },
        {       DEVINTS + 25,   GATE_386INT,    devint25,       GATE_KACC },
        {       DEVINTS + 26,   GATE_386INT,    devint26,       GATE_KACC },
        {       DEVINTS + 27,   GATE_386INT,    devint27,       GATE_KACC },
        {       DEVINTS + 28,   GATE_386INT,    devint28,       GATE_KACC },
        {       DEVINTS + 29,   GATE_386INT,    devint29,       GATE_KACC },
        {       DEVINTS + 30,   GATE_386INT,    devint30,       GATE_KACC },
        {       DEVINTS + 31,   GATE_386INT,    devint31,       GATE_KACC },
        {       APICINTS,       GATE_386INT,    ivct60,         GATE_KACC },
        {       APICINTS + 1,   GATE_386INT,    ivct61,         GATE_KACC },
        {       APICINTS + 2,   GATE_386INT,    ivct62,         GATE_KACC },
        {       APICINTS + 3,   GATE_386INT,    ivct63,         GATE_KACC },
        {       APICINTS + 4,   GATE_386INT,    ivct64,         GATE_KACC },
        {       APICINTS + 5,   GATE_386INT,    ivct65,         GATE_KACC },
        {       APICINTS + 6,   GATE_386INT,    ivct66,         GATE_KACC },
        {       APICINTS + 7,   GATE_386INT,    ivct67,         GATE_KACC },
        {       APICINTS + 8,   GATE_386INT,    ivct68,         GATE_KACC },
        {       APICINTS + 9,   GATE_386INT,    ivct69,         GATE_KACC },
        {       APICINTS + 10,  GATE_386INT,    ivct6A,         GATE_KACC },
        {       APICINTS + 11,  GATE_386INT,    ivct6B,         GATE_KACC },
        {       APICINTS + 12,  GATE_386INT,    ivct6C,         GATE_KACC },
        {       APICINTS + 13,  GATE_386INT,    ivct6D,         GATE_KACC },
        {       APICINTS + 14,  GATE_386INT,    ivct6E,         GATE_KACC },
        {       APICINTS + 15,  GATE_386INT,    ivct6F,         GATE_KACC },
        {       APICINTS + 16,  GATE_386INT,    ivct70,         GATE_KACC },
        {       APICINTS + 17,  GATE_386INT,    ivct71,         GATE_KACC },
        {       APICINTS + 18,  GATE_386INT,    ivct72,         GATE_KACC },
        {       APICINTS + 19,  GATE_386INT,    ivct73,         GATE_KACC },
        {       APICINTS + 20,  GATE_386INT,    ivct74,         GATE_KACC },
        {       APICINTS + 21,  GATE_386INT,    ivct75,         GATE_KACC },
        {       APICINTS + 22,  GATE_386INT,    ivct76,         GATE_KACC },
        {       APICINTS + 23,  GATE_386INT,    ivct77,         GATE_KACC },
        {       APICINTS + 24,  GATE_386INT,    ivct78,         GATE_KACC },
        {       APICINTS + 25,  GATE_386INT,    ivct79,         GATE_KACC },
        {       APICINTS + 26,  GATE_386INT,    ivct7A,         GATE_KACC },
        {       APICINTS + 27,  GATE_386INT,    ivct7B,         GATE_KACC },
        {       APICINTS + 28,  GATE_386INT,    ivct7C,         GATE_KACC },
        {       APICINTS + 29,  GATE_386INT,    ivct7D,         GATE_KACC },
        {       APICINTS + 30,  GATE_386INT,    ivct7E,         GATE_KACC },
        {       APICINTS + 31,  GATE_386INT,    ivct7F,         GATE_KACC },
        {       APICINTS + 32,  GATE_386INT,    ivct80,         GATE_KACC },
        {       APICINTS + 33,  GATE_386INT,    ivct81,         GATE_KACC },
        {       APICINTS + 34,  GATE_386INT,    ivct82,         GATE_KACC },
        {       APICINTS + 35,  GATE_386INT,    ivct83,         GATE_KACC },
        {       APICINTS + 36,  GATE_386INT,    ivct84,         GATE_KACC },
        {       APICINTS + 37,  GATE_386INT,    ivct85,         GATE_KACC },
        {       APICINTS + 38,  GATE_386INT,    ivct86,         GATE_KACC },
        {       APICINTS + 39,  GATE_386INT,    ivct87,         GATE_KACC },
        {       APICINTS + 40,  GATE_386INT,    ivct88,         GATE_KACC },
        {       APICINTS + 41,  GATE_386INT,    ivct89,         GATE_KACC },
        {       APICINTS + 42,  GATE_386INT,    ivct8A,         GATE_KACC },
        {       APICINTS + 43,  GATE_386INT,    ivct8B,         GATE_KACC },
        {       APICINTS + 44,  GATE_386INT,    ivct8C,         GATE_KACC },
        {       APICINTS + 45,  GATE_386INT,    ivct8D,         GATE_KACC },
        {       APICINTS + 46,  GATE_386INT,    ivct8E,         GATE_KACC },
        {       APICINTS + 47,  GATE_386INT,    ivct8F,         GATE_KACC },
        {       APICINTS + 48,  GATE_386INT,    ivct90,         GATE_KACC },
        {       APICINTS + 49,  GATE_386INT,    ivct91,         GATE_KACC },
        {       APICINTS + 50,  GATE_386INT,    ivct92,         GATE_KACC },
        {       APICINTS + 51,  GATE_386INT,    ivct93,         GATE_KACC },
        {       APICINTS + 52,  GATE_386INT,    ivct94,         GATE_KACC },
        {       APICINTS + 53,  GATE_386INT,    ivct95,         GATE_KACC },
        {       APICINTS + 54,  GATE_386INT,    ivct96,         GATE_KACC },
        {       APICINTS + 55,  GATE_386INT,    ivct97,         GATE_KACC },
        {       APICINTS + 56,  GATE_386INT,    ivct98,         GATE_KACC },
        {       APICINTS + 57,  GATE_386INT,    ivct99,         GATE_KACC },
        {       APICINTS + 58,  GATE_386INT,    ivct9A,         GATE_KACC },
        {       APICINTS + 59,  GATE_386INT,    ivct9B,         GATE_KACC },
        {       APICINTS + 60,  GATE_386INT,    ivct9C,         GATE_KACC },
        {       APICINTS + 61,  GATE_386INT,    ivct9D,         GATE_KACC },
        {       APICINTS + 62,  GATE_386INT,    ivct9E,         GATE_KACC },
        {       APICINTS + 63,  GATE_386INT,    ivct9F,         GATE_KACC },
        {       APICINTS + 64,  GATE_386INT,    ivctA0,         GATE_KACC },
        {       APICINTS + 65,  GATE_386INT,    ivctA1,         GATE_KACC },
        {       APICINTS + 66,  GATE_386INT,    ivctA2,         GATE_KACC },
        {       APICINTS + 67,  GATE_386INT,    ivctA3,         GATE_KACC },
        {       APICINTS + 68,  GATE_386INT,    ivctA4,         GATE_KACC },
        {       APICINTS + 69,  GATE_386INT,    ivctA5,         GATE_KACC },
        {       APICINTS + 70,  GATE_386INT,    ivctA6,         GATE_KACC },
        {       APICINTS + 71,  GATE_386INT,    ivctA7,         GATE_KACC },
        {       APICINTS + 72,  GATE_386INT,    ivctA8,         GATE_KACC },
        {       APICINTS + 73,  GATE_386INT,    ivctA9,         GATE_KACC },
        {       APICINTS + 74,  GATE_386INT,    ivctAA,         GATE_KACC },
        {       APICINTS + 75,  GATE_386INT,    ivctAB,         GATE_KACC },
        {       APICINTS + 76,  GATE_386INT,    ivctAC,         GATE_KACC },
        {       APICINTS + 77,  GATE_386INT,    ivctAD,         GATE_KACC },
        {       APICINTS + 78,  GATE_386INT,    ivctAE,         GATE_KACC },
        {       APICINTS + 79,  GATE_386INT,    ivctAF,         GATE_KACC },
        {       APICINTS + 80,  GATE_386INT,    ivctB0,         GATE_KACC },
        {       APICINTS + 81,  GATE_386INT,    ivctB1,         GATE_KACC },
        {       APICINTS + 82,  GATE_386INT,    ivctB2,         GATE_KACC },
        {       APICINTS + 83,  GATE_386INT,    ivctB3,         GATE_KACC },
        {       APICINTS + 84,  GATE_386INT,    ivctB4,         GATE_KACC },
        {       APICINTS + 85,  GATE_386INT,    ivctB5,         GATE_KACC },
        {       APICINTS + 86,  GATE_386INT,    ivctB6,         GATE_KACC },
        {       APICINTS + 87,  GATE_386INT,    ivctB7,         GATE_KACC },
        {       APICINTS + 88,  GATE_386INT,    ivctB8,         GATE_KACC },
        {       APICINTS + 89,  GATE_386INT,    ivctB9,         GATE_KACC },
        {       APICINTS + 90,  GATE_386INT,    ivctBA,         GATE_KACC },
        {       APICINTS + 91,  GATE_386INT,    ivctBB,         GATE_KACC },
        {       APICINTS + 92,  GATE_386INT,    ivctBC,         GATE_KACC },
        {       APICINTS + 93,  GATE_386INT,    ivctBD,         GATE_KACC },
        {       APICINTS + 94,  GATE_386INT,    ivctBE,         GATE_KACC },
        {       APICINTS + 95,  GATE_386INT,    ivctBF,         GATE_KACC },
        {       APICINTS + 96,  GATE_386INT,    ivctC0,         GATE_KACC },
        {       APICINTS + 97,  GATE_386INT,    ivctC1,         GATE_KACC },
        {       APICINTS + 98,  GATE_386INT,    ivctC2,         GATE_KACC },
        {       APICINTS + 99,  GATE_386INT,    ivctC3,         GATE_KACC },
        {       APICINTS + 100, GATE_386INT,    ivctC4,         GATE_KACC },
        {       APICINTS + 101, GATE_386INT,    ivctC5,         GATE_KACC },
        {       APICINTS + 102, GATE_386INT,    ivctC6,         GATE_KACC },
        {       APICINTS + 103, GATE_386INT,    ivctC7,         GATE_KACC },
        {       APICINTS + 104, GATE_386INT,    ivctC8,         GATE_KACC },
        {       APICINTS + 105, GATE_386INT,    ivctC9,         GATE_KACC },
        {       APICINTS + 106, GATE_386INT,    ivctCA,         GATE_KACC },
        {       APICINTS + 107, GATE_386INT,    ivctCB,         GATE_KACC },
        {       APICINTS + 108, GATE_386INT,    ivctCC,         GATE_KACC },
        {       APICINTS + 109, GATE_386INT,    ivctCD,         GATE_KACC },
        {       APICINTS + 110, GATE_386INT,    ivctCE,         GATE_KACC },
        {       APICINTS + 111, GATE_386INT,    ivctCF,         GATE_KACC },
        {       APICINTS + 112, GATE_386INT,    ivctD0,         GATE_KACC },
        {       APICINTS + 113, GATE_386INT,    ivctD1,         GATE_KACC },
        {       APICINTS + 114, GATE_386INT,    ivctD2,         GATE_KACC },
        {       APICINTS + 115, GATE_386INT,    ivctD3,         GATE_KACC },
        {       APICINTS + 116, GATE_386INT,    ivctD4,         GATE_KACC },
        {       APICINTS + 117, GATE_386INT,    ivctD5,         GATE_KACC },
        {       APICINTS + 118, GATE_386INT,    ivctD6,         GATE_KACC },
        {       APICINTS + 119, GATE_386INT,    ivctD7,         GATE_KACC },
        {       APICINTS + 120, GATE_386INT,    ivctD8,         GATE_KACC },
        {       APICINTS + 121, GATE_386INT,    ivctD9,         GATE_KACC },
        {       APICINTS + 122, GATE_386INT,    ivctDA,         GATE_KACC },
        {       APICINTS + 123, GATE_386INT,    ivctDB,         GATE_KACC },
        {       APICINTS + 124, GATE_386INT,    ivctDC,         GATE_KACC },
        {       APICINTS + 125, GATE_386INT,    ivctDD,         GATE_KACC },
        {       APICINTS + 126, GATE_386INT,    ivctDE,         GATE_KACC },
        {       APICINTS + 127, GATE_386INT,    ivctDF,         GATE_KACC },
        {       APICINTS + 128, GATE_386INT,    ivctE0,         GATE_KACC },
        {       APICINTS + 129, GATE_386INT,    ivctE1,         GATE_KACC },
        {       APICINTS + 130, GATE_386INT,    ivctE2,         GATE_KACC },
        {       APICINTS + 131, GATE_386INT,    ivctE3,         GATE_KACC },
        {       238,            GATE_386INT,    ivctEE,         GATE_KACC },
        {       240,            GATE_386INT,    ivctF0,         GATE_KACC },
        {       241,            GATE_386INT,    ivctF1,         GATE_KACC },
        {       242,            GATE_386INT,    ivctF2,         GATE_KACC },
        {       243,            GATE_386INT,    ivctF3,         GATE_KACC },
        {       255,            GATE_386INT,    ivctSTRAY,      GATE_KACC },
        { 0 },
};

static void conf_proc(void);

extern struct engine  *engine;                /* base of engine array */
extern struct engine  *engine_Nengine;        /* end of engine array */
extern int Nengine;
extern int cpurate;

extern ushort_t engine_ctl_port[];		/* see Space.c */
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
 * int
 * psm_numeng(void)
 *
 * Calling/Exit State:
 *	Returns number of engines available in the system.
 *	It is used by conf_proc() to calloc space for engine
 *	structures.
 *
 * Note:
 *	Cannot have missing processor slots.
 */
int
psm_numeng(void)
{
	int	lastslot = 1;
	unchar cpubyte;
	register int olimreg, olisreg;

	/* Get Selective Configuration */
	olimreg = inb (OLIMTRIGGER);
	olisreg = inb (OLISTRIGGER);

	outb (OLIMIMR, OLIMSPL5);
	outb (OLISIMR, OLISSPL5);

	/* Set in Unix High Performance Mode */
	outb (OLICACHECNTR, inb (OLICACHECNTR) | CACHEFLUSHSYN);

	/* Set Selective Configuration for local APICs */
	outb (OLILMTRIGGER, olimreg);
	outb (OLILSTRIGGER, olisreg);

	outb(CMOS_ADDR, OLIBUS_CONF);
        cpubyte = inb(CMOS_DATA);


	/*
	 * Code to detect no. of cpus in the system.
	 */

#ifdef	OLI_DEBUG
	if ((cpubyte & OLIBUS_ENG1_CONF) != OLIBUS_ENG1_CONF) {
		cmn_err(CE_PANIC, "BOOT_CPU not in slot 0");
	}
#endif

	ltopcpu[BOOTENG] = PHYS_ID0;

	if ((cpubyte & OLIBUS_ENG2_CONF) == OLIBUS_ENG2_CONF) {
		ltopcpu[lastslot++] = PHYS_ID1;
#ifdef	OLI_DEBUG
		cmn_err(CE_NOTE, "FOUND CPU in slot 1");
#endif
	}
	if((cpubyte & OLIBUS_ENG3_CONF) == OLIBUS_ENG3_CONF) {
		ltopcpu[lastslot++] = PHYS_ID2;
#ifdef	OLI_DEBUG
		cmn_err(CE_NOTE, "FOUND CPU in slot 2");
#endif
	}
	if ((cpubyte & OLIBUS_ENG4_CONF) == OLIBUS_ENG4_CONF ) {
		ltopcpu[lastslot++] = PHYS_ID3;
#ifdef	OLI_DEBUG
		cmn_err(CE_NOTE, "FOUND CPU in slot 3");
#endif
	}
	return(lastslot);
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
		outb(STAT_8042, TR_REBOOT);
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
 *	Mask the interrupt vector in the iplmask of the engine currently
 *	running on.
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

	*(ulong_t *)(&kvpage0[0x467]) = ((ulong_t)IO_ROM_SEG << 16) |
					 (ulong_t)IO_ROM_INIT;
	*(ushort_t *)(&kvpage0[0x472]) = RESET_FLAG;

	/* set shutdown flag to reset using int 19 */
	outb(CMOS_ADDR, SHUT_DOWN);
	outb(CMOS_DATA, SHUT5);
}

/*
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
 * psm_set_resetaddr(void (*)())
 *	Set the starting address of the target engine
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
psm_set_resetaddr(paddr_t reset_code)
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
	/* now put the address into warm reset vector (40:67) */
	vector = (char *)physmap(RESET_VECT, sizeof(struct resetaddr), KM_SLEEP);

	/*
	 * copy byte by byte since the reset vector port is
	 * not word aligned
	 */
	source = (char *) &start;
	for (i = 0; i < sizeof(struct resetaddr); i++)
		*vector++ = *source++;

	physmap_free(vector, sizeof(struct resetaddr), 0);
}

/*
 * Calling/Exit State:
 *	onoff_mutex lock is held on entry/exit.
 *
 * Remarks:
 *	Set the reset address and enable the engine to
 *	access the memory bus.
 */
void
psm_online_engine(int engnum, paddr_t startaddr, int flag)
{
	int stat;
	int timeout = 10000000;
	uint eng_port;

	if (flag == WARM_ONLINE)
		return;

	ASSERT(flag == COLD_ONLINE);

	/* set up the starting address of the target cpu */
	(void) psm_set_resetaddr(startaddr);

	apic_reset_cpu(LTOCPULU(engnum));

	eng_port = engine_ctl_port[ltopcpu[engnum]];
	stat = inb(eng_port);

	ASSERT((stat & CPU_DISPATCH)==0);

	outb(eng_port, stat|CPU_DISPATCH);

	while (timeout--) {
		stat = inb(eng_port);

		if ((stat & CPU_DISPATCH) == 0) {
#ifdef	OLI_DEBUG
			cmn_err(CE_CONT,
			 "\n5050PSM: Successfully started lcpu n. %d pcpu %d\n",
				engnum, ltopcpu[engnum]);
#endif
			return;		/* cpu is succesfully started */
		}
	}

#ifdef	OLI_DEBUG
	cmn_err(CE_CONT, "\n5050PSM: Read status: 0x%x\n", stat);
#endif
	cmn_err (CE_CONT, "\n5050PSM:: CPU n. %d did not start", engnum);
	/* Should this be done here, or elsewhere ? */
	Nengine--;
}

/* Nothing to do? */

void
psm_selfinit(void)
{
}

void
psm_do_softint(void)
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

/*
 * Note: we are running on an lowest-proirity based interrupt scheme
 * which means that no work to redistribute interrupts has to be done
 * as the remaining CPU(s) will eventually handle any interrupts.
 */
/* ARGSUSED */

void
psm_offline_self(void)
{
	pl_t s;
	apic_susp();
	s = spl0();
	splx(s);
}

/*
 *	Assert the cross-processor interrupt bit in the processor
 *	control port.
 */
void
psm_send_xintr(int engnum)
{
	int pcpunum; /* physical ccpu ID */
#ifdef	HAS_LED
	psm_ledctl(LED_ON, SXCALL_LED);
#endif
	
	pcpunum = LTOCPULU(engnum);
	apic_sendxintr(pcpunum);
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
 * psm_intr(uint vec, uint oldpl, uint edx, uint ecx,
 *               uint eax, uint es, uint ds, uint eip, uint cs)
 * Calling/Exit State:
 *	On the second engine (engine 1), this routine is called at PLHI with
 *	interrupts disabled.  Interrupts must remain disabled until the
 *	cross-processor interrupt is cleared.
 *
 *	On return, the engine is at PLHI with interrupts enabled.
 *
 * Remarks:
 *	This routine is called at PLHI with interrupts enabled.
 *
 *	Note that the same interrupt is used for both soft interrupts and
 *	xcall interrupts.
 *
 *	atomic_fnc is used to read and clear the psm_eventflags entry for
 *	this engine atomically using a bus lock.  A bus lock is used rather
 *	than a software lock for reasons given under Remarks for psm_sendsoft.
 *
 *	The flags in l.eventflags will be processed on the next return from
 *	trap or interrupt which goes to PLBASE.
 */
void
psm_intr(uint vec, uint oldpl, uint edx, uint ecx, uint eax, uint es, uint ds,
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
	engine_evtflags |= atomic_fnc(&psm_eventflags[myengnum]);

}

/*
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
