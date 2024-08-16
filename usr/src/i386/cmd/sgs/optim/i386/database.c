/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)optim:i386/database.c	1.10"
#include "sched.h"	
/* fields in the structure are:
** 1. pairability type.
** 2. size of the operands.
** 3. for integer opcodes: is the opcode unexpected (1).
**                         or does it have a prefix (2).
**    for fp opcodes: concurency.
** 4. what peepholes can be executed on this opcode.
**                             like for add (1)
**                             like for inc (2)
**                             like for cmp (3)
** 5. floating point flags.
*/
struct data_base opcodata[] = {
	/* CALL */	{WD2,LoNG,0,0,99},
	/* LCALL */	{WD2,LoNG,0,0,99},
	/* RET */	{X86,LoNG,0,0,1},
	/* LRET */	{X86,LoNG,0,0,1},
	/* JMP */	{WD2,LoNG,0,0,1},
	/* LJMP */	{X86,LoNG,0,0,1},
	/* JA */	{WD2,LoNG,0,0,1},
	/* JAE */	{WD2,LoNG,0,0,1},
	/* JB */	{WD2,LoNG,0,0,1},
	/* JBE */	{WD2,LoNG,0,0,1},
	/* JC */	{WD2,LoNG,0,0,1},
	/* JCXZ */	{WD2,LoNG,0,0,1},
	/* JE */	{WD2,LoNG,0,0,1},
	/* JG */	{WD2,LoNG,0,0,1},
	/* JGE */	{WD2,LoNG,0,0,1},
	/* JL */	{WD2,LoNG,0,0,1},
	/* JLE */	{WD2,LoNG,0,0,1},
	/* JNA */	{WD2,LoNG,0,0,1},
	/* JNAE */	{WD2,LoNG,0,0,1},
	/* JNB */	{WD2,LoNG,0,0,1},
	/* JNBE */	{WD2,LoNG,0,0,1},
	/* JNC */	{WD2,LoNG,0,0,1},
	/* JNE */	{WD2,LoNG,0,0,1},
	/* JNG */	{WD2,LoNG,0,0,1},
	/* JNGE */	{WD2,LoNG,0,0,1},
	/* JNL */	{WD2,LoNG,0,0,1},
	/* JNLE */	{WD2,LoNG,0,0,1},
	/* JNO */	{WD2,LoNG,0,0,1},
	/* JNP */	{WD2,LoNG,0,0,1},
	/* JNS */	{WD2,LoNG,0,0,1},
	/* JNZ */	{WD2,LoNG,0,0,1},
	/* JO */	{WD2,LoNG,0,0,1},
	/* JP */	{WD2,LoNG,0,0,1},
	/* JPE */	{WD2,LoNG,0,0,1},
	/* JPO */	{WD2,LoNG,0,0,1},
	/* JS */	{WD2,LoNG,0,0,1},
	/* JZ */	{WD2,LoNG,0,0,1},
	/* LOOP */	{WD2,LoNG,0,0,1},
	/* LOOPE */	{WD2,LoNG,0,0,1},
	/* LOOPNE */	{WD2,LoNG,0,0,1},
	/* LOOPNZ */	{WD2,LoNG,0,0,1},
	/* LOOPZ */	{WD2,LoNG,0,0,1},
	/* REP */	{X86,LoNG,0,0,1},
	/* REPNZ */	{X86,LoNG,0,0,1},
	/* REPZ */	{X86,LoNG,0,0,1},
	/* AAA */	{X86,LoNG,0,0,3},
	/* AAD */	{X86,LoNG,0,0,14},
	/* AAM */	{X86,LoNG,0,0,15},
	/* AAS */	{X86,LoNG,0,0,3},
	/* DAA */	{X86,LoNG,0,0,2},
	/* DAS */	{X86,LoNG,0,0,2},
	/* ADCB */	{WD1,ByTE,0,0,1 | M1A1 | M2A2},
	/* ADCW */	{WD1,WoRD,0,0,1 | M1A1 | M2A2},
	/* ADCL */	{WD1,LoNG,0,0,1 | M1A1 | M2A2},
	/* ADDB */	{WDA,ByTE,0,1,1 | M1A1 | M2A2},
	/* ADDW */	{WD1,WoRD,2,0,1 | M1A1 | M2A2},
	/* ADDL */	{WDA,LoNG,0,1,1 | M1A1 | M2A2},
	/* DECB */	{WDA,ByTE,0,2,1 | M1A2},
	/* DECW */	{WD1,WoRD,2,0,1 | M1A2},
	/* DECL */	{WDA,LoNG,0,2,1 | M1A2},
	/* DIVB */	{X86,ByTE,0,0,16},
	/* DIVW */	{X86,WoRD,2,0,24},
	/* DIVL */	{X86,LoNG,0,0,40},
	/* IDIVB */	{X86,ByTE,0,0,19},
	/* IDIVW */	{X86,WoRD,2,0,27},
	/* IDIVL */	{X86,LoNG,0,0,43},
	/* IMULB */	{X86,ByTE,2,0,13},
	/* IMULW */	{X86,WoRD,2,0,13},
	/* IMULL */	{X86,LoNG,2,0,13},
	/* INCB */	{WDA,ByTE,0,2,1 | M1A2},
	/* INCW */	{WD1,WoRD,2,0,1 | M1A2},
	/* INCL */	{WDA,LoNG,0,2,1 | M1A2},
	/* MULB */	{X86,ByTE,0,0,13},
	/* MULW */	{X86,WoRD,2,0,13},
	/* MULL */	{X86,LoNG,0,0,13},
	/* NEGB */	{X86,ByTE,0,2,1 | M1A1},
	/* NEGW */	{X86,WoRD,2,0,1 | M1A1},
	/* NEGL */	{X86,LoNG,0,2,1 | M1A1},
	/* SBBB */	{WD1,ByTE,0,0,1 | M1A1 | M2A2},
	/* SBBW */	{WD1,WoRD,0,0,1 | M1A1 | M2A2},
	/* SBBL */	{WD1,LoNG,0,0,1 | M1A1 | M2A2},
	/* SUBB */	{WDA,ByTE,0,1,1 | M1A1 | M2A2},
	/* SUBW */	{WD1,WoRD,2,0,1 | M1A1 | M2A2},
	/* SUBL */	{WDA,LoNG,0,1,1 | M1A1 | M2A2},
	/* ANDB */	{WDA,ByTE,0,1,1 | M1A1 | M2A2},
	/* ANDW */	{WD1,WoRD,2,0,1 | M1A1 | M2A2},
	/* ANDL */	{WDA,LoNG,0,1,1 | M1A1 | M2A2},
	/* ORB */	{WDA,ByTE,0,1,1 | M1A1 | M2A2},
	/* ORW */	{WD1,WoRD,2,0,1 | M1A1 | M2A2},
	/* ORL */	{WDA,LoNG,0,1,1 | M1A1 | M2A2},
	/* XORB */	{WDA,ByTE,0,1,1 | M1A1 | M2A2},
	/* XORW */	{WD1,WoRD,2,0,1 | M1A1 | M2A2},
	/* XORL */	{WDA,LoNG,0,1,1 | M1A1 | M2A2},
	/* CLRB */	{WDA,ByTE,0,0,3 | M1A1},
	/* CLRW */	{WD1,WoRD,2,0,3 | M1A1},
	/* CLRL */	{WDA,LoNG,0,0,3 | M1A1},
	/* RCLB */	{X86,ByTE,0,0,3 | M1A1},
	/* RCLW */	{X86,WoRD,2,0,3 | M1A1},
	/* RCLL */	{X86,LoNG,0,0,3 | M1A1},
	/* RCRB */	{X86,ByTE,0,0,3 | M1A1},
	/* RCRW */	{X86,WoRD,2,0,3 | M1A1},
	/* RCRL */	{X86,LoNG,0,0,3 | M1A1},
	/* ROLB */	{X86,ByTE,0,0,3 | M1A1},
	/* ROLW */	{X86,WoRD,2,0,3 | M1A1},
	/* ROLL */	{X86,LoNG,0,0,3 | M1A1},
	/* RORB */	{X86,ByTE,0,0,3 | M1A1},
	/* RORW */	{X86,WoRD,2,0,3 | M1A1},
	/* RORL */	{X86,LoNG,0,0,3 | M1A1},
	/* SALB */	{X86,ByTE,0,0,3 | M1A1},
	/* SALW */	{X86,WoRD,2,0,3 | M1A1},
	/* SALL */	{X86,LoNG,0,0,3 | M1A1},
	/* SARB */	{X86,ByTE,0,0,3 | M1A1},
	/* SARW */	{X86,WoRD,2,0,3 | M1A1},
	/* SARL */	{X86,LoNG,0,0,3 | M1A1},
	/* SHLB */	{X86,ByTE,0,0,3 | M1A1},
	/* SHLW */	{X86,WoRD,2,0,3 | M1A1},
	/* SHLL */	{X86,LoNG,0,0,3 | M1A1},
	/* SHRB */	{X86,ByTE,0,0,2 | M1A1},
	/* SHRW */	{X86,WoRD,2,0,2 | M1A1},
	/* SHRL */	{X86,LoNG,0,0,2 | M1A1},
	/* SHLDW */	{X86,WoRD,2,0,2 | M1A1},
	/* SHLDL */	{WDA,LoNG,2,0,2 | M1A1},
	/* SHRDW */	{WD1,WoRD,2,0,2 | M1A1},
	/* SHRDL */	{WDA,LoNG,2,0,2 | M1A1},
	/* CMPB */	{WDA,ByTE,0,3,1 | M1A1 | M2A1},
	/* CMPW */	{WD1,WoRD,2,0,1 | M1A1 | M2A1},
	/* CMPL */	{WDA,LoNG,0,3,1 | M1A1 | M2A1},
	/* TESTB */	{WDA,ByTE,0,3,1 | M1A1 | M2A1},
	/* TESTW */	{WD1,WoRD,2,0,1 | M1A1 | M2A1},
	/* TESTL */	{WDA,LoNG,0,3,1 | M1A1 | M2A1},
	/* CBTW */	{X86,WoRD,2,0,3},
	/* CWTL */	{X86,LoNG,0,0,3},
	/* CWTD */	{X86,LoNG,0,0,3},
	/* CLTD */	{X86,LoNG,0,0,3},
	/* LDS */	{X86,LoNG,0,0,6},
	/* LEAW */	{WD1,WoRD,2,0,1},
	/* LEAL */	{WDA,LoNG,0,0,1},
	/* LES */	{X86,LoNG,0,0,6},
	/* MOVB */	{WDA,ByTE,0,0,1},
	/* MOVW */	{WD1,WoRD,2,0,1},
	/* MOVL */	{WDA,LoNG,0,0,1},
	/* MOVSBW */	{WD1,ByTE,2,0,3},
	/* MOVSBL */	{WD1,ByTE,2,0,3},
	/* MOVSWL */	{WD1,WoRD,2,0,3},
	/* MOVZBW */	{WD1,ByTE,2,0,3},
	/* MOVZBL */	{WD1,ByTE,2,0,3},
	/* MOVZWL */	{WD1,WoRD,2,0,3},
	/* NOTB */	{X86,ByTE,0,2,1 | M1A2},
	/* NOTW */	{X86,WoRD,2,0,1 | M1A2},
	/* NOTL */	{X86,LoNG,0,2,1 | M1A2},
	/* POPW */	{WD1,WoRD,2,0,1 | M1A4},
	/* POPL */	{WDA,LoNG,0,0,1 | M1A4},
	/* PUSHW */	{WD1,WoRD,2,0,1 | M1A3},
	/* PUSHL */	{WDA,LoNG,0,0,1 | M1A3},
	/* XCHGB */	{X86,ByTE,0,0,3 | M1A2 | M2A2},
	/* XCHGW */	{X86,WoRD,2,0,3 | M1A2 | M2A2},
	/* XCHGL */	{X86,LoNG,0,0,3 | M1A2 | M2A2},
	/* XLAT */	{X86,LoNG,0,0,4},
	/* CLC */	{X86,LoNG,0,0,2},
	/* CLD */	{X86,LoNG,0,0,2},
	/* CLI */	{X86,LoNG,0,0,2},
	/* CMC */	{X86,LoNG,0,0,2},
	/* LAHF */	{X86,LoNG,0,0,3},
	/* POPF */	{X86,LoNG,0,0,9},
	/* PUSHF */	{X86,LoNG,0,0,4},
	/* SAHF */	{X86,LoNG,0,0,2},
	/* STC */	{X86,LoNG,0,0,2},
	/* STD */	{X86,LoNG,0,0,2},
	/* STI */	{X86,LoNG,0,0,5},
	/* SCAB */	{X86,ByTE,0,0,6},
	/* SCAW */	{X86,WoRD,2,0,6},
	/* SCAL */	{X86,LoNG,0,0,6},
	/* SCMPB */	{X86,ByTE,0,0,6},
	/* SCMPW */	{X86,WoRD,2,0,6},
	/* SCMPL */	{X86,LoNG,0,0,6},
	/* SLODB */	{X86,ByTE,0,0,12},
	/* SLODW */	{X86,WoRD,2,0,12},
	/* SLODL */	{X86,LoNG,0,0,12},
	/* SMOVB */	{X86,ByTE,0,0,12},
	/* SMOVW */	{X86,WoRD,2,0,12},
	/* SMOVL */	{X86,LoNG,0,0,12},
	/* SSTOB */	{X86,ByTE,0,0,12},
	/* SSTOW */	{X86,WoRD,2,0,12},
	/* SSTOL */	{X86,LoNG,0,0,12},
	/* INB */	{X86,ByTE,0,0,14},
	/* INW */	{X86,WoRD,2,0,14},
	/* INL */	{X86,LoNG,0,0,14},
	/* OUTB */	{X86,ByTE,0,0,16},
	/* OUTW */	{X86,WoRD,2,0,16},
	/* OUTL */	{X86,LoNG,0,0,16},
	/* ESC */	{X86,LoNG,0,0,1},
	/* HLT */	{X86,LoNG,0,0,99},
	/* INT */	{X86,LoNG,0,0,99},
	/* INTO */	{X86,LoNG,0,0,3},
	/* IRET */	{X86,LoNG,0,0,1},
	/* LOCK */	{X86,LoNG,0,0,1},
	/* WAIT */	{X86,LoNG,0,0,1},
	/* ENTER */	{X86,LoNG,0,0,14},
	/* LEAVE */	{X86,LoNG,0,0,5},
	/* PUSHA */	{X86,LoNG,0,0,11},
	/* POPA */	{X86,LoNG,0,0,9},
	/* INS */	{X86,LoNG,0,0,17},
	/* OUTS */	{X86,LoNG,0,0,17},
	/* BOUND */	{X86,LoNG,0,0,7},
	/* CTS */	{X86,LoNG,0,0,7},
	/* LGDT */	{X86,LoNG,0,0,11},
	/* SGDT */	{X86,LoNG,0,0,10},
	/* LIDT */	{X86,LoNG,0,0,11},
	/* SIDT */	{X86,LoNG,0,0,10},
	/* LLDT */	{X86,LoNG,0,0,11},
	/* SLDT */	{X86,LoNG,0,0,2},
	/* LTR */	{X86,LoNG,0,0,2},
	/* STR */	{X86,LoNG,0,0,2},
	/* LMSW */	{X86,WoRD,2,0,13},
	/* SMSW */	{X86,WoRD,2,0,2},
	/* LAR */	{X86,LoNG,0,0,11},
	/* LSL */	{X86,LoNG,0,0,10},
	/* ARPL */	{X86,LoNG,0,0,9},
	/* VERR */	{X86,LoNG,0,0,1},
	/* BOUNDL */	{X86,LoNG,0,0,1},
	/* BOUNDW */	{X86,LoNG,0,0,1},
	/* BSFL */	{X86,LoNG,0,0,1},
	/* BSFW */	{X86,LoNG,0,0,1},
	/* BSRL */	{X86,LoNG,0,0,1},
	/* BSRW */	{X86,LoNG,0,0,1},
	/* BSWAP */	{X86,LoNG,0,0,1},
	/* BTCL */	{X86,LoNG,0,0,1},
	/* BTCW */	{X86,LoNG,0,0,1},
	/* BTL */	{X86,LoNG,0,0,1},
	/* BTRL */	{X86,LoNG,0,0,1},
	/* BTRW */	{X86,LoNG,0,0,1},
	/* BTSL */	{X86,LoNG,0,0,1},
	/* BTSW */	{X86,LoNG,0,0,1},
	/* BTW */	{X86,LoNG,0,0,1},
	/* CLTS */	{X86,LoNG,0,0,1},
	/* CMPSB */	{X86,LoNG,0,0,1},
	/* CMPSL */	{X86,LoNG,0,0,1},
	/* CMPSW */	{X86,LoNG,0,0,1},
	/* CMPXCHGB */	{X86,LoNG,0,0,1},
	/* CMPXCHGL */	{X86,LoNG,0,0,1},
	/* CMPXCHGW */	{X86,LoNG,0,0,1},
	/* INSB */	{X86,LoNG,0,0,1},
	/* INSL */	{X86,LoNG,0,0,1},
	/* INSW */	{X86,LoNG,0,0,1},
	/* INVD */	{X86,LoNG,0,0,1},
	/* INVLPG */	{X86,LoNG,0,0,1},
	/* LARL */	{X86,LoNG,0,0,1},
	/* LARW */	{X86,LoNG,0,0,1},
	/* LDSL */	{X86,LoNG,0,0,1},
	/* LDSW */	{X86,LoNG,0,0,1},
	/* LESL */	{X86,LoNG,0,0,1},
	/* LESW */	{X86,LoNG,0,0,1},
	/* LFSL */	{X86,LoNG,0,0,1},
	/* LFSW */	{X86,LoNG,0,0,1},
	/* LGSL */	{X86,LoNG,0,0,1},
	/* LGSW */	{X86,LoNG,0,0,1},
	/* LODSB */	{X86,LoNG,0,0,1},
	/* LODSL */	{X86,LoNG,0,0,1},
	/* LODSW */	{X86,LoNG,0,0,1},
	/* LSLL */	{X86,LoNG,0,0,1},
	/* LSLW */	{X86,LoNG,0,0,1},
	/* LSSL */	{X86,LoNG,0,0,1},
	/* LSSW */	{X86,LoNG,0,0,1},
	/* MOVSL */	{X86,LoNG,0,0,1},
	/* NOP */	{X86,LoNG,0,0,1},
	/* OUTSB */	{X86,LoNG,0,0,1},
	/* OUTSL */	{X86,LoNG,0,0,1},
	/* OUTSW */	{X86,LoNG,0,0,1},
	/* POPAL */	{X86,LoNG,0,0,1},
	/* POPAW */	{X86,LoNG,0,0,1},
	/* POPFL */	{X86,LoNG,0,0,1},
	/* POPFW */	{X86,LoNG,0,0,1},
	/* PUSHAL */	{X86,LoNG,0,0,1},
	/* PUSHAW */	{X86,LoNG,0,0,1},
	/* PUSHFL */	{X86,LoNG,0,0,1},
	/* PUSHFW */	{X86,LoNG,0,0,1},
	/* REPE */	{X86,LoNG,0,0,1},
	/* REPNE */	{X86,LoNG,0,0,1},
	/* SCASB */	{X86,LoNG,0,0,1},
	/* SCASL */	{X86,LoNG,0,0,1},
	/* SCASW */	{X86,LoNG,0,0,1},
	/* SETA */	{X86,ByTE,0,0,1},
	/* SETAE */	{X86,ByTE,0,0,1},
	/* SETB */	{X86,ByTE,0,0,1},
	/* SETBE */	{X86,ByTE,0,0,1},
	/* SETC */	{X86,ByTE,0,0,1},
	/* SETE */	{X86,ByTE,0,0,1},
	/* SETG */	{X86,ByTE,0,0,1},
	/* SETGE */	{X86,ByTE,0,0,1},
	/* SETL */	{X86,ByTE,0,0,1},
	/* SETLE */	{X86,ByTE,0,0,1},
	/* SETNA */	{X86,ByTE,0,0,1},
	/* SETNAE */	{X86,ByTE,0,0,1},
	/* SETNB */	{X86,ByTE,0,0,1},
	/* SETNBE */	{X86,ByTE,0,0,1},
	/* SETNC */	{X86,ByTE,0,0,1},
	/* SETNE */	{X86,ByTE,0,0,1},
	/* SETNG */	{X86,ByTE,0,0,1},
	/* SETNL */	{X86,ByTE,0,0,1},
	/* SETNLE */	{X86,ByTE,0,0,1},
	/* SETNO */	{X86,ByTE,0,0,1},
	/* SETNP */	{X86,ByTE,0,0,1},
	/* SETNS */	{X86,ByTE,0,0,1},
	/* SETNZ */	{X86,ByTE,0,0,1},
	/* SETO */	{X86,ByTE,0,0,1},
	/* SETP */	{X86,ByTE,0,0,1},
	/* SETPE */	{X86,ByTE,0,0,1},
	/* SETPO */	{X86,ByTE,0,0,1},
	/* SETS */	{X86,ByTE,0,0,1},
	/* SETZ */	{X86,ByTE,0,0,1},
	/* SSCAB */	{X86,LoNG,0,0,1},
	/* SSCAL */	{X86,LoNG,0,0,1},
	/* SSCAW */	{X86,LoNG,0,0,1},
	/* STOSB */	{X86,LoNG,0,0,1},
	/* STOSL */	{X86,LoNG,0,0,1},
	/* STOSW */	{X86,LoNG,0,0,1},
	/* VERW */	{X86,LoNG,0,0,1},
	/* WBINVD */	{X86,LoNG,0,0,1},
	/* XADDB */	{X86,LoNG,0,0,1},
	/* XADDL */	{X86,LoNG,0,0,1},
	/* XADDW */	{X86,LoNG,0,0,1},
	/* F2XM1 */	{X86,LoNG,0, 2, ST_SRC | UNEXPECTED},
	/* FABS */	{X86,LoNG,0, 0, ST_SRC | SRC1 | DEST1},
	/* FCHS */	{X86,LoNG,0, 0, ST_SRC | SRC1 | DEST1},
	/* FCLEX */	{X86,LoNG,0, 0, UNEXPECTED},
	/* FCOMPP */	{X86,LoNG,3, 1, ST_SRC | ST1_SRC | SRC12 | SETCC},
	/* FDECSTP */	{X86,LoNG,0, 0, POP | UNEXPECTED},
	/* FINCSTP */	{X86,LoNG,0, 0, PUSH | UNEXPECTED},
	/* FINIT */	{X86,LoNG,0, 0, UNEXPECTED | NOST},
	/* FLD1 */	{X86,LoNG,2, 0, PUSH | DEST1 | _FLD},
	/* FLDL2E */	{X86,LoNG,2, 2, PUSH | UNEXPECTED | _FLD},
	/* FLDL2T */	{X86,LoNG,2, 2, PUSH | UNEXPECTED | _FLD},
	/* FLDLG2 */	{X86,LoNG,2, 2, PUSH | UNEXPECTED | _FLD},
	/* FLDLN2 */	{X86,LoNG,2, 2, PUSH | UNEXPECTED | _FLD},
	/* FLDPI */	{X86,LoNG,2, 2, PUSH | UNEXPECTED | _FLD},
	/* FLDZ */	{X86,LoNG,2, 0, PUSH | DEST1 | _FLD},
	/* FNCLEX */	{X86,LoNG,0, 0, UNEXPECTED | NOST},
	/* FNINIT */	{X86,LoNG,0, 0, UNEXPECTED | NOST},
	/* FNOP */	{X86,LoNG,0, 0, UNEXPECTED | NOST},
	/* FPATAN */	{X86,LoNG,0, 5, POP | ST_SRC  | ST1_SRC | UNEXPECTED},
	/* FPREM */	{X86,LoNG,0, 2, ST_DEST | ST1_SRC | UNEXPECTED},
	/* FPTAN */	{X86,LoNG,0, 70, ST1_SRC | UNEXPECTED},
	/* FRNDINT */	{X86,LoNG,0, 7, ST_SRC | UNEXPECTED},
	/* FSCALE */	{X86,LoNG,0, 2, ST_DEST | ST1_SRC | UNEXPECTED},
	/* FSETPM */	{X86,LoNG,0, 0, UNEXPECTED | NOST},
	/* FSQRT */	{X86,LoNG,0, 70, ST_SRC | UNEXPECTED},
	/* FTST */	{X86,LoNG,3, 1, ST_SRC | SRC1 | DEST1  | SETCC},
	/* FWAIT */	{X86,LoNG,0, 0, UNEXPECTED | NOST},
	/* FXAM */	{X86,LoNG,0, 0, ST_SRC | UNEXPECTED},
	/* FXTRACT */	{X86,LoNG,0, 4, ST_SRC | UNEXPECTED},
	/* FYL2X */	{X86,LoNG,0, 13, POP | ST_SRC | ST1_SRC | UNEXPECTED},
	/* FYL2XP1 */	{X86,LoNG,0, 13, POP | ST_SRC | ST1_SRC | UNEXPECTED},
	/* FLDCW */	{X86,WoRD,2, 0, SRC1 | _FLD | NOST},
	/* FSTCW */	{X86,WoRD,1, 0,DEST1 | NOST},
	/* FNSTCW */	{X86,WoRD,1, 0, UNEXPECTED | NOST},
	/* FSTSW */	{X86,WoRD,1, 0, DEST1 | NOST},
	/* FNSTSW */	{X86,WoRD,1, 0, DEST1 | NOST},
	/* FSTENV */	{X86,GREAT,1, 0, UNEXPECTED | NOST},
	/* FNSTENV */	{X86,GREAT,1, 0, UNEXPECTED | NOST},
	/* FLDENV */	{X86,GREAT,2, 0, UNEXPECTED | NOST},
	/* FSAVE */	{X86,GREAT,1, 0, UNEXPECTED | NOST},
	/* FNSAVE */	{X86,GREAT,1, 0, UNEXPECTED | NOST},
	/* FRSTOR */	{X86,GREAT,2, 0, UNEXPECTED | NOST},
	/* FBLD */	{X86,TEN,2, 0, PUSH | UNEXPECTED},
	/* FBSTP */	{X86,TEN,1, 0, POP | ST_SRC | UNEXPECTED},
	/* FIADD */	{X86,WoRD,3, 7, ST_DEST | SRC12 | DEST2},
	/* FIADDL */	{X86,LoNG,3, 7, ST_DEST | SRC12 | DEST2},
	/* FICOM */	{X86,WoRD,3, 1, ST_SRC | SRC12 | SETCC},
	/* FICOML */	{X86,LoNG,3, 1, ST_SRC |SRC12 | SETCC},
	/* FICOMP */	{X86,WoRD,3, 1, POP | ST_SRC | SRC12 | SETCC},
	/* FICOMPL */	{X86,LoNG,3, 1, POP | ST_SRC | SRC12 | SETCC},
	/* FIDIV */	{X86,WoRD,5, 70, ST_DEST | SRC12 | DEST2 | _DIV},
	/* FIDIVL */	{X86,LoNG,5, 70, ST_DEST | SRC12 | DEST2 | _DIV},
	/* FIDIVR */	{X86,WoRD,5, 70, ST_SRC | SRC12 | DEST2 | _DIV},
	/* FIDIVRL */	{X86,LoNG,5, 70, ST_DEST | SRC12 | DEST2 | _DIV},
	/* FILD */	{X86,WoRD,2, 4, PUSH | SRC1 | DEST2},
	/* FILDL */	{X86,LoNG,2, 8, PUSH | SRC1 | DEST2},
	/* FILDLL */	{X86,DoBL,2, 8, PUSH | SRC1 | DEST2},
	/* FIMUL */	{X86,WoRD,4, 8, ST_DEST | SRC12 | DEST2 | _MUL},
	/* FIMULL */	{X86,LoNG,4, 8, ST_DEST | SRC12 | DEST2 | _MUL},
	/* FIST */	{X86,WoRD,1, 0, ST_SRC | SRC1 | DEST2},
	/* FISTL */	{X86,LoNG,1, 0, ST_SRC | SRC1 | DEST2},
	/* FISTP */	{X86,WoRD,1, 0, POP | ST_SRC | SRC1 | DEST2},
	/* FISTPL */	{X86,LoNG,1, 0, POP | ST_SRC | SRC1 | DEST2},
	/* FISTPLL */	{X86,DoBL,1, 0, POP | ST_SRC | SRC1 | DEST2},
	/* FISUB */	{X86,WoRD,3, 7, ST_DEST | SRC12 | DEST2},
	/* FISUBL */	{X86,LoNG,3, 7, ST_DEST | SRC12 | DEST2},
	/* FISUBR */	{X86,WoRD,3, 7, ST_SRC | SRC12 | DEST2},
	/* FISUBRL */	{X86,LoNG,3, 7, ST_DEST | SRC12 | DEST2},
	/* FADD */	{X86,LoNG,3, 7, NOARG | SRC12 | DEST2},
	/* FADDS */	{X86,LoNG,3, 7, ST_DEST | SRC12 | DEST2},
	/* FADDL */	{X86,DoBL,3, 7, ST_DEST | SRC12 | DEST2},
	/* FADDP */	{X86,LoNG,3, 7, POP | SRC12 | DEST2},
	/* FCOM */	{X86,LoNG,3, 1, NOARG | SRC12 | SETCC},
	/* FCOMS */	{X86,LoNG,3, 1, ST_SRC | SRC12 | SETCC},
	/* FCOML */	{X86,DoBL,3, 1, ST_SRC | SRC12 | SETCC},
	/* FCOMP */	{X86,LoNG,3, 1, NOARG | POP | ST_DEST | SRC12 | SETCC},
	/* FCOMPS */	{X86,LoNG,3, 1, POP | ST_DEST | SRC12 | SETCC},
	/* FCOMPL */	{X86,DoBL,3, 1, POP | ST_DEST | SRC12 | SETCC},
	/* FDIV */	{X86,LoNG,5, 70, NOARG | SRC12 | DEST2 | _DIV},
	/* FDIVS */	{X86,LoNG,5, 70, ST_DEST | SRC12 | DEST2 | _DIV},
	/* FDIVL */	{X86,DoBL,5, 70, ST_DEST | SRC12 | DEST2 | _DIV},
	/* FDIVP */	{X86,LoNG,5, 70, POP | SRC12 | DEST2 | _DIV},
	/* FDIVR */	{X86,LoNG,5, 70, NOARG | SRC12 | DEST2 | _DIV},
	/* FDIVRS */	{X86,LoNG,5, 70, ST_DEST | SRC12 | DEST2 | _DIV},
	/* FDIVRL */	{X86,DoBL,5, 70, ST_DEST | SRC12 | DEST2 | _DIV},
	/* FDIVRP */	{X86,LoNG,5, 70, POP | SRC12 | DEST2 | _DIV},
	/* FFREE */	{X86,LoNG,0, 0, UNEXPECTED | NOST},
	/* FLD */	{X86,LoNG,2, 0, PUSH | SRC1 | DEST2 | _FLD},
	/* FLDS */	{X86,LoNG,2, 0, PUSH | SRC1 | DEST2 | _FLD},
	/* FLDL */	{X86,DoBL,2, 0, PUSH | SRC1 | DEST2 | _FLD},
	/* FLDT */	{X86,TEN,2, 0, PUSH | SRC1 | DEST2 | _FLD},
	/* FMUL */	{X86,LoNG,4, 13, NOARG | SRC12 | DEST2 | _MUL},
	/* FMULS */	{X86,LoNG,4, 13, ST_DEST | SRC12 | DEST2 | _MUL},
	/* FMULL */	{X86,DoBL,4, 13, ST_DEST | SRC12 | DEST2 | _MUL},
	/* FMULP */	{X86,LoNG,4, 13, POP | SRC12 | DEST2 | _MUL},
	/* FST */	{X86,LoNG,1, 0, ST_SRC | SRC1 | DEST2},
	/* FSTS */	{X86,LoNG,1, 0, ST_SRC | SRC1 | DEST2},
	/* FSTL */	{X86,DoBL,1, 0, ST_SRC | SRC1 | DEST2},
	/* FSTP */	{X86,LoNG,1, 0, POP | ST_SRC | SRC1 | DEST2},
	/* FSTPS */	{X86,LoNG,1, 0, POP | ST_SRC | SRC1 | DEST2},
	/* FSTPL */	{X86,DoBL,1, 0, POP | ST_SRC | SRC1 | DEST2},
	/* FSTPT */	{X86,TEN,1, 0, POP | ST_SRC | SRC1 | DEST2},
	/* FSUB */	{X86,LoNG,3, 7, NOARG | SRC12 | DEST2},
	/* FSUBS */	{X86,LoNG,3, 7, ST_DEST | SRC12 | DEST2},
	/* FSUBL */	{X86,DoBL,3, 7, ST_DEST | SRC12 | DEST2},
	/* FSUBP */	{X86,LoNG,3, 7, POP | SRC12 | DEST2},
	/* FSUBR */	{X86,LoNG,3, 7, NOARG | SRC12 | DEST2},
	/* FSUBRS */	{X86,LoNG,3, 7, ST_DEST | SRC12 | DEST2},
	/* FSUBRL */	{X86,DoBL,3, 7, ST_DEST | SRC12 | DEST2},
	/* FSUBRP */	{X86,LoNG,3, 7, POP | SRC12 | DEST2},
	/* FXCH */	{X86,LoNG,0,0,0},
	/* FCOS */	{X86,LoNG,0, 0, ST_SRC | SRC1 | DEST1},
	/* FPREM1*/	{X86,LoNG,0, 2, ST_SRC | UNEXPECTED},
	/* FSIN */	{X86,LoNG,0, 0, ST_SRC | SRC1 | DEST1},
	/* FSINCOS*/{X86,LoNG,0, 2, ST_SRC | UNEXPECTED},
	/* FUCOM*/	{X86,LoNG,0, 2, ST_SRC | UNEXPECTED},
	/* FUCOMP*/	{X86,LoNG,0, 2, ST_SRC | UNEXPECTED},
	/* FUCOMPP*/{X86,LoNG,0, 2, ST_SRC | UNEXPECTED},
	/* NOTHING*/	{X86,LoNG,1,0,0},
	/* OTHER */	{X86,LoNG,1,0}
	};


char test_flags[] = {
/*	JA	*/		CF | ZF	,
/*	JAE	*/		CF		,
/*	JB	*/		CF		,
/*	JBE	*/		CF | ZF	,
/*	JC	*/		CF		,
/*	JCXZ	*/	0		,
/*	JE	*/		ZF		,
/*	JG	*/		SF | OF | ZF	,
/*	JGE	*/		SF | OF	,
/*	JL	*/		SF | OF	,
/*	JLE	*/		SF | OF | ZF	,
/*	JNA	*/		CF | ZF	,
/*	JNAE	*/	CF		,
/*	JNB	*/		CF		,
/*	JNBE	*/	CF | ZF	,
/*	JNC	*/		CF		,
/*	JNE	*/		ZF		,
/*	JNG	*/		SF | OF | ZF	,
/*	JNGE	*/	SF | OF	,
/*	JNL	*/		SF | OF	,
/*	JNLE	*/	SF | OF | ZF	,
/*	JNO	*/		OF		,
/*	JNP	*/		PF		,
/*	JNS	*/		SF		,
/*	JNZ	*/		ZF		,
/*	JO	*/		OF		,
/*	JP	*/		PF		,
/*	JPE	*/		PF		,
/*	JPO	*/		PF		,
/*	JS	*/		SF		,
/*	JZ	*/		ZF
};
