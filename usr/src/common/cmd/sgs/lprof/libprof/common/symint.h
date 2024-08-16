/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)lprof:libprof/common/symint.h	1.3"

#include "symintHdr.h"

	/* protect against multiple inclusion */
#ifndef SYMINT_FCNS
#define SYMINT_FCNS

/* * * * * *
 * symint.c -- symbol information interface routines,
 * 	       interface definition.
 * 
 * these routines form a symbol information access
 * interface, for the profilers to get at object file
 * information.  this interface was designed to aid
 * in the COFF to ELF conversion of prof, lprof and friends.
 * 
 * this file includes all declarative information required
 * by a user of this interface.
 * 
 * ASSUMPTIONS
 * ===========
 * 
 * 1.	that there exists a routine _lprof_Malloc, with the following
 * 	(effective) prototype:
 * 		char * _lprof_Malloc (int item_count, int item_size);
 * 	which does NOT (necessarily) initialize the allocated storage,
 * 	and which issues an error message and calls exit() if
 * 	the storage could not be allocated.
 * 
 */


/* * * * * *
 * the interface routines:
 * 
 * 	1. open an object file, set up PROF_FILE et al. (_symintOpen).
 * 	1. close an object file, clean up PROF_FILE et al. (_symintClose).
 * 
 * the data:
 * 
 * 	(none yet.)
 * 
 */

					/* Returns!! */
					/* FAILURE or SUCCESS */

extern	  PROF_FILE *	_symintOpen();	/* NULL or ptr */

extern           void	_symintClose();	/* nuttin */



/* * * * * *
 * required to be provided by the user of the interface...
 */

extern         void *	_lprof_Malloc();	/* Safe alloc given ct & itemsize */

#endif
