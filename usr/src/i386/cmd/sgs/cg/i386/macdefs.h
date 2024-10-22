/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cg:i386/macdefs.h	1.22"
/*	macdefs - machine dependent macros and parameters
 *	i386 CG
 *		Intel iAPX386
 *
 */

/* initial values for stack offsets in bits */

#define ARGINIT 	(8*SZCHAR)
#define AUTOINIT	0
#define INTARGS

/* sizes of types in bits */

#define SZCHAR		8
#define SZSHORT		16
#define SZINT		32
#define SZPOINT		32

#define SZFLOAT		32
#define SZDOUBLE	64
#define SZLDOUBLE	96		/* only 80 bits are significant */
#define SZFPTR		64

/* alignments of types in bits */

#define ALCHAR		8
#define ALSHORT		16
#define ALINT		32
#define ALPOINT		32
#define ALSTACK		32
#define ALSTRUCT	8
#define ALINIT		32

#define ALFLOAT		32
#define ALDOUBLE	32
#define ALLDOUBLE	32
#define ALDOUBLE2	64
#define ALFPTR		32

/* Put character arrays at least 4 bytes 
** long on word boundary for performance 
*/
#define ALCARRAY
#define ALCARRAY_LIM	32

/* (SZLDOUBLE - 80)/SZCHAR	number bytes to pad long doubles */
#define LDOUBLE_PAD	2

/* Structures may be packed via #pragma pack(n) */

#define PACK

/* structure sizes must be a multiple of 32 bits */

#define STMULT 32

#define NOLONG			/* map longs to ints */

/* format for labels */

#define LABFMT ".L%d"

/* format for read-only labels:  useful for scheduler */
#define ROLABFMT ".L%d.RO"

/* feature definitions */

#ifndef STINCC
#define	STINCC			/* enable qcc version of compiler */
#endif
#ifndef IN_LINE
#define	IN_LINE			/* enable in-line asm's */
#endif
#ifndef REGSET
#define	REGSET			/* do register set version of compiler */
#endif

extern void fp_cleanup(), fp_push();
#define BEFORE_INLINE(p)	fp_cleanup(p)

#define AFTER_INLINE(p)		if(p->type == TFLOAT || \
				   p->type == TDOUBLE || \
				   p->type == TLDOUBLE) \
					 fp_push(p)

/* type size in number of regs */
/* all types take 1 regs: doubles/floats 1 287 reg, others 1 CPU reg + 5 */

typedef	unsigned long RST;	/* type to hold register bit maps */

#define szty(t) (1)

/* number of scratch registers:
** 3 are integer/pointer
** 1 are 287
*/

#define NRGS 4

/* total registers
** 8 are integer/pointer (3 scratch, 3 user register, esp/ebp)
** 1 are 287		 (1 scratch)
*/

#define TOTREGS	9
#define USRREGHI 7

/* Define symbol so we can allocate odd register pairs; in fact,
** no pairs are allocated
*/

#define ODDPAIR

/* params addressed with positive offsets from fp */

#undef BACKPARAM
#undef BACKARGS

/* temps and autos addressed with negative offsets from fp */

#define BACKTEMP
#define BACKAUTO

/* bytes run right to left */

#define RTOLBYTES

/* function calls and arguments */
/* don't nest function calls (where do the args go?) */

/* 	#define NONEST			  */

/* evaluate args right to left */

#undef LTORARGS

/* chars are signed */

/* enable register usage suggestions to optimizer */

#define IMPREGAL

/* disable alternate switch register */

#undef  IMPSWREG

/* don't pass around structures as scalars */

#define NOSIMPSTR

/* structures returned in temporary location -- define magic string */

#ifdef	OLDTMPSRET
#define TMPSRET         "\tleal\tZP,%eax\n"
#define AUXREG		REG_EAX
#else
/* TMPSRET is a dummy.  We don't want the compiler to pass the
** address of the return value automatically, because it shows
** up on the stack.  But we need a temporary intermediate register,
** and this may interfere with addressing expressions in call's
** left operand.  Leave the setup to STASG templates.
*/
#define TMPSRET         ""
#define AUXREG		REG_EAX
#endif

/* In constant folding, truncate shift count to 5 bits first. */

#define SHIFT_MASK      (0x1f)

/* enable C source in output comments */

/* comment indicator */

#define COMMENTSTR "/"
#define IDENTSTR	".ident"
#define ZEROSTR		".zero"
#define EXTSTR		".ext"
#define DBLSTR		".double"
#define FLTSTR		".float"
#define INTSTR		".long"
#define SHORTSTR	".value"
#define CHARSTR		".byte"
#define ALIGNSTR	".align"
#define LOCALSTR	".local"
#define COMMONSTR	".comm"
#define GLOBALSTR	".globl"
#define TYPESTR		".type"
#define SIZESTR		".size"

#define JMPSTR		"jmp"
#define LABSTR		".L"

/* asm markers */

#define SAFE_ASM_COMMENT "/SAFE_ASM"	/* partially optimizable asm */
#define ASM_PRAGMA SAFE_ASM_COMMENT
#define	ASM_COMMENT	"/ASM"
#define	ASM_END		"/ASMEND"

/* protection markers: no optimizations here*/

#define PROT_START	"/ASM"
#define PROT_END	"/ASMEND"

/* volatile operand end */

extern int cur_opnd;
#define vol_opnd_end()	{cur_opnd <<= 1;}
#define VOL_OPND_END	','

/* Register number for debugging */

	/* Temps int/pointer */
#define REG_EAX		0
#define REG_EDX		1
#define REG_ECX		2

	/* Temps floating point */
#define REG_FP0		3

	/* User Regs int/pointer */
#define REG_EBX		4
#define REG_ESI		5
#define REG_EDI		6

	/* Stack Pointer/Frame Pointer */
#define REG_ESP		7
#define REG_EBP		8

/* Enable assembly language comments */

#define	ASSYCOMMENT

/* user-level fix-up at symbol definition time */

#define FIXDEF(p)	fixdef(p)

/* support for structure debug info */

#define FIXSTRUCT(p,q)	strend(p)

/* Map register number to external register number for debugger */

#define OUTREGNO(p)     (outreg[(p)->offset])
extern int outreg[];

/* To turn on proper IEEE floating point standard comparison of non-trapping NaN's.
/* Two floating point comparisons:  CMPE for exception raising on all NaN's.
/* CMP for no exception raising for non-trapping NaN's, used for fp == and !=
*/
#define IEEE

/* All arithmetic done in extended precision */
#define FP_EXTENDED

/* expand string space in sty */

#define	NSTRING		60000

/* expand shape space in sty */

#define	NSTYSHPS	10000

/* expand template space in sty */

#define	NOPTB		1000
#define	NTMPLTS		1000

/* expand refine shape space in sty */

#define	NREFSH		1000

/* bypass initialization of INTs through templates */

#define	MYINIT	sincode

/* We supply locctr */

#define MYLOCCTR

extern void p2cleanup();
#define MYP2CLEANUP()	p2cleanup()

/* Our routine to move to a different register */

#define RS_MOVE         rs_move
extern void rs_move();

/* Routine to suggest subtree to spill */

#define SUGGEST_SPILL(p)	suggest_spill(p)

#define EXIT(x) myexit(x)

/* extend DIMTAB and SYMTAB size for kernel and friends */
/*
#define	DIMTABSZ	4500
#define	SYMTSZ		3000
*/

#define MYRET_TYPE(t)	myret_type(t)
/* use standard switch handling code */

#define	BCSZ		 200
#define	PARAMSZ		 300
#define TREESZ		1000
#define NTSTRBUF	 120 

extern void
	acon(),
	conput(),
	adrput(),
	upput(),
	zzzcode(),
	insput(),
	ex_before(),
	sw_direct(),
	bfcode(),
	efcode(),
	stasg(),
	starg(),
	begf();

extern int
	locctr(),
	getlab();

#define	emit_str(s)	fputs(s,outfile)
#define	MYREADER	myreader
#define	BASEREG		REG_EBX

/* gotflag is a bit vector initialized to NOGOTREF in bfcode().  GOTREF
 * or GOTSWREF may be set independently of each other.
 * NOGOTREF:	no references to GOT in function
 * GOTREF:	non-switch reference to GOT
 * GOTSWREF:	reference to GOT via switch
 */

#define NOGOTREF 0
#define GOTREF	 01
#define GOTSWREF 02
extern int gotflag;

extern int picflag;
extern void myreader();
extern chars_signed; /* "Signedness" of "plain" chars, determined by acomp */

#define	WATCHDOG	200
#define FORCE_LC(lc)	(-(lc))		/*forcing location counter*/

/* Generate compare/je sequence for switch statement */

#define sw_cmp(v, l) \
    fprintf(outfile, "\tcmpl\t$%ld,%%eax\n\tje\t.L%d\n", v, l)

#define sw_jg(l) \
    fprintf(outfile, "\tjg\t.L%d\n", l)

/* Routines not needed for this instance of the compiler */

#define setswreg(p)	(p)	/* pass node thru */
#define exname(name)	(name)	/* name is unchanged */
