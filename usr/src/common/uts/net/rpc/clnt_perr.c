/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/rpc/clnt_perr.c	1.6"
#ident 	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *	clnt_perr.c, miscellaneous error routines for rpc.
 */

#include <util/types.h>
#include <net/rpc/types.h>
#include <net/rpc/auth.h>
#include <net/rpc/clnt.h>

extern	char		*strcpy();

/*
 * clnt_sperrno(stat)
 *	Given error status, return error string.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns an error string.
 *
 * Description:
 *	Given error status, return error string. It
 *	is for use by client side rpc.
 *
 * Parameters:
 *
 *	stat			# error status
 *
 */
char *
clnt_sperrno(enum clnt_stat stat)
{
	switch (stat) {
		case RPC_SUCCESS: 
			return ("RPC: Success"); 
		case RPC_CANTENCODEARGS: 
			return ("RPC: Can't encode arguments");
		case RPC_CANTDECODERES: 
			return ("RPC: Can't decode result");
		case RPC_CANTSEND: 
			return ("RPC: Unable to send");
		case RPC_CANTRECV: 
			return ("RPC: Unable to receive");
		case RPC_TIMEDOUT: 
			return ("RPC: Timed out");
		case RPC_INTR:
			return ("RPC: Interrupted");
		case RPC_VERSMISMATCH: 
			return ("RPC: Incompatible versions of RPC");
		case RPC_AUTHERROR: 
			return ("RPC: Authentication error");
		case RPC_PROGUNAVAIL: 
			return ("RPC: Program unavailable");
		case RPC_PROGVERSMISMATCH: 
			return ("RPC: Program/version mismatch");
		case RPC_PROCUNAVAIL: 
			return ("RPC: Procedure unavailable");
		case RPC_CANTDECODEARGS: 
			return ("RPC: Server can't decode arguments");
		case RPC_SYSTEMERROR: 
			return ("RPC: Remote system error");
		case RPC_UNKNOWNHOST: 
			return ("RPC: Unknown host");
		case RPC_UNKNOWNPROTO:
			return ("RPC: Unknown protocol");
		case RPC_PMAPFAILURE: 
			return ("RPC: Port mapper failure");
		case RPC_PROGNOTREGISTERED: 
			return ("RPC: Program not registered");
		case RPC_FAILED: 
			return ("RPC: Failed (unspecified error)");
	}

	return ("RPC: (unknown error code)");
}
