/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/getdate_gd.c	1.1"

#ifdef __STDC__
	#pragma weak getdate_err = _getdate_err
#endif
#include "synonyms.h"

int	getdate_err = 0;  /* This global variable is set
		            * when an error condition is
			    * encountered and is used to
			    * differentiate among different
			    * error conditions. */
