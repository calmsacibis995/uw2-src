#ident	"@(#)librpcsvc:common/lib/librpcsvc/rwall.x	1.1.6.2"
#ident  "$Header: rwall.x 1.2 91/06/26 $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*	PROPRIETARY NOTICE (Combined)
*
* This source code is unpublished proprietary information
* constituting, or derived under license from AT&T's UNIX(r) System V.
* In addition, portions of such source code were derived from Berkeley
* 4.3 BSD under license from the Regents of the University of
* California.
*
*
*
*	Copyright Notice 
*
* Notice of copyright on this source code product does not indicate 
*  publication.
*
*	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
*	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
*	(c) 1990,1991  UNIX System Laboratories, Inc.
*          All rights reserved.
*/ 
/* @(#)rwall.x 1.1 89/03/09 SMI 	*/

#ifdef RPC_HDR
%typedef char *wrapstring;
#endif

program WALLPROG {
	version WALLVERS {
		void	
		WALLPROC_WALL(wrapstring) = 2;

	} = 1;
} = 100008;
