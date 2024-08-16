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

#ident	"@(#)kern:net/inet/in.c	1.11"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *	STREAMware TCP/IP Release 1.0
 *
 *	Copyrighted as an unpublished work.
 *	(c) Copyright 1990 INTERACTIVE Systems Corporation
 *	All Rights Reserved.
 *
 *	Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *	All Rights Reserved.
 *
 *	The copyright above and this notice must be preserved in all
 *	copies of this source code.  The copyright above does not
 *	evidence any actual or intended publication of this source
 *	code.
 *
 *	This is unpublished proprietary trade secret source code of
 *	Lachman Associates.  This source code may not be copied,
 *	disclosed, distributed, demonstrated or licensed except as
 *	expressly authorized by Lachman Associates.
 *
 *	System V STREAMS TCP was jointly developed by Lachman
 *	Associates and Convergent Technologies.
 */

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *		PROPRIETARY NOTICE (Combined)
 *
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 *
 *
 *
 *		Copyright Notice
 *
 * Notice of copyright on this source code product does not indicate
 * publication.
 *
 *	(c) 1986,1987,1988.1989,1990  Sun Microsystems, Inc
 *	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *		  All rights reserved.
 *
 */

#include <util/param.h>
#include <util/types.h>
#include <util/ksynch.h>
#include <io/stream.h>
#include <io/conf.h>
#include <net/inet/ip/ip.h>
#include <net/inet/ip/ip_str.h>
#include <net/inet/ip/ip_kern.h>
#include <util/mod/moddefs.h>

#include <io/ddi.h>		/* must come last */

extern struct ip_provider	provider[];
extern struct ip_provider	*lastprov;
extern lock_t	*ip_fastlck;

int in_interfaces;		/* number of external internet interfaces */

int	inetdevflag = D_NEW|D_MP;

#define DRVNAME "inet - Internet utilities module"

MOD_MISC_WRAPPER(in, NULL, NULL, DRVNAME);

/*
 * void in_strip_ip_opts(mblk_t *bp, mblk_t *moptbp)
 *	Strip out IP options, at higher level protocol in the kernel.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  bp		Message block containing IP struct with possible
 *			IP option list.
 *	  moptbp	If non-NULL place options here.
 *
 *	Locking:
 *	  None.
 */
void
in_strip_ip_opts(mblk_t *bp, mblk_t *moptbp)
{
	struct ip *ip;
	int i;
	caddr_t opts;
	int olen, optsoff = 0;

	ip = BPTOIP(bp);
	olen = (ip->ip_hl << 2) - sizeof (struct ip);
	opts = (caddr_t)(ip + 1);
	/*
	 * Copy the options if there's a place to put them.
	 */
	if (moptbp) {
		if (moptbp->b_wptr == moptbp->b_rptr) {
			bcopy(&ip->ip_dst, moptbp->b_wptr,
				sizeof (struct in_addr));
			moptbp->b_wptr += sizeof (struct in_addr);
		}
		/*
		 * Push the rest of the options in.  We don't have to worry
		 * about the other IP level options like we do the source
		 * routing, so just search for them and insert them into the
		 * mblk.  Notice that anything dealing with source routing is
		 * ignored.
		 *
		 * If we detect any sort of invalid length, then we just
		 * stop processing at that point.
		 */
		while (optsoff + 1 <= olen) {
			uint len;
			switch ((unsigned char)opts[optsoff]) {
			case IPOPT_LSRR:
			case IPOPT_SSRR:
				len = opts[optsoff + IPOPT_OLEN];
				if (len <= 0 || (len + optsoff) > olen) {
					goto out;
				}
				optsoff += len;
				break;

			case IPOPT_NOP:
				optsoff++;
				break;

			case IPOPT_EOL:
				*moptbp->b_wptr = opts[optsoff++];
				moptbp->b_wptr++;
				break;

			default:
				len = opts[optsoff + IPOPT_OLEN];
				if (len <= 0 || (len + optsoff) > olen) {
					goto out;
				}
				bcopy((caddr_t) & opts[optsoff],
				      (caddr_t) moptbp->b_wptr, len);
				moptbp->b_wptr += len;
				optsoff += len;
				break;
			}
		}
	}
out:
	/*
	 * Actually strip out the old options data.
	 */
	i = (bp->b_wptr - bp->b_rptr) - (sizeof (struct ip) + olen);
	bcopy(opts + olen, opts, (unsigned int)i);
	bp->b_wptr -= olen;
	ip->ip_hl = sizeof (struct ip) >> 2;
}
