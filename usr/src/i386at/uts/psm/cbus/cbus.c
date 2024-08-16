/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/cbus/cbus.c	1.16"
#ident	"$Header: $"

/*
 * AT PSM/mHAL hardware-specific routines and data.
 *
 * This file contains a set of subroutines and a set of data, all of which
 * are used elsewhere in the MP kernel but which vary for MP hardware from
 * different vendors. In general, all the routines with an "psm_" prefix
 * should be defined for whatever hardware you use, even if the routine does
 * nothing. The same applies for data (unfortunately the data names may not
 * have a distinctive prefix), even if it might not be used. Some routines
 * and data defined in this file are for specific platforms only. Vendors
 * do not need to support these items for their own hardware.
 *
 * The requirements for each item in this file are clearly stated in the
 * PSM/mHAL interface specification document.
 *
 * This MP PSM/mHAL file supports both the Corollary Architecture.
 */

#include <io/cram/cram.h>
#include <io/prf/prf.h>
#include <mem/immu.h>
#include <mem/vm_mdep.h>
#include <mem/vmparam.h>
#include <svc/bootinfo.h>
#include <svc/corollary.h>
#include <svc/eisa.h>
#include <svc/intr.h>
#include <svc/pic.h>
#include <svc/pit.h>
#include <svc/psm.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/processor.h>
#include <util/types.h>

#include <io/f_ddi.h>
#include <io/ddi.h>	/* Must come after other kernel headers */

/*
 * Interrupt entry points. 
 */

#define SOFTINT	62

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
extern void devint63(void);
extern void devint96(void);
extern void devint97(void);
extern void devint98(void);
extern void devint99(void);
extern void devint100(void);
extern void devint101(void);
extern void devint102(void);
extern void devint103(void);
extern void devint104(void);
extern void devint105(void);
extern void devint106(void);
extern void devint107(void);
extern void devint108(void);
extern void devint109(void);
extern void devint110(void);
extern void devint111(void);
extern void devint112(void);
extern void devint113(void);
extern void devint114(void);
extern void devint115(void);
extern void devint116(void);
extern void devint117(void);
extern void devint118(void);
extern void devint119(void);
extern void devint120(void);
extern void devint121(void);
extern void devint122(void);
extern void devint123(void);
extern void devint124(void);
extern void devint125(void);
extern void devint126(void);
extern void devint127(void);
extern void devint128(void);
extern void devint129(void);
extern void devint130(void);
extern void devint131(void);
extern void devint132(void);
extern void devint133(void);
extern void devint134(void);
extern void devint135(void);
extern void devint136(void);
extern void devint137(void);
extern void devint138(void);
extern void devint139(void);
extern void devint140(void);
extern void devint141(void);
extern void devint142(void);
extern void devint143(void);
extern void devint144(void);
extern void devint145(void);
extern void devint146(void);
extern void devint147(void);
extern void devint148(void);
extern void devint149(void);
extern void devint150(void);
extern void devint151(void);
extern void devint152(void);
extern void devint153(void);
extern void devint154(void);
extern void devint155(void);
extern void devint156(void);
extern void devint157(void);
extern void devint158(void);
extern void devint159(void);
extern void devint160(void);
extern void devint161(void);
extern void devint162(void);
extern void devint163(void);
extern void devint164(void);
extern void devint165(void);
extern void devint166(void);
extern void devint167(void);
extern void devint168(void);
extern void devint169(void);
extern void devint170(void);
extern void devint171(void);
extern void devint172(void);
extern void devint173(void);
extern void devint174(void);
extern void devint175(void);
extern void devint176(void);
extern void devint177(void);
extern void devint178(void);
extern void devint179(void);
extern void devint180(void);
extern void devint181(void);
extern void devint182(void);
extern void devint183(void);
extern void devint184(void);
extern void devint185(void);
extern void devint186(void);
extern void devint187(void);
extern void devint188(void);
extern void devint189(void);
extern void devint190(void);
extern void devint191(void);
extern void devint192(void);
extern void devint193(void);
extern void devint194(void);
extern void devint195(void);
extern void devint196(void);
extern void devint197(void);
extern void devint198(void);
extern void devint199(void);
extern void devint200(void);
extern void devint201(void);
extern void devint202(void);
extern void devint203(void);
extern void devint204(void);
extern void devint205(void);
extern void devint206(void);
extern void devint207(void);
extern void devint208(void);
extern void devint209(void);
extern void devint210(void);
extern void devint211(void);
extern void devint212(void);
extern void devint213(void);
extern void devint214(void);
extern void devint215(void);
extern void devint216(void);
extern void devint217(void);
extern void devint218(void);
extern void devint219(void);
extern void devint220(void);
extern void devint221(void);
extern void devint222(void);
extern void devint223(void);
extern void devint224(void);
extern void devint225(void);
extern void devint226(void);
extern void devint227(void);
extern void devint228(void);
extern void devint229(void);
extern void devint230(void);
extern void devint231(void);
extern void devint232(void);
extern void devint233(void);
extern void devint234(void);
extern void devint235(void);
extern void devint236(void);
extern void devint237(void);
extern void devint238(void);
extern void devint239(void);
extern void devint240(void);
extern void devint241(void);
extern void devint242(void);
extern void devint243(void);
extern void devint244(void);
extern void devint245(void);
extern void devint246(void);
extern void devint247(void);
extern void devint248(void);
extern void devint249(void);
extern void devint250(void);
extern void devint251(void);
extern void devint252(void);
extern void devint253(void);
extern void devint254(void);
extern void devint255(void);


extern void corollary_reboot();

#define CI_INTS		0

/*
 * P0 IDT (interrupt distribution table) initialization.
 */
/*
 * The following table does not fill in every idt entry.  The idt
 * entry offsets are specified in the first column.  The idt contains
 * four different kinds of entries: 8259 interrupts, APIC/CBC interrupts,
 * the 486/SCSI interrupt, and the soft interrupt.  The DEVINTS variable
 * corresponds to PIC_VECTBASE and should probably use that value instead
 * of having a new define.  The 486/SCSI interrupt is hard coded to 0x3f.
 *
 * Since clocks are IRQ 0, clocks are assigned in the various platform
 * specific modules to use the first entry of their interrupt grouping
 * (i.e. CBUS-I uses devint0, CBUS-2 uses devint63.  The code in intr_p.s
 * that handles MPSTATS profiling will need to take into account the
 * fact that the different platforms use different entry points for the
 * clock.
 */
struct idt_init corollary_idt_init[] = {
	{	SOFTINT,	GATE_386INT,	softint,	GATE_KACC },
	{	DEVINTS,	GATE_386INT,	devint0,	GATE_KACC },
	{	DEVINTS + 1,	GATE_386INT,	devint1,	GATE_KACC },
	{	DEVINTS + 2,	GATE_386INT,	devint2,	GATE_KACC },
	{	DEVINTS + 3,	GATE_386INT,	devint3,	GATE_KACC },
	{	DEVINTS + 4,	GATE_386INT,	devint4,	GATE_KACC },
	{	DEVINTS + 5,	GATE_386INT,	devint5,	GATE_KACC },
	{	DEVINTS + 6,	GATE_386INT,	devint6,	GATE_KACC },
	{	DEVINTS + 7,	GATE_386INT,	devint7,	GATE_KACC },
	{	DEVINTS + 8,	GATE_386INT,	devint8,	GATE_KACC },
	{	DEVINTS + 9,	GATE_386INT,	devint9,	GATE_KACC },
	{	DEVINTS + 10,	GATE_386INT,	devint10,	GATE_KACC },
	{	DEVINTS + 11,	GATE_386INT,	devint11,	GATE_KACC },
	{	DEVINTS + 12,	GATE_386INT,	devint12,	GATE_KACC },
	{	DEVINTS + 13,	GATE_386INT,	devint13,	GATE_KACC },
	{	DEVINTS + 14,	GATE_386INT,	devint14,	GATE_KACC },
	{	DEVINTS + 15,	GATE_386INT,	devint15,	GATE_KACC },
	{	CI_INTS + 63,	GATE_386INT,	devint63,	GATE_KACC },
	{	CI_INTS + 96,	GATE_386INT,	devint96,	GATE_KACC },
	{	CI_INTS + 97,	GATE_386INT,	devint97,	GATE_KACC },
	{	CI_INTS + 98,	GATE_386INT,	devint98,	GATE_KACC },
	{	CI_INTS + 99,	GATE_386INT,	devint99,	GATE_KACC },
	{	CI_INTS + 100,	GATE_386INT,	devint100,	GATE_KACC },
	{	CI_INTS + 101,	GATE_386INT,	devint101,	GATE_KACC },
	{	CI_INTS + 102,	GATE_386INT,	devint102,	GATE_KACC },
	{	CI_INTS + 103,	GATE_386INT,	devint103,	GATE_KACC },
	{	CI_INTS + 104,	GATE_386INT,	devint104,	GATE_KACC },
	{	CI_INTS + 105,	GATE_386INT,	devint105,	GATE_KACC },
	{	CI_INTS + 106,	GATE_386INT,	devint106,	GATE_KACC },
	{	CI_INTS + 107,	GATE_386INT,	devint107,	GATE_KACC },
	{	CI_INTS + 108,	GATE_386INT,	devint108,	GATE_KACC },
	{	CI_INTS + 109,	GATE_386INT,	devint109,	GATE_KACC },
	{	CI_INTS + 110,	GATE_386INT,	devint110,	GATE_KACC },
	{	CI_INTS + 111,	GATE_386INT,	devint111,	GATE_KACC },
	{	CI_INTS + 112,	GATE_386INT,	devint112,	GATE_KACC },
	{	CI_INTS + 113,	GATE_386INT,	devint113,	GATE_KACC },
	{	CI_INTS + 114,	GATE_386INT,	devint114,	GATE_KACC },
	{	CI_INTS + 115,	GATE_386INT,	devint115,	GATE_KACC },
	{	CI_INTS + 116,	GATE_386INT,	devint116,	GATE_KACC },
	{	CI_INTS + 117,	GATE_386INT,	devint117,	GATE_KACC },
	{	CI_INTS + 118,	GATE_386INT,	devint118,	GATE_KACC },
	{	CI_INTS + 119,	GATE_386INT,	devint119,	GATE_KACC },
	{	CI_INTS + 120,	GATE_386INT,	devint120,	GATE_KACC },
	{	CI_INTS + 121,	GATE_386INT,	devint121,	GATE_KACC },
	{	CI_INTS + 122,	GATE_386INT,	devint122,	GATE_KACC },
	{	CI_INTS + 123,	GATE_386INT,	devint123,	GATE_KACC },
	{	CI_INTS + 124,	GATE_386INT,	devint124,	GATE_KACC },
	{	CI_INTS + 125,	GATE_386INT,	devint125,	GATE_KACC },
	{	CI_INTS + 126,	GATE_386INT,	devint126,	GATE_KACC },
	{	CI_INTS + 127,	GATE_386INT,	devint127,	GATE_KACC },
	{	CI_INTS + 128,	GATE_386INT,	devint128,	GATE_KACC },
	{	CI_INTS + 129,	GATE_386INT,	devint129,	GATE_KACC },
	{	CI_INTS + 130,	GATE_386INT,	devint130,	GATE_KACC },
	{	CI_INTS + 131,	GATE_386INT,	devint131,	GATE_KACC },
	{	CI_INTS + 132,	GATE_386INT,	devint132,	GATE_KACC },
	{	CI_INTS + 133,	GATE_386INT,	devint133,	GATE_KACC },
	{	CI_INTS + 134,	GATE_386INT,	devint134,	GATE_KACC },
	{	CI_INTS + 135,	GATE_386INT,	devint135,	GATE_KACC },
	{	CI_INTS + 136,	GATE_386INT,	devint136,	GATE_KACC },
	{	CI_INTS + 137,	GATE_386INT,	devint137,	GATE_KACC },
	{	CI_INTS + 138,	GATE_386INT,	devint138,	GATE_KACC },
	{	CI_INTS + 139,	GATE_386INT,	devint139,	GATE_KACC },
	{	CI_INTS + 140,	GATE_386INT,	devint140,	GATE_KACC },
	{	CI_INTS + 141,	GATE_386INT,	devint141,	GATE_KACC },
	{	CI_INTS + 142,	GATE_386INT,	devint142,	GATE_KACC },
	{	CI_INTS + 143,	GATE_386INT,	devint143,	GATE_KACC },
	{	CI_INTS + 144,	GATE_386INT,	devint144,	GATE_KACC },
	{	CI_INTS + 145,	GATE_386INT,	devint145,	GATE_KACC },
	{	CI_INTS + 146,	GATE_386INT,	devint146,	GATE_KACC },
	{	CI_INTS + 147,	GATE_386INT,	devint147,	GATE_KACC },
	{	CI_INTS + 148,	GATE_386INT,	devint148,	GATE_KACC },
	{	CI_INTS + 149,	GATE_386INT,	devint149,	GATE_KACC },
	{	CI_INTS + 150,	GATE_386INT,	devint150,	GATE_KACC },
	{	CI_INTS + 151,	GATE_386INT,	devint151,	GATE_KACC },
	{	CI_INTS + 152,	GATE_386INT,	devint152,	GATE_KACC },
	{	CI_INTS + 153,	GATE_386INT,	devint153,	GATE_KACC },
	{	CI_INTS + 154,	GATE_386INT,	devint154,	GATE_KACC },
	{	CI_INTS + 155,	GATE_386INT,	devint155,	GATE_KACC },
	{	CI_INTS + 156,	GATE_386INT,	devint156,	GATE_KACC },
	{	CI_INTS + 157,	GATE_386INT,	devint157,	GATE_KACC },
	{	CI_INTS + 158,	GATE_386INT,	devint158,	GATE_KACC },
	{	CI_INTS + 159,	GATE_386INT,	devint159,	GATE_KACC },
	{	CI_INTS + 160,	GATE_386INT,	devint160,	GATE_KACC },
	{	CI_INTS + 161,	GATE_386INT,	devint161,	GATE_KACC },
	{	CI_INTS + 162,	GATE_386INT,	devint162,	GATE_KACC },
	{	CI_INTS + 163,	GATE_386INT,	devint163,	GATE_KACC },
	{	CI_INTS + 164,	GATE_386INT,	devint164,	GATE_KACC },
	{	CI_INTS + 165,	GATE_386INT,	devint165,	GATE_KACC },
	{	CI_INTS + 166,	GATE_386INT,	devint166,	GATE_KACC },
	{	CI_INTS + 167,	GATE_386INT,	devint167,	GATE_KACC },
	{	CI_INTS + 168,	GATE_386INT,	devint168,	GATE_KACC },
	{	CI_INTS + 169,	GATE_386INT,	devint169,	GATE_KACC },
	{	CI_INTS + 170,	GATE_386INT,	devint170,	GATE_KACC },
	{	CI_INTS + 171,	GATE_386INT,	devint171,	GATE_KACC },
	{	CI_INTS + 172,	GATE_386INT,	devint172,	GATE_KACC },
	{	CI_INTS + 173,	GATE_386INT,	devint173,	GATE_KACC },
	{	CI_INTS + 174,	GATE_386INT,	devint174,	GATE_KACC },
	{	CI_INTS + 175,	GATE_386INT,	devint175,	GATE_KACC },
	{	CI_INTS + 176,	GATE_386INT,	devint176,	GATE_KACC },
	{	CI_INTS + 177,	GATE_386INT,	devint177,	GATE_KACC },
	{	CI_INTS + 178,	GATE_386INT,	devint178,	GATE_KACC },
	{	CI_INTS + 179,	GATE_386INT,	devint179,	GATE_KACC },
	{	CI_INTS + 180,	GATE_386INT,	devint180,	GATE_KACC },
	{	CI_INTS + 181,	GATE_386INT,	devint181,	GATE_KACC },
	{	CI_INTS + 182,	GATE_386INT,	devint182,	GATE_KACC },
	{	CI_INTS + 183,	GATE_386INT,	devint183,	GATE_KACC },
	{	CI_INTS + 184,	GATE_386INT,	devint184,	GATE_KACC },
	{	CI_INTS + 185,	GATE_386INT,	devint185,	GATE_KACC },
	{	CI_INTS + 186,	GATE_386INT,	devint186,	GATE_KACC },
	{	CI_INTS + 187,	GATE_386INT,	devint187,	GATE_KACC },
	{	CI_INTS + 188,	GATE_386INT,	devint188,	GATE_KACC },
	{	CI_INTS + 189,	GATE_386INT,	devint189,	GATE_KACC },
	{	CI_INTS + 190,	GATE_386INT,	devint190,	GATE_KACC },
	{	CI_INTS + 191,	GATE_386INT,	devint191,	GATE_KACC },
	{	CI_INTS + 192,	GATE_386INT,	devint192,	GATE_KACC },
	{	CI_INTS + 193,	GATE_386INT,	devint193,	GATE_KACC },
	{	CI_INTS + 194,	GATE_386INT,	devint194,	GATE_KACC },
	{	CI_INTS + 195,	GATE_386INT,	devint195,	GATE_KACC },
	{	CI_INTS + 196,	GATE_386INT,	devint196,	GATE_KACC },
	{	CI_INTS + 197,	GATE_386INT,	devint197,	GATE_KACC },
	{	CI_INTS + 198,	GATE_386INT,	devint198,	GATE_KACC },
	{	CI_INTS + 199,	GATE_386INT,	devint199,	GATE_KACC },
	{	CI_INTS + 200,	GATE_386INT,	devint200,	GATE_KACC },
	{	CI_INTS + 201,	GATE_386INT,	devint201,	GATE_KACC },
	{	CI_INTS + 202,	GATE_386INT,	devint202,	GATE_KACC },
	{	CI_INTS + 203,	GATE_386INT,	devint203,	GATE_KACC },
	{	CI_INTS + 204,	GATE_386INT,	devint204,	GATE_KACC },
	{	CI_INTS + 205,	GATE_386INT,	devint205,	GATE_KACC },
	{	CI_INTS + 206,	GATE_386INT,	devint206,	GATE_KACC },
	{	CI_INTS + 207,	GATE_386INT,	devint207,	GATE_KACC },
	{	CI_INTS + 208,	GATE_386INT,	devint208,	GATE_KACC },
	{	CI_INTS + 209,	GATE_386INT,	devint209,	GATE_KACC },
	{	CI_INTS + 210,	GATE_386INT,	devint210,	GATE_KACC },
	{	CI_INTS + 211,	GATE_386INT,	devint211,	GATE_KACC },
	{	CI_INTS + 212,	GATE_386INT,	devint212,	GATE_KACC },
	{	CI_INTS + 213,	GATE_386INT,	devint213,	GATE_KACC },
	{	CI_INTS + 214,	GATE_386INT,	devint214,	GATE_KACC },
	{	CI_INTS + 215,	GATE_386INT,	devint215,	GATE_KACC },
	{	CI_INTS + 216,	GATE_386INT,	devint216,	GATE_KACC },
	{	CI_INTS + 217,	GATE_386INT,	devint217,	GATE_KACC },
	{	CI_INTS + 218,	GATE_386INT,	devint218,	GATE_KACC },
	{	CI_INTS + 219,	GATE_386INT,	devint219,	GATE_KACC },
	{	CI_INTS + 220,	GATE_386INT,	devint220,	GATE_KACC },
	{	CI_INTS + 221,	GATE_386INT,	devint221,	GATE_KACC },
	{	CI_INTS + 222,	GATE_386INT,	devint222,	GATE_KACC },
	{	CI_INTS + 223,	GATE_386INT,	devint223,	GATE_KACC },
	{	CI_INTS + 224,	GATE_386INT,	devint224,	GATE_KACC },
	{	CI_INTS + 225,	GATE_386INT,	devint225,	GATE_KACC },
	{	CI_INTS + 226,	GATE_386INT,	devint226,	GATE_KACC },
	{	CI_INTS + 227,	GATE_386INT,	devint227,	GATE_KACC },
	{	CI_INTS + 228,	GATE_386INT,	devint228,	GATE_KACC },
	{	CI_INTS + 229,	GATE_386INT,	devint229,	GATE_KACC },
	{	CI_INTS + 230,	GATE_386INT,	devint230,	GATE_KACC },
	{	CI_INTS + 231,	GATE_386INT,	devint231,	GATE_KACC },
	{	CI_INTS + 232,	GATE_386INT,	devint232,	GATE_KACC },
	{	CI_INTS + 233,	GATE_386INT,	devint233,	GATE_KACC },
	{	CI_INTS + 234,	GATE_386INT,	devint234,	GATE_KACC },
	{	CI_INTS + 235,	GATE_386INT,	devint235,	GATE_KACC },
	{	CI_INTS + 236,	GATE_386INT,	devint236,	GATE_KACC },
	{	CI_INTS + 237,	GATE_386INT,	devint237,	GATE_KACC },
	{	CI_INTS + 238,	GATE_386INT,	devint238,	GATE_KACC },
	{	CI_INTS + 239,	GATE_386INT,	devint239,	GATE_KACC },
        {       CI_INTS + 240,  GATE_386INT,    devint240,      GATE_KACC },
        {       CI_INTS + 241,  GATE_386INT,    devint241,      GATE_KACC },
        {       CI_INTS + 242,  GATE_386INT,    devint242,      GATE_KACC },
        {       CI_INTS + 243,  GATE_386INT,    devint243,      GATE_KACC },
        {       CI_INTS + 244,  GATE_386INT,    devint244,      GATE_KACC },
        {       CI_INTS + 245,  GATE_386INT,    devint245,      GATE_KACC },
        {       CI_INTS + 246,  GATE_386INT,    devint246,      GATE_KACC },
        {       CI_INTS + 247,  GATE_386INT,    devint247,      GATE_KACC },
        {       CI_INTS + 248,  GATE_386INT,    devint248,      GATE_KACC },
        {       CI_INTS + 249,  GATE_386INT,    devint249,      GATE_KACC },
        {       CI_INTS + 250,  GATE_386INT,    devint250,      GATE_KACC },
        {       CI_INTS + 251,  GATE_386INT,    devint251,      GATE_KACC },
        {       CI_INTS + 252,  GATE_386INT,    devint252,      GATE_KACC },
        {       CI_INTS + 253,  GATE_386INT,    devint253,      GATE_KACC },
        {       CI_INTS + 254,  GATE_386INT,    devint254,      GATE_KACC },
        {       CI_INTS + 255,  GATE_386INT,    devint255,      GATE_KACC },
	{ 0 },
};


/*
 * TODO:
 *	Should be extendible to four processor without any
 *	changes, except in the psm.cf/Space.c file.
 */

#define	IO_ROM_INIT	0xfff0
#define	IO_ROM_SEG	0xf000
#define	RESET_FLAG	0x1234
#define	SHUT_DOWN	0x8f
#define	SHUT5		5
#define	STAT_8042	0x64

void softreset(void);

extern int sanity_clk;
extern struct idt_init *idt_init;
extern void (*ivect[])();	/* interrupt routines */
extern uchar_t iplmask[];

extern uchar_t intpri[];	/* priority levels for interrupts */
extern int intcpu[];

/*
 * defined in psm.cf/Space.c
 */
extern ushort_t engine_ctl_port[];
extern ushort_t engine_ivec_port[];
extern pl_t plsti;

volatile int	wait_cpus[MAXACPUS];
volatile int	startup_sync[MAXACPUS];

unsigned	spin_count = 10000;
unsigned	corollary_early_boot = 1;

/*
 * Array of eventflags, one per engine, used in implementation of sendsoft
 */
volatile uint_t psm_eventflags[MAXNUMCPU];

/*
 * P0 IDT (interrupt distribution table) initialization.
 */
extern struct idt_init corollary_idt_init[];

/*
 * This flag has two functions.  The first function is in the
 * startup code where this flag is used to determine whether the
 * processors are going to be brought up at startup time (486 SCSI)
 * or whether they will be brought up at online time (psradm).
 *
 * The second function of this variable is to control whether or
 * not the spin lock routines poll for cross processor interrupts.
 * This flag is used for CBUS-I machines that may have to block
 * off all interrupts during a critical section.
 *
 * The PSM sets this flag, so this is set on a per platform basis
 * (i.e. Cbus-I or Cbus-II.
 */
extern int corollary;

#define str(s)  #s
#define xstr(s) str(s)

void
psm_do_softint(void)
{
        asm("int  $" xstr(SOFTINT));
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
/* ARGSUSED */
int
psm_doarg(char *s)
{
	return -1;		/* unknown argument */
}

/*
 * void
 * psm_reboot(int)
 *
 * Calling/Exit State:
 *	Intended use for flag values:
 *		flag == 0	halt the system and wait for user interaction
 *		flag != 0	automatic reboot, no user interaction required
 */
void
psm_reboot(int flag)
{
	splhi();

	/* if sanity timer in use, turn off to allow clean soft reboot */
	if ((bootinfo.machflags & EISA_IO_BUS) && sanity_clk)
		outb(SANITY_CHECK, RESET_SANITY);

	reboot_prompt(flag);

	softreset();

	if (myengnum != 0)
	{
		corollary_cpuintr(0, corollary_reboot, 0);
	}
	else
	{
		corollary_reboot();
	}

	for (;;)
		continue;
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
 * psm_idt(uint_t engnum)
 *
 * Calling/Exit State:
 *      Returns a pointer to an idt_init table for all the
 *      sub-platform-specific IDT entries for the given engine.
 *
 * Remarks:
 *	This routine must be called before init_desc_tables() is called
 *	from selfinit().
 *
 * Note:
 *	It is also called very early in the system startup while setting
 *	up the tmp_init_desc_tables().
 */
/* ARGSUSED */
struct idt_init *
psm_idt(int engnum)
{
	return corollary_idt_init;
}


/*
 * void
 * psm_intr_init(void)
 *	Initialize programmable interrupt controller.
 *
 * Calling/Exit State:
 *	Called when the system is being initialized.
 */
void
psm_intr_init(void)
{
	if (IS_BOOT_ENG(myengnum))
	{
		/*
		 * Initialize the i8259 based interrupt controller
		 */
		picinit();
		return;
	}

	corollary_xcall_init();
}


/*
 * void
 * psm_intr_start(void)
 *	Enable interrupts.
 *
 * Calling/Exit State:
 *	Called from selfinit() when the engine is being onlined. 
 *
 * TODO:
 *	picstart() must be changed to take logical engnum as an argument.
 */
void
psm_intr_start(void)
{
	corollary_intr_init();

        /*
         * If this is an early boot then the additional cpu
         * turned on interrupts in psm_selfinit, to allow the
         * processor to get interrupts before it was online.
         */
        if ((IS_BOOT_ENG(myengnum)) || (corollary_early_boot == 0))
                picstart();

        asm("sti");     /* ENABLE */
}

/*
 * int
 * psm_numeng(void)
 *
 * Calling/Exit State:
 *	Returns number of engines available in the system
 *	It is used by conf_proc() to calloc space for engine
 *	structures.
 */
int
psm_numeng(void)
{
	return corollary_num_cpus;
}


/*
 * void
 * psm_configure(void)
 *	Gather miscellaneous hardware configuration information.
 *
 * Calling/Exit State:
 *	None.
 */
void
psm_configure(void)
{
	if (corollary_pres() == 0)
	{
		cmn_err(CE_WARN, "Not a Corollary Architecture Machine\n");
		return;
	}

	corollary_findcpus();

	corollary_setup();

	corollary_initialize_cpu(myengnum);
}


/*
 * void
 * psm_online_engine(int, paddr_t, int)
 *
 * Calling/Exit State:
 *	onoff_mutex lock is held on entry/exit.
 *
 * Remarks:
 *	Set the reset address and enable the engine to
 *	access the memory bus.
 */
/* ARGSUSED */
void
psm_online_engine(int engnum, paddr_t startaddr, int flag)
{
	if (flag == WARM_ONLINE)
		return;

	if (corollary_early_boot == 0) {
		corollary_online_engine(engnum, startaddr);
	} else {
                /*
		 * If corollary_early_boot is set then the processor
		 * is already running and in the kernel.  Just reset
		 * the flag that the processor is spinning on, and it
		 * will fall through into the kernel (idle).
		 */
		startup_sync[engnum] = 0;
	}
}

/*
 * void
 * psm_online_self(void)
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	Do any necessary work to turn on LEDs or redistribute interrupts.
 */
void
psm_online_self(void)
{
}


/*
 * void
 * psm_offline_self(void)
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	Do any necessary work to turn off LEDs or redistribute interrupts.
 */
void
psm_offline_self(void)
{
}

/*
 * void
 * psm_clear_xintr(int)
 *	Clear the cross-processor interrupt bit in the processor
 *	control port.
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	Should only be called by an engine to clear its own cross-processor
 *	interrupt.
 */
void
psm_clear_xintr(int engnum)
{
	ASSERT(engnum == myengnum);

	corollary_clr_intr(engnum);
}


/*
 * void
 * psm_send_xintr(int)
 *	Assert the cross-processor interrupt bit in the processor
 *	control port.
 *
 * Calling/Exit State:
 *	None.
 */
void
psm_send_xintr(int engnum)
{
	corollary_set_intr(engnum);
}

/*
 * void
 * psm_intr(uint vec, uint oldpl, uint edx, uint ecx,
 *               uint eax, uint es, uint ds, uint eip, uint cs)
 *	Interrupt handler for cross-processor interrupts and
 *	floating-point coprocessor interrupts. It distinguishes
 *	between the two by checking the PINT bit in processor
 *	control port and then calls the appropriate handler.
 *
 * Calling/Exit State:
 *	On the second engine (engine 1), this routine is called at PLHI with
 *	interrupts disabled.  Interrupts must remain disabled until the
 *	cross-processor interrupt is cleared.
 *
 *	On return, the engine is at PLHI with interrupts enabled.
 *
 * Description:
 *	The cross-processor interrupt is used to signal soft interrupts
 *	as well as xcall interrupts.  Thus, once psm_intr determines that
 *	the interrupt was actually a cross-processor interrupt (rather
 *	than a fpu interrupt), it does several steps:
 *		(1) Clear the interrupt bit in the control register
 *		(2) Enable interrupts at the CPU
 *		(3) Copy new soft interrupt flags (if any) to engine_evtflags
 *		(4) Call xcall_intr to handle a pending xcall if present.
 *		(5) Handle profiling if that was signalled.
 *		(6) Handle local clock processing if that was signalled.
 *
 * Remarks:
 *	On engine 0, the routine is called at PLHI with interrupts enabled;
 *	otherwise it is called with interrupts DISABLEd.
 *
 *	Note that the same interrupt is used for both soft interrupts and
 *	xcall interrupts.  There are a number of races possible, but these
 *	are all handled correctly because both the soft interrupt handler
 *	and the xcall handler are implemented such that they effectively
 *	do nothing if there is nothing to do.  See xcall_intr in xcall.c
 *	for more details.
 *
 *	atomic_fnc is used to read and clear the psm_eventflags entry for
 *	this engine atomically using a bus lock.  A bus lock is used rather
 *	than a software lock for reasons given under Remarks for psm_sendsoft.
 *
 *	The flags in engine_evtflags will be processed on the next return from
 *	trap or interrupt which goes to PLBASE.
 */
/* ARGSUSED */
void
psm_intr(uint vec, uint oldpl, uint edx, uint ecx, uint eax, uint es, uint ds,
		uint eip, uint cs)
{
	corollary_intr(oldpl, &eax, eip, cs);
}


/*
 * void
 * psm_sendsoft(int engnum, uint_t arg)
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
 * void
 * psm_timer_init(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
psm_timer_init(void)
{
	corollary_timer_init();
}


/*
 * ulong_t
 * psm_time_get(psmtime_t *)
 *
 * Calling/Exit State:
 *	- save the current time stamp that is opaque to the base kernel
 *	  in psmtime_t.	  
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
 * psm_misc_init(void)
 *      Performs any necessay per-engine initialization.
 *
 * Calling/Exit State:
 *      Called from selfinit() when the engine is brought online
 *      for the very first time.
 *
 */
/* ARGSUSED */
void
psm_misc_init(void)
{
	extern boolean_t soft_sysdump;
	extern int corollary_nmi(void);

	/*
	 * avoid register corollary_nmi for multiple times.
	 */
	if (IS_BOOT_ENG(myengnum)) {
		soft_sysdump = B_TRUE;
		drv_callback(NMI_ATTACH, corollary_nmi, NULL);
	}
}

/*
 * void
 * psm_intron(int iv, pl_t level, int engnum, int intmp, int itype)
 *	Unmask the interrupt vector from the iplmask.
 *
 * Calling/Exit State:
 *	iv is the interrupt request no. that needs to be enabled.
 *	engnum is the engine on which the interrupt must be unmasked.
 *      intmp is the flag that indicates if the driver is multithreaded or not.
 *      itype is the interrupt type.
 *
 *      mod_iv_lock spin lock is held on entry/exit.
 *
 * Remarks:
 *      intcpu          intmp
 *       -1               1             Bind it to cpu 0   (Handled here)
 *       !-1              1             Bind it to cpu !-1 (Ignore intmp)
 *       !-1              0             Bind it to cpu !-1 (Ignore intmp)
 *       -1               0             Bind it to cpu 0   (DLM handles it
 *                                                          by passing the
 *                                                          correct engnum)
 *
 * Remarks:
 *	The code is similar to ndisableint()
 */
/* ARGSUSED */
void
psm_intron(int iv, pl_t level, int engnum, int intmp, int itype)
{
	nenableint(iv, level, engnum, intmp, itype);
}


/*
 * void
 * psm_introff(int iv, int engnum)
 *	Mask the interrupt vector in the iplmask of the engine currently
 *	running on.
 *
 * Calling/Exit State:
 *	iv is the interrupt that needs to be disabled
 *	engnum is the engine on which the interrupt must be masked.
 *      level is the interrupt priority level of the iv.
 *      itype is the interrupt type.
 *
 *      mod_iv_lock spin lock is held on entry/exit.
 *
 * Remarks:
 *	The code is similar to ndisableint()
 */
/* ARGSUSED */
void
psm_introff(int iv, pl_t level, int engnum, int itype)
{
	ndisableint(iv, level, engnum, itype);
}


/*
 * void
 * psm_ledctl(led_request_t, uint_t)
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	Noop for Syspro and Syspro/XL.
 */
/* ARGSUSED */
void
psm_ledctl(led_request_t req, uint_t led_bits)
{
	if (req == LED_OFF)
		corollary_ledoff(myengnum);
	else
		corollary_ledon(myengnum);
}


/*
 * void
 * psm_selfinit(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
psm_selfinit(void)
{
	int		i;
	engine_t	*eng;
	long		*kl1pt;
	extern void	reset_code(void);
	extern paddr_t	online_kl1pt;
	extern ulong_t	online_engno;
	unsigned	check_count;

	/*
	 * corollary_early_boot is set on a per platform basis.
	 * this flag controls whether the machine will boot all
	 * of the processors at startup, or wait until psradm
	 * to start the processors.
	 */
	if (corollary_early_boot == 0)
	{
		if (IS_BOOT_ENG(myengnum))
		{
			corollary_xcall_init();
			return;
		}

		corollary_initialize_cpu(myengnum);

		return;
	}

	/*
         * The additional processors (not the boot processor) will
         * execute inside of this if statement.
         */
	if (!IS_BOOT_ENG(myengnum))
	{
		wait_cpus[myengnum] = 0;
		startup_sync[myengnum] = 1;

		/*
		 * Initialize this processor's data - done ONCE by each
		 * processor.  Typically, this processor's local interrupt
		 * controller is initialized here.
		 */
		corollary_initialize_cpu(myengnum);

		picstart();

		/*
                 * Allow the hardware address translation layer to
                 * do its thing.
                 */
		hat_online();

		asm("sti");     /* ENABLE */

		/*
                 * At this point, interrupts are enabled and the hat
                 * is turned on.  This means that the additional processors
                 * can get interrupts even though they do not actuall
                 * run processes.  This early start code is here specifically
                 * for the 486/SCSI.
                 */
		corollary_init();

		/*
                 * When the processors are truely "online" (psradm),
                 * this flag will be reset and the processors will fall
                 * through to handle processes.
                 */
		while (startup_sync[myengnum])
			continue;

		/*
                 * Turn on the dispatcher to all this processor to
                 * run processes.
                 */
		disponline(&engine[myengnum]);

		return;
	}

	/*
         * If we are here then we are running on the boot (CPU 0)
         * processor.
         */
	corollary_xcall_init();

	/*
         * see selfinit for why this flag has to be temporarily set.
         */
	upyet = 1;

	/*
         * Loop through all of the processors that aren't booted and
         * boot them.
         */
	for (i=1 ; i < Nengine ; i++)
	{
		check_count = 0;

		eng = &engine[i];

		online_engno = i;

		kl1pt = (void *)physmap((paddr_t)&online_kl1pt, 
			sizeof(paddr_t), KM_NOSLEEP);

		*kl1pt = vtop((caddr_t)&eng->e_local->pp_kl1pt[0][0], NULL);

		wait_cpus[i] = 1;

		corollary_online_engine(i, (paddr_t)reset_code);

		while (wait_cpus[i] && (check_count <= spin_count))
		{
			drv_usecwait(2000);
			check_count++;
		}

		if (wait_cpus[i])
		{
			cmn_err(CE_WARN, "CPU %d failed to start up", i);
		}
#ifdef COROLLARY_DEBUG
		else
		{
			cmn_err(CE_NOTE, "CPU %d started", i);
		}
#endif
	}
	upyet = 0;
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
 *	On processors with pics, picipl is generally set to the ipl
 *	of the highest priority pending interrupt on this processor;
 *	if picipl == PLBASE, then there are no pending interrupts on
 *	this processor.
 *
 *	On processors without pics, picipl is set to ipl.
 *
 *	This routine returns TRUE only if both:
 *		(a) this processor has a pic (implied by plsti != 0), and
 *		(b) picipl on this processor is greater than the incoming
 *			pl (implies deferred interrupts)
 */
boolean_t
psm_intrpend(pl_t pl)
{
	extern pl_t plsti, picipl;

	return (boolean_t) ((plsti != 0) && (picipl > pl));
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
