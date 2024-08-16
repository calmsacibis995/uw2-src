/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/nwcommon.h	1.3"
#ifndef _NET_NW_COMMON_H  /* wrapper symbol for kernel use */
#define _NET_NW_COMMON_H  /* subject to change without notice */

#ident	"$Id: nwcommon.h,v 1.7 1994/03/16 15:26:59 vtag Exp $"

/*
 * Copyright 1991, 1992 Unpublished Work of Novell, Inc.
 * All Rights Reserved.
 *
 * This work is an unpublished work and contains confidential,
 * proprietary and trade secret information of Novell, Inc. Access
 * to this work is restricted to (I) Novell employees who have a
 * need to know to perform tasks within the scope of their
 * assignments and (II) entities other than Novell who have
 * entered into appropriate agreements.
 *
 * No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 */

#ifdef _KERNEL_HEADERS
#include <net/nw/kdrivers.h>
#else
#include "kdrivers.h"
#endif

#ifdef _KERNEL

/*
**	Common macros for streams modules
**
**	DATA_SIZE	= size of data in this message block
**	DATA_BSIZE	= max size of available message space in message block
**	DATA_REMAINING	= available message space remaining message block
**	MTYPE		= type of message block
**	BIGENOUGH	= Tests if max available message space in message block
**					is larger than SIZE.  If so, and if the message block
**					is not referenced elsewhere, set read and write pointers
**					to top of data area and return 1.  If not so, return 0.
*/
#define DATA_SIZE(mp)	((mp)->b_wptr - (mp)->b_rptr)
#define DATA_REMAINING(mp)	((mp)->b_datap->db_lim - (mp)->b_wptr)
#define DATA_BSIZE(mp) ((mp)->b_datap->db_lim - (mp)->b_datap->db_base)
#define MTYPE(x) ((x)->b_datap->db_type)
#define BIGENOUGH(MP,SIZE) (((MP->b_datap->db_ref == 1) &&\
	(MP->b_datap->db_lim - MP->b_datap->db_base >= SIZE))?\
	MP->b_rptr = MP->b_wptr = MP->b_datap->db_base,1:0)

/*
**	Define NWSLOG
*/
#ifdef NWSLOG
#undef NWSLOG
#endif

/*
**	Tracing defines
**
**	DEBUG_TRACE - Enables NVLT tracing for the whole kernel
**	NTR_TRACING - Enables NWU portable tracing driver
*/

#ifdef DEBUG_TRACE
# define NWSLOG(args)			NVLTstrlog args
# define NWKLOG(args)			NVLTstrlog args
#else
# ifdef NTR_TRACING
#  define NWSLOG(args)			NTRstrlog args
#  define NWKLOG(args)			NTRstrlog args
# else
#  define NWKLOG(args)
#  ifdef DEBUG
#   define NWSLOG(params)	/*	strlog params */
#  else
#   define NWSLOG(params)
#  endif
# endif /* ndef DEBUG_TRACE */
#endif /* ndef NTR_TRACING */

#ifndef STRLOG
#if defined(DEBUG_TRACE)
#define STRLOG              NVLTstrlog
#elif   defined(DEBUG)
#define STRLOG              strlog
#else
#define STRLOG
#endif
#endif  /* STRLOG */

#endif /* _KERNEL */

/*
**	Define NWSLOG tracing level values for NetWare
*/
#define PNW_ERROR			0
#define PNW_DROP_PACKET		1
#define PNW_ENTER_ROUTINE	2
#define PNW_EXIT_ROUTINE	3
#define PNW_ASSIGNMENT		4
#define PNW_SWITCH_CASE		5
#define PNW_SWITCH_DEFAULT	6
#define PNW_PROTO_HEX		7
#define PNW_DATA_HEX		8
#define PNW_DATA_ASCII		9
#define PNW_ALL				10


#ifdef NTR_TRACING
/*
**	Values for strlog using NTR
*/
#define IPXID 				(NTRT_Strlog | NTRM_ipx)
#define LIPMXID				(NTRT_Strlog | NTRM_lipmx)
#define RIPXID 				(NTRT_Strlog | NTRM_ripx)
#define SPXID 				(NTRT_Strlog | NTRM_spx)
#define NCPIPXID			(NTRT_Strlog | NTRM_ncpipx)
#define NWETCID				(NTRT_Strlog | NTRM_nwetc)
#define NEMUXID				(NTRT_Strlog | NTRM_nemux)
#define ELAPID				(NTRT_Strlog | NTRM_elap)
#define DDPID				(NTRT_Strlog | NTRM_ddp)
#define ATPID				(NTRT_Strlog | NTRM_atp)
#define PAPID				(NTRT_Strlog | NTRM_pap)
#define ASPID				(NTRT_Strlog | NTRM_asp)
#define NBIOID				(NTRT_Strlog | NTRM_nbio)
#define NBDGID				(NTRT_Strlog | NTRM_nbdg)
#define NBIXID				(NTRT_Strlog | NTRM_nbix)
#define SFDID				(NTRT_Strlog | NTRM_sfd)
#else
/*
**	Values for standard strlog
*/
#define IPXID 				M_IPXID
#define LIPMXID				M_LIPMXID
#define RIPXID 				M_RIPXID
#define SPXID				M_SPXID
#define NCPIPXID			M_NCPIPXID
#define NWETCID				M_NWETCID
#define NEMUXID				M_NEMUXID
#define ELAPID				M_ELAPID
#define DDPID				M_DDPID
#define ATPID				M_ATPID
#define PAPID				M_PAPID
#define ASPID				M_ASPID
#define NBIOID				M_NBIOID
#define NBDGID				M_NBDGID
#define NBIXID				M_NBIXID
#endif /* NTR_TRACING */

#endif /* _NET_NW_COMMON_H */
