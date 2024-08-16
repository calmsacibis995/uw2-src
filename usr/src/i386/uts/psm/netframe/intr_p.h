/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:psm/netframe/intr_p.h	1.1"
#ident	"$Header: $"

#ifndef _SVC_INTR_P_H	/* wrapper symbol for kernel use */
#define _SVC_INTR_P_H	/* subject to change without notice */

/*
 * Interrupt entry points. 
 */

#ifdef _KERNEL

#define SOFTINT 63              /* software interrupt */

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
extern void devint32(void);
extern void devint33(void);
extern void devint34(void);
extern void devint35(void);
extern void devint36(void);
extern void devint37(void);
extern void devint38(void);
extern void devint39(void);
extern void devint40(void);
extern void devint41(void);
extern void devint42(void);
extern void devint43(void);
extern void devint44(void);
extern void devint45(void);
extern void devint46(void);
extern void devint47(void);
extern void devint48(void);
extern void devint49(void);
extern void devint50(void);
extern void devint51(void);
extern void devint52(void);
extern void devint53(void);
extern void devint54(void);
extern void devint55(void);
extern void devint56(void);
extern void devint57(void);
extern void devint58(void);
extern void devint59(void);
extern void devint60(void);
extern void devint61(void);
extern void devint62(void);
extern void devint63(void);
extern void devint64(void);
extern void devint65(void);
extern void devint66(void);
extern void devint67(void);
extern void devint68(void);
extern void devint69(void);
extern void devint70(void);
extern void devint71(void);

extern void ivct60(void), ivct61(void), ivct62(void), ivct63(void);
extern void ivct64(void), ivct65(void), ivct66(void), ivct67(void);
extern void ivct68(void), ivct69(void), ivct6A(void), ivct6B(void);
extern void ivct6C(void), ivct6D(void), ivct6E(void), ivct6F(void);
extern void ivct70(void), ivct71(void), ivct72(void), ivct73(void);
extern void ivct74(void), ivct75(void), ivct76(void), ivct77(void);
extern void ivct78(void), ivct79(void), ivct7A(void), ivct7B(void);
extern void ivct7C(void), ivct7D(void), ivct7E(void), ivct7F(void);
extern void ivct80(void), ivct81(void), ivct82(void), ivct83(void);
extern void ivct84(void), ivct85(void), ivct86(void), ivct87(void);
extern void ivct88(void), ivct89(void), ivct8A(void), ivct8B(void);
extern void ivct8C(void), ivct8D(void), ivct8E(void), ivct8F(void);
extern void ivct90(void), ivct91(void), ivct92(void), ivct93(void);
extern void ivct94(void), ivct95(void), ivct96(void), ivct97(void);
extern void ivct98(void), ivct99(void), ivct9A(void), ivct9B(void);
extern void ivct9C(void), ivct9D(void), ivct9E(void), ivct9F(void);
extern void ivctA0(void), ivctA1(void), ivctA2(void), ivctA3(void);
extern void ivctA4(void), ivctA5(void), ivctA6(void), ivctA7(void);
extern void ivctA8(void), ivctA9(void), ivctAA(void), ivctAB(void);
extern void ivctAC(void), ivctAD(void), ivctAE(void), ivctAF(void);
extern void ivctB0(void), ivctB1(void), ivctB2(void), ivctB3(void);
extern void ivctB4(void), ivctB5(void), ivctB6(void), ivctB7(void);
extern void ivctB8(void), ivctB9(void), ivctBA(void), ivctBB(void);
extern void ivctBC(void), ivctBD(void), ivctBE(void), ivctBF(void);
extern void ivctC0(void), ivctC1(void), ivctC2(void), ivctC3(void);
extern void ivctC4(void), ivctC5(void), ivctC6(void), ivctC7(void);
extern void ivctC8(void), ivctC9(void), ivctCA(void), ivctCB(void);
extern void ivctCC(void), ivctCD(void), ivctCE(void), ivctCF(void);
extern void ivctD0(void), ivctD1(void), ivctD2(void), ivctD3(void);
extern void ivctD4(void), ivctD5(void), ivctD6(void), ivctD7(void);
extern void ivctD8(void), ivctD9(void), ivctDA(void), ivctDB(void);
extern void ivctDC(void), ivctDD(void), ivctDE(void), ivctDF(void);
extern void ivctE0(void), ivctE1(void), ivctE2(void), ivctE3(void);
extern void ivctE4(void), ivctE5(void), ivctE6(void), ivctE7(void);
extern void ivctE8(void), ivctE9(void), ivctEA(void), ivctEB(void);
extern void ivctEC(void), ivctED(void), ivctEE(void), ivctEF(void);
extern void ivctF0(void);
extern void ivctF1(void);
extern void ivctF2(void);
extern void ivctF3(void);
extern void ivctSTRAY(void);
extern void apic_ivctSTRAY(void);

#endif /* __STDC__ */

#endif /* _KERNEL */

#endif /* _SVC_INTR_P_H */
