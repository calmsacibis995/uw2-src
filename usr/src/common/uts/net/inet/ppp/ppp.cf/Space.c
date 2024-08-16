/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/inet/ppp/ppp.cf/Space.c	1.5"
#ident	"$Header: $"

#include <config.h>
#include <sys/types.h>
#include <sys/bitmasks.h>
#include <netinet/in_comp.h>

/*
 * PPP "minor devices" bitmask array.
 * PPP_UNITS (from System file).
 * Two is added to the number of PPP minor devices
 * to account for the control and logging streams.
 */
int	pppdev_cnt = PPP_UNITS + 2;
uint_t	pppdev_words = BITMASK_NWORDS(PPP_UNITS + 2);
uint_t	pppdev[BITMASK_NWORDS(PPP_UNITS + 2)];
/*
 * High water mark for PPP module_info.
 * PPPHIWAT is a tunable.
 */
ulong	ppphiwat = PPPHIWAT;
/*
 * Default PPP maximum transmission unit.
 * Should be a multiple of 2 + 40 bytes (IP header size).
 * PPPMTU is a tunable.
 */
int	pppmtu = PPPMTU;

int	ppp_rcv1172compat = 1;
int	ppp_snd1172compat = 1;
int	ppp_vjc_max_slot = MAX_STATES;
int	ppp_vjc_min_slot = MIN_STATES;
int	ppp_vjc_comp_slot = 1;
