/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_MOD_MOD_INTR_H	/* wrapper symbol for kernel use */
#define _UTIL_MOD_MOD_INTR_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:util/mod/mod_intr.h	1.2"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * The number of interrupt handlers each packet can hold.
 * This number should be large enough to make the need for
 * multiple packets a rarity.
 */
#define	MOD_NSI	8

/*
 * Packet of interrupt handlers for a given vector.
 * When needed, multiple packets can be allocated and linked together.
 */
struct	mod_shr_v	{
	void	(*msv_sih[MOD_NSI+1])();	/* pointers to interrupt handlers */
	int	msv_cnt;			/* number of handlers in this packet */
	struct	mod_shr_v	*msv_next;	/* pointer to next packet */
};

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_MOD_MOD_INTR_H */
