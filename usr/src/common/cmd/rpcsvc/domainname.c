/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)rpcsvc:domainname.c	1.1.5.3"
#ident  "$Header: domainname.c 1.2 91/06/27 $"

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 *	(c) 1986,1987,1988,1989,1990  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991  UNIX System Laboratories, Inc.
 * 	          All rights reserved.
 *  
 */


/*
 * domainname -- get (or set domainname)
 */
#include <stdio.h>

char domainname[256];
extern int errno;

main(argc,argv)
        char *argv[];
{
        int     myerrno;

        argc--;
        argv++;
        if (argc) {
                if (setdomainname(*argv,strlen(*argv)))
                        perror("setdomainname");
                myerrno = errno;
        } else {
                getdomainname(domainname,sizeof(domainname));
                myerrno = errno;
                printf("%s\n",domainname);
        }
        exit(myerrno);
	/* NOTREACHED */
}






