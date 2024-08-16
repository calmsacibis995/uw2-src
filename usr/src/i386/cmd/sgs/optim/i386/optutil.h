/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)optim:i386/optutil.h	1.1.4.13"
#ifndef OPTUTIL_H
#define OPTUTIL_H
#include "optim.h"
/* optutil.h
**
**	Declarations for optimizer utilities
**/

/* isreg expression is true if the operand is a register */
#define isreg(s) ((s) != NULL && *(s) == '%')
#define isconst(s) ((s) != NULL && *(s) == '$')
#define isimmed(operand) ((operand) && *(operand) == '$' && \
		(isdigit(operand[1]) || operand[1] == '-' && isdigit(operand[2]))) 
#define is_logic(p)	((p->op >= ANDB) && (p->op <= XORL))
#define hasdisplacement(op) ((op) && (*op) != '$' && (*op) != '%'\
								  && (*op) != '(' )

#define ismem(op)	(((op) && (*op) && (*op) != '%' && (*op) != '$' && \
					!(op[1] == 'F' && op[2] == '#')) ? MEM : 0)
#define cp_vol_info(P1,P2)	(P2)->userdata = (P1)->userdata
extern boolean is_unsable_br();
extern void sign2unsign();
extern NODE * insert();		/* insert new instruction node */
extern void chgop();		/* change op code */
extern NODE *duplicate();	/* make a copy of a NODE after another node */
extern void invbr();		/* inverse brach op */
extern void br2set();       /* inverse branch op to setcc op */
extern void br2nset();      /* inverse branch op to oposite setcc op */
extern char *sreg2b();      /* return byte reg of scratch reg        */
extern boolean isdyadic();	/* is instruction dyadic? */
extern boolean is2dyadic();	/* is instruction dyadic? */
extern boolean is_fp_arit();	/* is floating point arithmetic instruction? */
extern boolean istriadic();	/* is instruction triadic? */
extern boolean isfp();		/* is floating point instruction? */
extern boolean is_byte_instr();
extern boolean is_read_only();
extern boolean is_legal_op1();  /* check consistency of size of 1st operand */
extern void makelive();		/* make register live */
extern void makedead();		/* make register dead */
extern boolean isindex();	/* operand is an index off a register */
extern boolean ismove();	/* is instruction a move-type? */
extern boolean iszoffset();	/* is operand 0 offset from register? */
extern boolean isnumlit();	/* is operand a numeric literal: &[-]n? */
extern boolean isiros();    /* is operand immediate, register or stack*/
extern void exchange();		/* exchange node and successor */
extern boolean usesvar();	/* does first operand use second? */
extern boolean isvolatile();	/* is operand volatile? */
extern boolean has_scale(); /*does indexing contain scaling? */
extern void ldelin();	
extern void ldelin2();	/* routines to preserve */
extern void lexchin();
extern void lmrgin1();	/* line number info */
/*extern void lmrgin2();*/
extern void lmrgin3();
extern void revbr();	/* reverse sense of a jump node */
extern char * dst();		/* return pointer to destination string */
extern int stype();		/* return implied type of op code */
extern boolean samereg();	/* return true if same register */
extern boolean usesreg();	/* usesreg(a,b) true if reg. b used in a */
extern unsigned int uses();		/* return register use bits */
extern unsigned int scanreg();	/* determine registers referenced in operand */
extern unsigned int sets();	/* return set register destination bits */
extern unsigned int uses_but_not_indexing();
extern boolean isdead();	/* true iff *cp is dead after p */
extern char * getp();		/* return pointer to jump destination operand */
extern char * newlab();		/* generate a new label */
extern void prinst();		/* print instruction */
extern void dprinst();		/* print instruction */
extern void fprinst();		/* print instruction */
extern boolean ishlp();		/* return true if a fixed label present */
extern unsigned int setreg();		/* return bits corresponding to register */
extern boolean isdead();	/* return true if reg. dead after node */
extern boolean change_by_const();
extern int get_free_reg();

extern unsigned int hard_uses();
extern int is_jmp_ind();
extern int find_pointer_write();
extern char *w2l();
extern char *w2h();
extern int pairable();
#ifdef DEBUG
extern void prdag(),prnode();
#endif
struct RI 	{ unsigned int reg_number;
			      char * regname[3];
     		     };         /* Register indexes   */

/* declare functions that are always handy for optimizations */

extern int atoi();
extern long atol();
extern long strtol();


/* declare functions defined in optim.c that are handy here */
#ifdef DEBUG
extern void wchange();		/* flag a window change */
#else
#define wchange()		/* nothing */
#endif

#endif
