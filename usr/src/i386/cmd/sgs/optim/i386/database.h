/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)optim:i386/database.h	1.9"
#ifndef DATABASE_H
#define DATABASE_H
struct data_base 
{	
	unsigned char pairability;
	unsigned char size;
	unsigned char unexpected;/*Preference value for P5 fp candidate list order*/
	unsigned char peep_type; /* And concurency data for i486 fp */
	unsigned int  float_data;
};
extern struct data_base opcodata[];
#define X86     0  /* four types of pairability */
#define WD2     1
#define WD1     2
#define WDA     3
#define ByTE	1  /* lengths of instructions    */
#define WoRD	2
#define LoNG	4
#define DoBL	8
#define TEN		10
#define GREAT	28
#define OpLength(p)		opcodata[p->op - (p->op > SAFE_ASM ? \
						(FIRST_opc + SAFE_ASM) : FIRST_opc)].size
#define Unexpected(p)	(p->op < FIRST_opc || (int)(p->op) > LAST_opc || \
						(p->op < FIRST387 && \
						 opcodata[p->op - FIRST_opc].unexpected == 1))
#define Hasprefix(p)	(p->op < FIRST387 && \
						 opcodata[p->op - FIRST_opc].unexpected == 2)
#define ISRISCY(p)		(p->op < FIRST387 && \
                         opcodata[p->op - FIRST_opc].peep_type == 1)
#define Isreflexive(p)	(p->op < FIRST387 && \
                         opcodata[p->op - FIRST_opc].peep_type == 2)
#define Istest(p)		(p->op < FIRST387 && \
                         opcodata[p->op - FIRST_opc].peep_type == 3)

#define M1 0xf00
#define M2 0xf000 
#define M1A1 0x100
#define M1A2 0x200
#define M1A3 0x300
#define M1A4 0x400
#define M2A1 0x1000
#define M2A2 0x2000
#define M2A3 0x3000
#define M2A4 0x4000
/* FP convert from stack to regs and back registers */
#define PUSH 0x1 /* Push to FP stack and make dest %st(0) reg */
#define POP 0x2  /* kill top of stack */
#define ST_SRC 0x4 /* %st(0) is the source */
#define ST_DEST 0x8 /* %st(0) is the destination */
#define ST1_SRC 0x10 /* %st(1) is source */
#define NOARG 0x20   /* this opcode if used without args is: OP %st,%st(1) */
#define UNEXPECTED 0x8000 
/* Parameter info on register (FF#?)  format  */
#define SRC1 0x40  /* op1 is source */ 
#define SRC2 0x80  /* op2 is source */ 
#define SRC12 0xc0 /* op1 and 2 are sources */
#define DEST1 0x100 /* op1 is destination */
#define DEST2 0x200 /* op2 is destination */
#define DESTCC  0x400 /* Sets CC */
#define _MUL 0 /* Not used, FMUL{sl} */
#define NOST 0x800 /* No use of fp stack FSTSW, FLDCW, FSTCW ect.*/
#define _DIV 0x1000 /* FDIV{sl} */
#define _FLD 0x2000 /* FLD{sl} */
#define SETCC 0x4000 /* FCOM{sl}{p|pp} */
#define FIRST387 F2XM1
#define LAST387 (OTHER - 1)
#define FIRST_opc CALL
#define LAST_opc  LAST387

#define FP(op) ( (op >= FIRST387) && ((int)op < LAST387)) 
#define FNOARG(op) (opcodata[op - FIRST_opc].float_data & NOARG)
#define FPUSH(op) (opcodata[op - FIRST_opc].float_data & PUSH)
#define FPOP(p) ((opcodata[p->op - FIRST_opc].float_data & POP) ||  \
				(FNOARG(p->op) && p->op1 == NULL))
#define FST_SRC(op) (opcodata[op - FIRST_opc].float_data & ST_SRC)
#define FST_DEST(op) (opcodata[op - FIRST_opc].float_data & ST_DEST)
#define FST1_SRC(op) (opcodata[op - FIRST_opc].float_data & ST1_SRC)
#define FSRC1(op) (opcodata[op - FIRST_opc].float_data & SRC1)
#define FSRC2(op) (opcodata[op - FIRST_opc].float_data & SRC2)
#define FSRC12(op) (opcodata[op - FIRST_opc].float_data & SRC12)
#define FDEST1(op) (opcodata[op - FIRST_opc].float_data & DEST1)
#define FDEST2(op) (opcodata[op - FIRST_opc].float_data & DEST2)
/*
#define F_MUL(op) (opcodata[op - FIRST_opc].float_data & _MUL)
_MUL is currently #defined 0, we dont have free bits to #define it.
If a bit will sometime become free, or if we shall decide to add length
to the structure, we will be able to use the above form, which is faster.
*/
#define F_MUL(op) (op == FIMUL || op == FIMULL || op == FMUL || \
                   op == FMULS || op == FMULL  || op == FMULP) 
#define F_DIV(op) (opcodata[op - FIRST_opc].float_data & _DIV)
#define F_FLD(op) (opcodata[op - FIRST_opc].float_data & _FLD)
#define FNOST(op) (opcodata[op - FIRST_opc].float_data & NOST)
#define FSETCC(op) (opcodata[op - FIRST_opc].float_data & SETCC)
#define FUNEXPECTED(op) (opcodata[op - FIRST_opc].float_data & UNEXPECTED)
#define FRBITS(str) (FP0 << (str[3] - '0'))
#define ISFREG(cp) ((cp != NULL) && (cp[2] == '#'))
#define CYCLES(op) (opcodata[op - FIRST_opc].float_data)
#define CONCURRENT(op) opcodata[op - FIRST_opc].peep_type
#define FPPREFERENCE(op) opcodata[op - FIRST_opc].unexpected
#endif

/* macros for eflags data */

extern char test_flags[];

#define OF	1
#define ZF	2
#define SF	3
#define CF	4
#define PF	5

#define FIRST_JCC	JA
#define LAST_JCC	JZ
#define TEST_OF(op)	((op >= FIRST_JCC && op <= LAST_JCC) && \
						(test_flags[op - FIRST_JCC] & OF))
#define TEST_ZF(op)	((op >= FIRST_JCC && op <= LAST_JCC) && \
						(test_flags[op - FIRST_JCC] & ZF))
#define TEST_SF(op)	((op >= FIRST_JCC && op <= LAST_JCC) && \
						(test_flags[op - FIRST_JCC] & SF))
#define TEST_CF(op)	((op >= FIRST_JCC && op <= LAST_JCC) && \
						(test_flags[op - FIRST_JCC] & CF))
#define TEST_PF(op)	((op >= FIRST_JCC && op <= LAST_JCC) && \
						(test_flags[op - FIRST_JCC] & PF))
