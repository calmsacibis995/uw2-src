/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NLIST_H
#define _NLIST_H

#ident	"@(#)sgs-head:common/head/nlist.h	1.8.4.1"

#if defined(__cplusplus)
extern "C" {
#endif



struct nlist
{
        char            *n_name;        /* symbol name */
        long            n_value;        /* value of symbol */
        short           n_scnum;        /* section number */
        unsigned short  n_type;         /* type and derived type */
        char            n_sclass;       /* storage class */
        char            n_numaux;       /* number of aux. entries */
};

#if defined(__STDC__)
extern int nlist(const char *, struct nlist *);
#endif  /* __STDC__ */

#if defined(__cplusplus)
        }
#endif
#endif 	/* _NLIST_H */
