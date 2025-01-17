/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.bin/telnet/terminal.c	1.1"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *      System V STREAMS TCP - Release 4.0
 *
 *  Copyright 1990 Interactive Systems Corporation,(ISC)
 *  All Rights Reserved.
 *
 *      Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *      All Rights Reserved.
 *
 *      The copyright above and this notice must be preserved in all
 *      copies of this source code.  The copyright above does not
 *      evidence any actual or intended publication of this source
 *      code.
 *
 *      This is unpublished proprietary trade secret source code of
 *      Lachman Associates.  This source code may not be copied,
 *      disclosed, distributed, demonstrated or licensed except as
 *      expressly authorized by Lachman Associates.
 *
 *      System V STREAMS TCP was jointly developed by Lachman
 *      Associates and Convergent Technologies.
 */

/*      SCCS IDENTIFICATION        */

/*
 * Copyright (c) 1988, 1990 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted provided
 * that: (1) source distributions retain this entire copyright notice and
 * comment, and (2) distributions including binaries display the following
 * acknowledgement:  ``This product includes software developed by the
 * University of California, Berkeley and its contributors'' in the
 * documentation or other materials provided with the distribution and in
 * all advertising materials mentioning features or use of this software.
 * Neither the name of the University nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
static char sccsid[] = "@(#)terminal.c	5.2 (Berkeley) 3/1/91";
#endif /* not lint */

#include <arpa/telnet.h>
#include <sys/types.h>

#include "ring.h"

#include "externs.h"
#include "types.h"

Ring		ttyoring, ttyiring;
unsigned char	ttyobuf[2*BUFSIZ], ttyibuf[BUFSIZ];

int termdata;			/* Debugging flag */

#ifdef	USE_TERMIO
# ifndef VDISCARD
cc_t termFlushChar;
# endif
# ifndef VLNEXT
cc_t termLiteralNextChar;
# endif
# ifndef VSUSP
cc_t termSuspChar;
# endif
# ifndef VWERASE
cc_t termWerasChar;
# endif
# ifndef VREPRINT
cc_t termRprntChar;
# endif
# ifndef VSTART
cc_t termStartChar;
# endif
# ifndef VSTOP
cc_t termStopChar;
# endif
# ifndef VEOL
cc_t termForw1Char;
# endif
# ifndef VEOL2
cc_t termForw2Char;
# endif
# ifndef VSTATUS
cc_t termAytChar;
# endif
#else
cc_t termForw2Char;
cc_t termAytChar;
#endif

/*
 * initialize the terminal data structures.
 */

    void
init_terminal()
{
    if (ring_init(&ttyoring, ttyobuf, sizeof ttyobuf) != 1) {
	exit(1);
    }
    if (ring_init(&ttyiring, ttyibuf, sizeof ttyibuf) != 1) {
	exit(1);
    }
    autoflush = TerminalAutoFlush();
}


/*
 *		Send as much data as possible to the terminal.
 *
 *		Return value:
 *			-1: No useful work done, data waiting to go out.
 *			 0: No data was waiting, so nothing was done.
 *			 1: All waiting data was written out.
 *			 n: All data - n was written out.
 */


    int
ttyflush(drop)
    int drop;
{
    register int n, n0, n1;

    n0 = ring_full_count(&ttyoring);
    if ((n1 = n = ring_full_consecutive(&ttyoring)) > 0) {
	if (drop) {
	    TerminalFlushOutput();
	    /* we leave 'n' alone! */
	} else {
	    n = TerminalWrite(ttyoring.consume, n);
	}
    }
    if (n > 0) {
	if (termdata && n) {
	    Dump('>', ttyoring.consume, n);
	}
	/*
	 * If we wrote everything, and the full count is
	 * larger than what we wrote, then write the
	 * rest of the buffer.
	 */
	if (n1 == n && n0 > n) {
		n1 = n0 - n;
		if (!drop)
			n1 = TerminalWrite(ttyoring.bottom, n1);
		n += n1;
	}
	ring_consumed(&ttyoring, n);
    }
    if (n < 0)
	return -1;
    if (n == n0) {
	if (n0)
	    return -1;
	return 0;
    }
    return n0 - n + 1;
}


/*
 * These routines decides on what the mode should be (based on the values
 * of various global variables).
 */


    int
getconnmode()
{
    extern int linemode;
    int mode = 0;
#ifdef	KLUDGELINEMODE
    extern int kludgelinemode;
#endif

    if (In3270)
	return(MODE_FLOW);

    if (my_want_state_is_dont(TELOPT_ECHO))
	mode |= MODE_ECHO;

    if (localflow)
	mode |= MODE_FLOW;

    if (my_want_state_is_will(TELOPT_BINARY))
	mode |= MODE_INBIN;

    if (his_want_state_is_will(TELOPT_BINARY))
	mode |= MODE_OUTBIN;

#ifdef	KLUDGELINEMODE
    if (kludgelinemode) {
	if (my_want_state_is_dont(TELOPT_SGA)) {
	    mode |= (MODE_TRAPSIG|MODE_EDIT);
	    if (dontlecho && (clocks.echotoggle > clocks.modenegotiated)) {
		mode &= ~MODE_ECHO;
	    }
	}
	return(mode);
    }
#endif
    if (my_want_state_is_will(TELOPT_LINEMODE))
	mode |= linemode;
    return(mode);
}

    void
setconnmode(force)
    int force;
{
#ifdef	ENCRYPT
    static int enc_passwd = 0;
#endif
    register int newmode;

    newmode = getconnmode()|(force?MODE_FORCE:0);

    TerminalNewMode(newmode);

#ifdef  ENCRYPT
    if ((newmode & (MODE_ECHO|MODE_EDIT)) == MODE_EDIT) {
	if (my_want_state_is_will(TELOPT_ENCRYPT)
				&& (enc_passwd == 0) && !encrypt_output) {
	    encrypt_request_start(0, 0);
	    enc_passwd = 1;
	}
    } else {
	if (enc_passwd) {
	    encrypt_request_end();
	    enc_passwd = 0;
	}
    }
#endif

}


    void
setcommandmode()
{
    TerminalNewMode(-1);
}
