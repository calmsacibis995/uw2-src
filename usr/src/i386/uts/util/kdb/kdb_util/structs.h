/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_KDB_KDB_STRUCTS_H	/* wrapper symbol for kernel use */
#define _UTIL_KDB_KDB_STRUCTS_H	/* subject to change without notice */

#ident	"@(#)kern-i386:util/kdb/kdb_util/structs.h	1.4"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/* the following structures are used for symbolic disassembly */

/* structures for holding information about external and static symbols */

/* extern-static hash table structure */
typedef	struct node essymrec;
typedef struct node *pessymrec;
struct	node {
			char 	*name;
			long	symval;
			struct node	*next;
};

/* extern-static union-array list structure */
typedef struct ua	uarec;
typedef	struct ua	*puarec;
struct	ua {
			char	*name;
			long	symval;
			char	type;
			int	range;
			struct ua	*next;
};

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_KDB_KDB_STRUCTS_H */
