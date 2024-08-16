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

#ident	"@(#)ucb:common/ucblib/libc/port/sys/setreid.c	1.2"
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


setreuid(ruid, euid) 
        int     ruid;
        int     euid;
{
        /*
         * Priv access checking are done by the system calls
	 * If either setuid or seteuid fails, it returns with -1.
         */

	int	error = 0;

        if ( ruid != -1 ) { 
		error = setuid(ruid);
		if ( error < 0 )
			return (error);
	} 
	if ( euid != -1 ) {
		error = seteuid(euid);
		if ( error < 0 )
			return (error);
	}
	return (0);
}


setregid(rgid, egid) 
        int     rgid;
        int     egid;
{
        /*
         * Priv access checking are done by the system calls
         * If either setgid or setegid fails, it returns with -1. 
         */
 
	int	error = 0;
        if ( rgid != -1 ) { 
                error = setgid(rgid);    
                if ( error < 0 ) 
                        return (error);
        } 
        if ( egid != -1 ) {
                error = setegid(egid);    
                if ( error < 0 ) 
                        return (error);
        }
        return (0);
} 
