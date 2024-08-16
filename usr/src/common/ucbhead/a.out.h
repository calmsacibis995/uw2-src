/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)ucb:common/ucbhead/a.out.h	1.3"
#ident	"$Header: $"

/*******************************************************************

		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 


#ifndef _A_OUT_H
#define _A_OUT_H

#ifndef _NLIST_H
#include <nlist.h>	/* included for all machines */
#endif

#if defined(__STDC__)

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

#include "filehdr.h"
#include "aouthdr.h"
#include "scnhdr.h"
#include "reloc.h"
#include "linenum.h"
#include "syms.h"

#else
#include "filehdr.h"
#include "aouthdr.h"
#include "scnhdr.h"
#include "reloc.h"
#include "linenum.h"
#include "syms.h"
#endif  /* __STDC__ */

#endif /* _A_OUT_H */
