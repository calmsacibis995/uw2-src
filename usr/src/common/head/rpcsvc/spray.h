/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)head.usr:rpcsvc/spray.h	1.1.9.3"
#ident  "$Header:  "

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
/*	@(#)spray.h 1.4 89/03/24 SMI	*/

/*
 * spray.h
 */

/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#define SPRAYMAX 8845

struct spraytimeval {
	u_int sec;
	u_int usec;
};
typedef struct spraytimeval spraytimeval;
bool_t xdr_spraytimeval();

struct spraycumul {
	u_int counter;
	spraytimeval clock;
};
typedef struct spraycumul spraycumul;
bool_t xdr_spraycumul();

typedef struct {
	u_int sprayarr_len;
	char *sprayarr_val;
} sprayarr;
bool_t xdr_sprayarr();

#define SPRAYPROG ((u_long)100012)
#define SPRAYVERS ((u_long)1)
#define SPRAYPROC_SPRAY ((u_long)1)
extern void *sprayproc_spray_1();
#define SPRAYPROC_GET ((u_long)2)
extern spraycumul *sprayproc_get_1();
#define SPRAYPROC_CLEAR ((u_long)3)
extern void *sprayproc_clear_1();
