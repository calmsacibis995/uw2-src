/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	HASH_H
#define HASH_H

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.bootpd/hash.h	1.3"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/* hash.h */
/************************************************************************
          Copyright 1988, 1991 by Carnegie Mellon University

                          All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted, provided
that the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation, and that the name of Carnegie Mellon University not be used
in advertising or publicity pertaining to distribution of the software
without specific, written prior permission.

CARNEGIE MELLON UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
IN NO EVENT SHALL CMU BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.
************************************************************************/

/*
 * Generalized hash table ADT
 *
 * Provides multiple, dynamically-allocated, variable-sized hash tables on
 * various data and keys.
 *
 * This package attempts to follow some of the coding conventions suggested
 * by Bob Sidebotham and the AFS Clean Code Committee.
 */


/*
 * The user must supply the following:
 *
 *	1.  A comparison function which is declared as:
 *
 *		int compare(data1, data2)
 *		hash_datum *data1, *data2;
 *
 *	    This function must compare the desired fields of data1 and
 *	    data2 and return TRUE (1) if the data should be considered
 *	    equivalent (i.e. have the same key value) or FALSE (0)
 *	    otherwise.  This function is called through a pointer passed to
 *	    the various hashtable functions (thus pointers to different
 *	    functions may be passed to effect different tests on different
 *	    hash tables).
 *
 *	    Internally, all the functions of this package always call the
 *	    compare function with the "key" parameter as the first parameter,
 *	    and a full data element as the second parameter.  Thus, the key
 *	    and element arguments to functions such as hash_Lookup() may
 *	    actually be of different types and the programmer may provide a
 *	    compare function which compares the two different object types
 *	    as desired.
 *
 *	    Example:
 *
 *		int compare(key, element)
 *		char *key;
 *		struct some_complex_structure *element;
 *		{
 *		    return !strcmp(key, element->name);
 *		}
 *
 *		key = "John C. Doe"
 *		element = &some_complex_structure
 *		hash_Lookup(table, hashcode, compare, key);
 *
 *	2.  A hash function yielding an unsigned integer value to be used
 *	    as the hashcode (index into the hashtable).  Thus, the user
 *	    may hash on whatever data is desired and may use several
 *	    different hash functions for various different hash tables.
 *	    The actual hash table index will be the passed hashcode modulo
 *	    the hash table size.
 *
 *	    A generalized hash function, hash_HashFunction(), is included
 *	    with this package to make things a little easier.  It is not
 *	    guarenteed to use the best hash algorithm in existence. . . .
 */



/*
 * Various hash table definitions
 */


/*
 * Define "hash_datum" as a universal data type
 */
#ifdef __STDC__
typedef void hash_datum;
#else
typedef char hash_datum;
#endif

typedef struct hash_memberstruct  hash_member;
typedef struct hash_tblstruct     hash_tbl;
typedef struct hash_tblstruct_hdr hash_tblhdr;

struct hash_memberstruct {
    hash_member *next;
    hash_datum  *data;
};

struct hash_tblstruct_hdr {
    unsigned	size, bucketnum;
    hash_member *member;
};

struct hash_tblstruct {
    unsigned	size, bucketnum;
    hash_member *member;		/* Used for linear dump */
    hash_member	*table[1];		/* Dynamically extended */
};

/* ANSI function prototypes or empty arg list? */
#ifdef	__STDC__
#define P(args) args
#else
#define P(args) ()
#endif

typedef int (*hash_cmpfp) P((hash_datum *, hash_datum *));
typedef void (*hash_freefp) P((hash_datum *));

extern hash_tbl	  *hash_Init P((u_int tablesize));

extern void	   hash_Reset P((hash_tbl *tbl, hash_freefp));

extern unsigned	   hash_HashFunction P((u_char *str, u_int len));

extern int	   hash_Exists P((hash_tbl *, u_int code,
				  hash_cmpfp, hash_datum *key));

extern int	   hash_Insert P((hash_tbl *, u_int code,
				  hash_cmpfp, hash_datum *key,
				  hash_datum *element));

extern int	   hash_Delete P((hash_tbl *, u_int code,
				  hash_cmpfp, hash_datum *key,
				  hash_freefp));

extern hash_datum *hash_Lookup P((hash_tbl *, u_int code,
				  hash_cmpfp, hash_datum *key));

extern hash_datum *hash_FirstEntry P((hash_tbl *));

extern hash_datum *hash_NextEntry P((hash_tbl *));

#undef P

#endif	/* HASH_H */
