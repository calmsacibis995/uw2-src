/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_RPC_RPCENT_H	/* wrapper symbol for kernel use */
#define _NET_RPC_RPCENT_H	/* subject to change without notice */

#ident	"@(#)kern:net/rpc/rpcent.h	1.12"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *	rpcent.h, definitions for converting rpc program
 *	numbers to names etc.
 */

struct rpcent {
	char	*r_name;	/* name of server for this rpc program */
	char	**r_aliases;	/* alias list */
	int	r_number;	/* rpc program number */
};

#ifdef __STDC__

extern	struct rpcent	*getrpcbyname (char *);
extern	struct rpcent	*getrpcbynumber (int);
extern	struct rpcent	*getrpcent (void);
extern	int		setrpcent (int);
extern	void		endrpcent (void);

#else

extern	struct rpcent	*getrpcbyname ();
extern	struct rpcent	*getrpcbynumber ();
extern	struct rpcent	*getrpcent ();
extern	int		setrpcent();
extern	void		endrpcent();

#endif

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_RPC_RPCENT_H */
