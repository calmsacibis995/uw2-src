/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libelf:common/mmap.c	1.3"
#include "libelf.h"
#include "decl.h"
#include <sys/mman.h>

/*	This "program" is built (but not run) to see if the following
 *	services are available.  If so, libelf uses them.  Otherwise,
 *	it uses traditional read/write/....
 */

main()
{
	ftruncate(0, 0);
	mmap(0,0,0,0,0,0);
	msync(0,0,0);
	munmap(0,0);
}
