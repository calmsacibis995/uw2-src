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

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.bin/talk/look_up.c	1.3.9.4"
#ident  "$Header: look_up.c 1.2 91/06/26 $"

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


#include "talk_ctl.h"

#ifdef SYSV
#define	bcopy(a,b,c)	memcpy((b),(a),(c))
#endif /* SYSV */

    /* see if the local daemon has a invitation for us */

CTL_RESPONSE swapresponse();

check_local()
{
    CTL_RESPONSE response;

	/* the rest of msg was set up in get_names */

    msg.ctl_addr = ctl_addr;

    if (!look_for_invite(&response)) {

	    /* we must be initiating a talk */

	return(0);
    }

        /*
	 * there was an invitation waiting for us, 
	 * so connect with the other (hopefully waiting) party 
	 */

    current_state = "Waiting to connect with caller";

    response = swapresponse(response);
    while (connect(sockt, &response.addr, sizeof(response.addr)) != 0) {
	if (errno == ECONNREFUSED) {

		/* the caller gave up, but the invitation somehow
		 * was not cleared. Clear it and initiate an 
		 * invitation. (We know there are no newer invitations,
		 * the talkd works LIFO.)
		 */

	    ctl_transact(rem_machine_addr, msg, DELETE, &response);
	    close(sockt);
	    open_sockt();
	    return(0);
	} else if (errno == EINTR) {
		/* we have returned from an interupt handler */
	    continue;
	} else {
	    p_error("Unable to connect with initiator");
	}
    }

    return(1);
}

    /* look for an invitation on 'machine' */

look_for_invite(response)
CTL_RESPONSE *response;
{
    struct in_addr machine_addr;

    current_state = "Checking for invitation on caller's machine";

    ctl_transact(rem_machine_addr, msg, LOOK_UP, response);

	/* the switch is for later options, such as multiple 
	   invitations */

    switch (response->answer) {

	case SUCCESS:

	    msg.id_num = response->id_num;
	    return(1);

	default :
		/* there wasn't an invitation waiting for us */
	    return(0);
    }
}

/*  
 * heuristic to detect if need to reshuffle CTL_RESPONSE structure
 */

struct ctl_response_runrise {
	char type;
	char answer;
	short junk;
	int id_num;
	struct sockaddr_in addr;
};

CTL_RESPONSE
swapresponse(rsp)
	CTL_RESPONSE rsp;
{
	struct ctl_response_runrise swaprsp;
	
	if (rsp.addr.sin_family != AF_INET) {
		bcopy(&rsp, &swaprsp, sizeof(CTL_RESPONSE));
		if (swaprsp.addr.sin_family == AF_INET) {
			rsp.addr = swaprsp.addr;
			rsp.type = swaprsp.type;
			rsp.answer = swaprsp.answer;
			rsp.id_num = swaprsp.id_num;
		} else {
#define swapshort(a) (((a << 8) | ((unsigned short) a >> 8)) & 0xffff)
			if ( swapshort(rsp.addr.sin_family) == AF_INET)
				rsp.addr.sin_family = swapshort(rsp.addr.sin_family);
		}
	}
	return (rsp);
}
