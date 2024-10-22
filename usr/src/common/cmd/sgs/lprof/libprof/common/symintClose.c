/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)lprof:libprof/common/symintClose.c	1.1"

#include "hidelibc.h"		/* uses "_" to hide libc functions */
#include "hidelibelf.h"		/* uses "_" to hide libelf functions */

#include "symint.h"
#include "debug.h"

/* * * * * *
 * symintFcns.c -- symbol information interface routines.
 * 
 * these routines form a symbol information access
 * interface, for the profilers to get at object file
 * information.  this interface was designed to aid
 * in the COFF to ELF conversion of prof, lprof and friends.
 * 
 */




/* * * * * *
 * _symintClose(profPtr)
 * profPtr	- structure allocated by _symintOpen(),
 * 		  indicating structures to free and
 * 		  object file to close.
 * 
 * specifically, elf_end() and fclose() are called for the object file,
 * and the PROF_SYMBOL and section hdr arrays are freed.
 * 
 * 
 * No Returns.
 */

void
_symintClose(profPtr)
PROF_FILE *profPtr; {

	DEBUG_LOC("_symintClose: top");
	if (profPtr) {
		(void) elf_end(profPtr->pf_elf_p);
		(void) close(profPtr->pf_fildes);

		(void) free(profPtr->pf_shdarr_p);
		(void) free(profPtr->pf_symarr_p);
	}
	DEBUG_LOC("_symintClose: bottom");
}
