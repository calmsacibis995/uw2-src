/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _A_OUT_H
#define _A_OUT_H

#ident	"@(#)sgs-head:common/head/a.out.h	2.12.2.6"

#if defined(__cplusplus)
extern "C" {
#endif


#ifndef _NLIST_H
#include <nlist.h>	/* included for all machines */
#endif

 /*		COMMON OBJECT FILE FORMAT

	For a description of the common object file format (COFF) see
	the Common Object File Format chapter of the UNIX System V Support 
	Tools Guide

 		OBJECT FILE COMPONENTS

 	HEADER FILES:
 			/usr/include/filehdr.h
			/usr/include/aouthdr.h
			/usr/include/scnhdr.h
			/usr/include/reloc.h
			/usr/include/linenum.h
			/usr/include/syms.h
			/usr/include/storclass.h

	STANDARD FILE:
			/usr/include/a.out.h    "object file" 
   */

#include <filehdr.h>
#include <aouthdr.h>
#include <scnhdr.h>
#include <reloc.h>
#include <linenum.h>
#include <syms.h>

#if defined(__cplusplus)
        }
#endif

#endif /* _A_OUT_H */
