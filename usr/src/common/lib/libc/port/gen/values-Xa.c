/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/values-Xa.c	1.3"

#include "synonyms.h"
#include <math.h>

/* variables which differ depending on the
 * compilation mode
 *
 * ANSI conforming mode
 * This file is linked into the a.out immediately following
 * the startup routine if the -Xa compilation mode is selected
 */

 const enum version _lib_version = ansi_1;
