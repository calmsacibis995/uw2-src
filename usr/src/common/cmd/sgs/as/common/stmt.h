/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nas:common/stmt.h	1.3"
/*
* common/stmt.h - common assembler statement/instruction header
*
* Depends on:
*	"common/as.h"
*/

#ifdef __STDC__
   typedef size_t InstGen(Section *, Code *);
#else
   typedef size_t InstGen();
#endif

struct t_inst_	/* common information about an instruction */
{
	const Uchar	*inst_name;	/* instruction mnemonic */
	InstGen		*inst_gen;	/* pointer to generation fcn */
	Ushort		inst_impdep;	/* for implementation use */
	Ushort		inst_minsz;	/* minimum size; used first */
};

	/* implementation provides */
#ifdef __STDC__
void	operinst(const Inst *, Operand *);		/* check inst operand */
void	gennops(Section *, const Code *, const Code *);	/* fill with "nop"s */
#else
void	operinst(), gennops();
#endif
