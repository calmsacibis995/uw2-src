/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/uniintrn.h	1.2"
/****************************************************************************
 *																			*
 * Library name:	NWLOCALE.LIB												*
 *																			*
 * Filename:		UNIINTRN.H												*
 *																			*
 *					This file has a	bunch of stuff in it that the unicode 	*
 *					library needs, which applications don't need to know 	*
 *					about.													*
 *																			*
 * Date Created:	October 18, 1991										*
 *																			*
 * Date Recreated:	July 24, 1992											*
 *																			*
 * Version:			2.00													*
 *																			*
 * Programmers:		Lloyd Honomichl											*
 *																			*
 * (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.			*
 *																			*
 * No part of this file may be duplicated, revised, translated, localized	*
 * or modified in any manner or compiled, linked or uploaded or downloaded	*
 * to or from any computer system without the prior written consent of 		*
 * Novell, Inc.																*
 *																			*
 ****************************************************************************/

#include "unicode.h"
/* #include "nwcaldef.h" */
#include "npackon.h"

/****************************************************************************/

/*
	Lookup types
*/
#define	LOOKUP_TABLE	1		/* Table lookup								*/
#define	LOOKUP_TREE		2		/* Tree lookup								*/

/****************************************************************************/

/*
	Header at the start of a rule file
*/
typedef	struct
{
	nuint8 	tag[47],			/* Tag that identifies a valid file			*/
			desc[79];			/* Description of the rule table			*/
	nuint16	length;				/* Length of rule table file, less header	*/
} HEADER;

/*
	Rule table structure in memory	
*/
typedef struct
{
	nuint8	tag[4],				/* Should be "Rule"							*/
			type;				   /* Type of rule table						*/
	nuint16	length,				/* Length of rule table in memory			*/
			defaultChar,		/* Default character						*/	
			numUnits,			/* Number of translation units				*/
			unitOffset[1];		/* Offsets of translation units				*/
} RULE;

/*
	Translation unit structure
*/
typedef struct
{
	nuint8	inputSize,			/* Size of input in bytes					*/
			outputSize,			/* Size of output in bytes					*/
			swap,				   /* Should input be swapped?					*/
			type;				   /* Is this a tree or table lookup?			*/
} TRAN;

/*
	Tree lookup structure
*/
typedef struct
{
	nuint8	inputSize,			/* Size of input in bytes					*/
			outputSize,			/* Size of output in bytes					*/
			swap,				   /* Should input be swapped?					*/
			type,				   /* Will be LOOKUP_TREE						*/
			numSlices,			/* Number of slices in this translation unit*/
			sliceSizes[1];		/* Slice sizes.  Tree data follows			*/
} TREE;

/*
	Table lookup structure
*/
typedef struct
{
	nuint8	inputSize,			/* Size of input in bytes					*/
			outputSize,			/* Size of output in bytes					*/
			swap,				   /* Should input be swapped?					*/
			type;				   /* Will be LOOKUP_TABLE						*/
	nuint16	numEntries;			/* Number of entries in the table			*/
} TABLE;

/****************************************************************************/

/*
	This is the tag we expect to find in the header of a valid rule file
*/
#define HEADER_TAG	"NetWare Unicode Rule Table File, Version 1.00\r\n"

/*
	Tag expected at the start of a rule table
*/
#define RTABLE_TAG "Rule"

/*
	Rule table types
*/
#define RT_LOCAL_UNICODE	1	/* Convert local to unicode					*/
#define RT_UNICODE_LOCAL	2	/* Convert unicode to local					*/
#define RT_UNICODE_COLL	   3	/* Convert unicode to collation	weights		*/
#define RT_UNICODE_MONO		4	/* Convert unicode to monocase unicode		*/
#define RT_UNICODE_NORMAL	5	/* Convert unicode to normalized unicode	*/

/****************************************************************************/
/*
	Internal function prototypes
*/

int N_FAR NWUniOpenReadOnly(nuint16 N_FAR *fileHandle,
                          char N_FAR *filePath);

int N_FAR NWUniRead(nuint16 fileHandle,
                  void N_FAR *buffer,
                  nuint16 N_FAR *readLength);

int N_FAR NWUniClose(nuint16 fileHandle);

/****************************************************************************/
/****************************************************************************/
#include "npackoff.h"
