/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:inc/hsearch.h	1.1"
/*LINTLIBRARY*/

#ifdef OPEN
#    undef CHAINED
#else
#ifndef CHAINED
#    define OPEN
#endif
#endif

#ifdef MULT
#    undef DIV
#else
#ifndef DIV
#    define MULT
#endif
#endif

typedef enum {
    FIND,		/* Find, if present */
    ENTER		/* Find; enter if not present */
} ACTION;
typedef char *POINTER;
typedef struct entry {	/* Hash table entry */
    POINTER key;
    POINTER data;
} ENTRY;

#ifdef CHAINED
typedef struct node {	/* Part of the linked list of entries */
	ENTRY item;
	struct node *next;
} NODE;
typedef NODE *TABELEM;
static NODE **table;	/* The address of the hash table */
static ENTRY *build();
#else
#ifdef OPEN
typedef ENTRY TABELEM;	/* What the table contains (TABle ELEMents) */
#endif
#endif

