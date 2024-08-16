/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/rpc/token.c	1.7"
#ident 	"$Header: $"

/*
 *	token.c, token resolution service routines.
 */

#include <net/rpc/types.h>
#include <net/rpc/token.h>
#include <net/xti.h>
#include <svc/errno.h>
#include <util/types.h>

#ifdef RPCESV

/*
 * get_remote_token(addr, type, val, size)
 *	Map attributes into reciever's token.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns the mapped token.
 *
 * Description:
 *	This routine is called on the sender to map attributes
 *	into reciever's token. 
 *
 * Parameters:
 *
 *
 *	addr			# address of remote host
 *	type			# attribute type to map
 *	val			# address of local representation
 *	size			# size of area in val
 */
s_token
get_remote_token(struct netbuf *addr, u_int type, caddr_t val, u_int size)
{
	if (val == NULL || size == 0 || addr == NULL)
		return(0);

	switch(type) {
		case PRIVS_T:
			/* LINTED pointer alignment */
			return (s_token)(*(pvec_t *)val);
		case SENS_T:
			/* LINTED pointer alignment */
			return (s_token)(*(lid_t *)val);
		case INFO_T:
			/* LINTED pointer alignment */
			return (s_token)(*(lid_t *)val);
		case INTEG_T:
			return (0);
		case NCS_T:
			return (0);
		case ACL_T:
			/*
			 * for now return 0. later we will need general
			 * token service.
			 */
			return (0);
		default:
			return (0);
	}
}

/*
 * map_local_token(token, type, resp, size)
 *	Map received token into local attribute.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns error on failure, 0 on success.
 *
 * Description:
 *	This routine is called on the receiver to map a
 *	received token into a local internal representation.
 *
 * Parameters:
 *
 *
 *	token			# the token to map
 *	type			# attribute type to map
 *	val			# address to write local representation
 *	size			# size of area in resp
 */
u_int
map_local_token(s_token token, u_int type, caddr_t resp, u_int size)
{
	/* LINTED pointer alignment */
	lid_t	*tmplid = (lid_t *)resp;

	/* LINTED pointer alignment */
	pvec_t	*tpvec = (pvec_t *)resp;

	if (resp == NULL || size == 0)
		return(0);

	switch(type) {
		case PRIVS_T:
			*tpvec = (pvec_t)token;
			return (sizeof(pvec_t));
		case SENS_T:
			*tmplid = (lid_t)token;
			return (sizeof(lid_t));
		case INFO_T:
			*resp = 0;
			return (0);
		case INTEG_T:
			*resp = 0;
			return (0);
		case NCS_T:
			*resp = 0;
			return (0);
		case ACL_T:
			*resp = 0;
			/*
			 * for now; will later need general token svc
			 */
			return (0);
		default:
			*resp = 0;
			return (0);
	}
}

#endif
