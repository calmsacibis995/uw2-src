/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/import_def.c	1.1"

#include "synonyms.h"
#include <stdlib.h>

VOID * (* _libc_malloc)() = &malloc;
VOID * (* _libc_realloc)() = &realloc;
VOID * (* _libc_calloc)() = &calloc;
void (* _libc_free)() = &free;
