#ident	"@(#)librpcsvc:common/lib/librpcsvc/spray.x	1.1.6.2"
#ident  "$Header: spray.x 1.2 91/06/26 $"

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
/* @(#)spray.x 1.3 88/02/08 Copyr 1987 Sun Micro */

/*
 * Spray a server with packets
 * Useful for testing flakiness of network interfaces
 */

const SPRAYMAX = 8845;	/* max amount can spray */

/*
 * GMT since 0:00, 1 January 1970
 */
struct spraytimeval {
	unsigned int sec;
	unsigned int usec;
};

/*
 * spray statistics
 */
struct spraycumul {
	unsigned int counter;
	spraytimeval clock;
};

/*
 * spray data
 */
typedef opaque sprayarr<SPRAYMAX>;

program SPRAYPROG {
	version SPRAYVERS {
		/*
		 * Just throw away the data and increment the counter
		 * This call never returns, so the client should always 
		 * time it out.
		 */
		void
		SPRAYPROC_SPRAY(sprayarr) = 1;

		/*
		 * Get the value of the counter and elapsed time  since
		 * last CLEAR.
		 */
		spraycumul	
		SPRAYPROC_GET(void) = 2;

		/*
		 * Clear the counter and reset the elapsed time
		 */
		void
		SPRAYPROC_CLEAR(void) = 3;
	} = 1;
} = 100012;
