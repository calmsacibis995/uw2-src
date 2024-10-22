/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)lprof:hdr/covfile.h	1.5"
#ifndef	_CACOVFILE


#define	EOD		'\0'		/* end of data delimiter */
					/* null object file name */
#define NULLOBJ		"############################################################"
#define OBJMAX		1		/* maximum number of object file 
						entries in a COVFILE */
#define ONAMESIZE	60		/* maximum object file name size */
#define UNKNOWN		0		/* unknown machine type */
#define MVAX		1 		/* VAX machine type */
#define MSIMPLEX	2		/* 3B20S machine type */
#define M32		3		/* 3B5 and 3B2 machine type */
#define M386            4               /* 386 machine type */
#define M860		5		/* 860 machine type */
#define M68K		6		/* 68K machine type */
#define M88K		7		/* 88K machine type */
#define MSPARC		8		/* SPARC machine type */
#define VERSION		3		/* current version number */

#include "mach_type.h"
#ifndef MACH_TYPE
#define MACH_TYPE	UNKNOWN
#endif
#ifndef CANAME
#define CANAME		"__coverage."
#endif



/**********COVFILE HEADER***********/

struct caCOV_HDR
   {	unsigned char	hdr_size;	/* size of COVFILE header */
	unsigned char	mach_type;	/* type of machine COVFILE built on */
	unsigned char 	ca_ver;		/* CA version number */
	unsigned char 	edit_flag;	/* edit flag */
	unsigned char	comp_flag;	/* completion flag */
	unsigned char	no_obj;		/* no. entries in object file section */
    };


/*******OBJECT FILE INFO ENTRY*******/

struct caOBJ_ENTRY
   {	char		name[ONAMESIZE];	/* name */
	long		time;			/* time of creation */
	PROF_MAGIC	magic_no;		/* magic number */
	long		offset;			/* offset to coverage data for
							this object file */
    };


/***********COVERAGE DATA************/

struct caCOV_DATA
   {
	unsigned short	fname_size;		/* length of function name */
	unsigned char	*func_name;		/* function name */
	    /* for XPROF */
	unsigned char	xca_size;		/* #bytes in coverage array */
	unsigned char	*xca_array;		/* ptr to coverag array */
	    /* for LPROF */

	/* * * * * *
	 * Dec-30-1988 rjp
	 * The three former fields were mapped into the five fields
	 * below, in order to remove the overloaded meanings.
	 * glossary: wd=word, ex.ct.=execution count, bblk=basic block.
	 * also: coverage data=line execution counts plus start line numbers.
	 */
	unsigned long	lca_words;		/* #wds of coverage data */
	unsigned long	lca_bblks;		/* #bblks (or ex.ct. array) */

	unsigned long	*lca_counts;		/* ptr to ex.ct. array */
	unsigned long	*lca_lineos;		/* ptr to line number array */
    };

	/*
	* This comment and the field definitions following, were
	* removed.	Dec-30-1988 rjp
	*
	*	The following fields have been overloaded to minimize
	*	the development time involved in the transition from
	*	COFF to ELF.  These structures are used by the dump
	*	routine in the prof library to write the coverage
	*	file and by lprof to read  the coverage file.
	*
	*	The dump routine assumes that lca_size is the size
	*	of the coverage structure (the count, the array,
	*	and the line numbers) and that lca_array points to
	*	the beginning of the structure.
	*
	*	The read routine reads it in the same format, but
	*	then translates it a form more compatible with the
	*	existing code (in which there was no line number
	*	information).  That is, lca_size becomes the number
	*	of entries in the array (which is the number of
	*	entries in the strucuture less one divided by two),
	*	lca_array points to the array rather than the
	*	structure (the first word of which is a count) and
	*	that lca_lineos (formerly undefined) points to the
	*	line number information.
	*
	*	See also the routine "revise" in exist.c.
	*/

	/* * * * * *
	 * unsigned long	lca_size;	!* #words in coverage array *!
	 * unsigned long	*lca_array;	!* ptr to coverag array *!
	 * unsigned long	*lca_lineos;	!* ptr to line number array *!
	 */

	/* * * * * *
	 * the above was removed. Dec-30-1988
	 */


#define	_CACOVFILE

#endif
