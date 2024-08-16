/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nas:common/objf.h	1.1"
/*
* common/objf.h - common assembler object file header
*
* Depends on:
*	"common/as.h"
*/

#ifdef __STDC__
int	openobjf(const char *);			/* open object file */
void	closeobjf(void);			/* finish object file */
int	objfsrcstr(Expr *);			/* note src file string */
void	objfmksyms(size_t, size_t, size_t);	/* start symbol table */
void	objfmksect(size_t, size_t);		/* start section table */
void	objfsection(const Section *);		/* create section entry */
void	objfsymbol(const Symbol *);		/* create symbol entry */
#else
int	openobjf();
void	closeobjf();
int	objfsrcstr();
void	objfmksyms(), objfmksect();
void	objfsection();
void	objfsymbol();
#endif
