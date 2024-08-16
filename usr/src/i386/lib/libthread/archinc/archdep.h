/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libthread:i386/lib/libthread/archinc/archdep.h	1.2.1.2"
/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

#if !defined(_ARCHDEP_H)
#define _ARCHDEP_H

/*
 * A stack frame looks like:
 *
 *	|--------------------------------|
 *	|  arguments                 	 |
 *	|--------------------------------|-\
 *	|  return address            	 | |
 *	|--------------------------------|  >-----\
 *	|  caller's %ebp (frame pointer) | |      |
 *%ebp->|--------------------------------|-/      |
 *	|  Locals, temps, saved floats	 |         > minimum stack frame
 *	|--------------------------------|-\      |
 *	|  caller's %edi (local register)| |      |
 *	|--------------------------------| |      |
 *	|  caller's %esi (local register)|  >-----/
 *	|--------------------------------| |
 *	|  caller's %ebx (local register)| |
 *%esp->|--------------------------------|-/
 */

/*
 * Constants defining a stack frame.
 */
#define MINFRAME	20 /* minimum stack frame */

/*
 * Stack alignment macros.
 */
#define STACK_ALIGN	4
#define SA(X)	(((X)+(STACK_ALIGN-1)) & ~(STACK_ALIGN-1))

/*
 * structure of lwp private data area from the perspective of the
 * thread library. It currently includes a pointer to the lwp private 
 * data area for the thread currently running on the lwp followed
 * by a boolean indicating whether or not floating point has been done
 * during the current thread time slice.
 * The structure is defined here so the offset of the fpu used flag
 * can be properly defined in symbols.c and used in cswtch.s
 */

typedef struct __lwp_private_page __lwp_private_page_t;

struct __lwp_private_page {
	void		**_lwp_private_data_pointer;
	boolean_t	_lwp_private_fpu_used;
};
 
#endif /* _ARCHDEP_H */
