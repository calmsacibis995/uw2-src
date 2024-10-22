/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:oh/fm_mn_par.h	1.11.3.3"

extern	char *fld_eval();

#define	sing_eval(a,b)		fld_eval(&(a)->single, b, (a)->seqno, &(a)->in_use_flag)   /* abs s18 */

#define	multi_eval(a,b,c)	fld_eval((a)->multi + b, c, (a)->seqno, &(a)->in_use_flag) /* abs s18 */

/* #define	sing_eval(a,b)		fld_eval(&(a)->single, b, (a)->seqno)
 * #define	multi_eval(a,b,c)	fld_eval((a)->multi + b, c, (a)->seqno)
 */
#define KEYWORDSIZE	14

#define VAL_CALC	-1
#define CMD		32

/* Possible return types for attribute. */

#define RET_INT		0x1
#define RET_STR		0x2
#define RET_BOOL	0x3
#define RET_LIST	0x4
#define RET_ARGS	0x5
#define RET_PATH	0x40
#define EVAL_ONCE	0x80
#define EVAL_ALWAYS	0x100
#define EVAL_SOMETIMES	0x200
#define FREEIT		0x400
#define MAKE_COPY	0x800
#define MENU_MARKED	0x1000
#define MENU_CHECKED	0x2000
#define ATTR_VALIDATED	0x4000   /* was  ATTR_TOUCHED  abs s13 */

#ifndef EV_SQUIG                 /* must match EV_SQUIG in inc/eval.h       */
#define EV_SQUIG	0x8000	 /* set when {} are special in a descriptor */
#endif                           /* careful.. flag is flipped in eval()     */
#define RETS		7

#define INLINE		1
#define VT_EXISTS	2	/* set flag when vt is created for forms
				 * (including text frames.)  abs s14  */

/* parse table indexes for items that must have the same index
   in more than one parse table
 */

	
#define  PAR_INTR   0
#define  PAR_ONINTR 1
#define  PAR_DONE   2
#define  PAR_ACTION 2
#define  PAR_NAME   3	


struct attribute {
	char *testring;
	int flags;
        char *i18n_msgid;
	char *def;
	char *cur;
	unsigned int seqno;
};

struct fld {
	struct attribute **attrs;
};

struct fm_mn {
	unsigned int seqno;
	struct fld single;
	struct fld *multi;
	unsigned int in_use_flag;       	/* abs s18 */
};

extern unsigned int Fm_mn_seq;			/* abs s18 */
