/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:inc/shlib.h	1.10"
/*
 * This header file contains all the macros and definitons
 *  needed for importing symbols for dynamic shared library
 * 
 */
#ifdef DSHLIB

#define malloc	(* _libc_malloc)		
#define free	(* _libc_free)	
#define realloc (* _libc_realloc)
#define calloc (* _libc_calloc)

#endif
