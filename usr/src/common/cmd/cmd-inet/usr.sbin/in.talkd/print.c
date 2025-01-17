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

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.talkd/print.c	1.1.7.2"
#ident  "$Header: print.c 1.2 91/06/26 $"

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
 * 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991  UNIX System Laboratories, Inc.
 * 	          All rights reserved.
 *  
 */

/* debug print routines */

#include <stdio.h>
#include "ctl.h"

print_request(request)
CTL_MSG *request;
{
    
    printf("type is %d, l_user %s, r_user %s, r_tty %s\n",
	    request->type, request->l_name, request->r_name,
	    request->r_tty);
    printf("        id = %d\n", request->id_num);
    fflush(stdout);
}

print_response(response)
CTL_RESPONSE *response;
{
    printf("type is %d, answer is %d, id = %d\n\n", response->type, 
	    response->answer, response->id_num);
    fflush(stdout);
}
