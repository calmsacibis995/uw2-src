/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)acomp:common/init.h	52.4"
/* init.h */

extern void in_decl();		/* declarator being initialized */
extern void in_start();		/* start of initializer */
extern void in_end();		/* end of initializer */
extern void in_lc();		/* saw { in initializer */
extern void in_rc();		/* saw } in initializer */
extern void in_init();		/* next initializer expression */
extern void in_chkval();	/* check whether initializer value fits */
