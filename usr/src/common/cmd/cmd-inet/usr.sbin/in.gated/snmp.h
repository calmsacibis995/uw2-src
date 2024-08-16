/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.gated/snmp.h	1.2"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *      System V STREAMS TCP - Release 4.0
 *
 *      Copyright 1990 Interactive Systems Corporation,(ISC)
 *      All Rights Reserved.
 *
 *      Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *      All Rights Reserved.
 *
 *      System V STREAMS TCP was jointly developed by Lachman
 *      Associates and Convergent Technologies.
 */
/*      SCCS IDENTIFICATION        */
/*
 * $Header: /users/jch/src/gated/src/RCS/snmp.h,v 2.0 90/04/16 16:54:00 jch Exp $
 */

/********************************************************************************
*										*
*	GateD, Release 2							*
*										*
*	Copyright (c) 1990 by Cornell University				*
*	    All rights reserved.						*
*										*
*	    Royalty-free licenses to redistribute GateD Release 2 in		*
*	    whole or in part may be obtained by writing to:			*
*										*
*	    Center for Theory and Simulation in Science and Engineering		*
*	    Cornell University							*
*	    Ithaca, NY 14853-5201.						*
*										*
*	THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR		*
*	IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED		*
*	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.	*
*										*
*	GateD is based on Kirton's EGP, UC Berkeley's routing daemon		*
*	(routed), and DCN's HELLO routing Protocol.  Development of Release	*
*	2 has been supported by the National Science Foundation.		*
*										*
*	The following acknowledgements and thanks apply:			*
*										*
*	    Mark Fedor (fedor@psi.com) for the development and maintenance	*
*	    up to release 1.3.1 and his continuing advice.			*
*										*
*********************************************************************************
*      Portions of this software may fall under the following			*
*      copyrights: 								*
*										*
*	Copyright (c) 1988 Regents of the University of California.		*
*	All rights reserved.							*
*										*
*	Redistribution and use in source and binary forms are permitted		*
*	provided that the above copyright notice and this paragraph are		*
*	duplicated in all such forms and that any documentation,		*
*	advertising materials, and other materials related to such		*
*	distribution and use acknowledge that the software was developed	*
*	by the University of California, Berkeley.  The name of the		*
*	University may not be used to endorse or promote products derived	*
*	from this software without specific prior written permission.		*
*	THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR		*
*	IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED		*
*	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.	*
********************************************************************************/


#if	defined(AGENT_SNMP)

#define CORE_VALUE	0
#define	SNMP_REGISTER_INTERVAL	60

#define	ASN_SET_TYPE(dst, type)		*dst = type
#define	ASN_SET_LENGTH(dst, len)	*(dst+1) = len
#define	ASN_SET_VALUE(dst, vp)		memcpy((dst+2), (char *)vp, *(dst+1))
#define	ASN_LENGTH(dst)			*(dst+1) + 2
#define	ASN_INCR(dst)			dst += *(dst+1) + 2

#define AGENT_REG	1		/* Register variables */
#define AGENT_REQ	2		/* Request value of a variable */
#define AGENT_ERR	3		/* Error response */
#define AGENT_RSP	4		/* Returned value of a request */
#define	AGENT_REQN	5		/* Request next object identifier and value*/
#define	AGENT_REQO	6		/* Request object identifier and value */
#define AGENT_RSPO	7		/* Returned object identifier and value */
#define	AGENT_TRAP	8		/* Please generate this trap for me */
#define	AGENT_SET	9		/* Request to set a variable */
#define	AGENT_QUERY	10		/* Query for additional features */
#define	AGENT_REGSET	11		/* Register variables which can be set */

struct mibtbl {
    int length;				/* Length of object */
    char object[16];			/* Object */
    flag_t flags;			/* Flags */
    int (*function) ();			/* Function */
    const char *name;			/* Variable name */
};

#define	MIBF_WRITE	0x01		/* Variable is writable */
#define	MIBF_ONLY	0x02		/* Variable is single instance */

#define	TRACE_SNMPPKT(proto, direction, packet, length) { \
	if (trace_flags & TR_SNMP) \
		snmp_trace( proto, direction, packet, length); \
	}

#define	AGENT_SNMP_PORT	167

extern int doing_snmp;

extern void snmp_init();
extern void snmp_trap_egpNeighborLoss();

#endif				/* AGENT_SNMP */
