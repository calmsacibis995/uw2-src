/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:uniloc.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/uniloc.c,v 1.1 1994/09/26 17:21:36 rebekah Exp $"
/****************************************************************************
 *																			*
 * Library name:	NWLOCALE.LIB												*
 *																			*
 * Filename:		UNILOC.C												*
 *																			*
 * Date Created:	October 18, 1991										*
 *																			*
 * Date Recreated:	July 24, 1992											*
 *																			*
 * Seriously																*
 * Optimized:		September 25, 1992										*
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

/*
	Description:	This function perform conversions from unicode to local
					code pages.  It uses the rule table specified by "rule"
					to perform the conversion.
					
					The "output" buffer will be filled with the converted
					data and will be terminated by an 8 bit null character.
					The length of "output" will be no more than "size" bytes,
					including the terminating null character.

					If no rule exists for the conversion of any character in
					the input, (quite likely) then the value of "noMap" will
					be placed in the output buffer.  If "noMap" is zero the
					default character from the rule table will be placed in
					the output buffer.  If "noMap" is zero and no default
					value is specified in the rule table, the function
					returns UNI_NO_DEFAULT.

	If successful:	Returns zero.  Results of conversion are stored in "out".
					Number of bytes in the output (including the ending null)
					is stored in "len".

	Otherwise:		Returns one of the following errors:

					UNI_HANDLE_BAD:			The rule handle doesn't point to
											a valid rule table.

					UNI_HANDLE_MISMATCH:	The rule handle points to a rule
											table that is not for unicode to
											local conversion.

					UNI_NO_DEFAULT:			A character was found that could
											not be converted, the noMap
											character was zero and no default
											rule was found in the rule table.
											In this case, "len" will contain
											the number of bytes placed in
											"out" before the error occured.
*/

/****************************************************************************/

#ifdef WIN32
#define  NOATOM                  // Atom Manager routines
#define  NOCLIPBOARD             // Clipboard routines
#define  NOCOMM                  // to eliminate lint warnings
#define  NODEFERWINDOWPOS        // DeferWindowPos routines
#define  NOGDICAPMASKS           // CC_*, LC_*, PC_*, CP_*, etc
#define  NOKANJI                 // Kanji support stuff
#define  NOMETAFILE              // typedef METAFILEPICT
#define  NOMINMAX                // NO min(a,b) or max(a,b)
#define  NOOPENFILE              // OpenFile(), OemToAnsi(), AnsiToOem()
#define  NOPROFILER              // Profiler interface
#define  NOSOUND                 // Sound Driver routines
#undef   OEMRESOURCE             // OEM Resource values
#undef   NOLSTRING               // using lstrlen()

#include <windows.h>
#include "ntypes.h"
#include "unicode.h"

int N_API NWUnicodeToLocal(

	void N_FAR *ruleBuffer,		/* Handle to the rule table					*/
	unsigned char N_FAR *out,				/* Buffer for resulting local text			*/
	size_t size,				   /* Size of the buffer for resulting text	*/
	unicode N_FAR *in,			/* Unicode to be converted					*/
	unsigned char noMap,			/* Result for non mapable characters		*/
	size_t N_FAR *len)			/* Length of output							*/
{
	int	flags;

   ruleBuffer = ruleBuffer;

   *len = (size_t) WideCharToMultiByte(CP_ACP, 0, (LPWSTR)in, -1,
                          (LPSTR) out, (int) size, &noMap, &flags);

   return 0;
}

#else
#ifndef MACINTOSH
#ifndef NWNLM

#include "locdefs.h"
#if defined (__BORLANDC__)
# include <mem.h>
#else
# include <string.h>
#endif

#include "ntypes.h"
#include "uniintrn.h"
#include "localias.h"
#include "unicode.h"

/****************************************************************************/

#ifndef TRUE
#define	TRUE	1
#endif

#ifndef FALSE
#define	FALSE	0
#endif

/****************************************************************************/

/*
	Local functions
*/
static int RunTranUnit(    /* Execute a translation unit				*/
   TRAN N_FAR *tran,       /* Pointer to translation unit				*/
   unicode     input,      /* Unicode character to be converted		*/
   unsigned char N_FAR *target);    /* Buffer for target data					*/

static int ByteTree(       /* Perform a tree lookup for a byte output	*/
   TREE N_FAR *tree,       /* Pointer to tree data						*/
   unicode     input,      /* Input to be looked up					*/
   char N_FAR *output);    /* Buffer for the output					*/

static int WordTree(       /* Perform a tree lookup for a word output	*/
   TREE N_FAR *tree,       /* Pointer to tree data						*/
   unicode     input,      /* Input to be looked up					*/
   char N_FAR *output);    /* Buffer for the output					*/

static int ByteTable(      /* Perform a table lookup for a byte output	*/
   TABLE N_FAR *table,     /* Table data to be used					*/
   unicode      input,     /* Input to be looked up					*/
   char N_FAR  *output);   /* Buffer for the output					*/

static int WordTable(      /* Perform a table lookup for a word output	*/
   TABLE N_FAR *table,     /* Table data to be used					*/
   unicode      input,     /* Input to be looked up					*/
   char N_FAR  *output);   /* Buffer for the output					*/

/****************************************************************************/

/*
	Description:	Convert Unicode into local code

	Called by:		Applications that are too ignorant to do it themselves
*/

int N_API NWUnicodeToLocal(

	void N_FAR *ruleBuffer,		/* Handle to the rule table					*/
	unsigned char N_FAR *out,				/* Buffer for resulting local text			*/
	size_t size,				   /* Size of the buffer for resulting text	*/
	unicode N_FAR *in,			/* Unicode to be converted					*/
	unsigned char noMap,			/* Result for non mapable characters		*/
	size_t N_FAR *len)			/* Length of output							*/

{

   int         ccode,      /* Error code								*/
               i,          /* Loop variable							*/
               emitted;    /* Bytes emitted by translation unit		*/
   unicode     input;      /* Unicode input							*/
   RULE N_FAR *rule;       /* Pointer to rule table					*/
   TRAN N_FAR *tran;       /* Pointer to a translation unit			*/

	/*
		Convert the rule handle to an easier to use form
	*/
	rule = ruleBuffer;

	/*
		Nothing converted so far
	*/
	*len = 0;

	/*
		Make sure the rule handle is valid
	*/
   if (rule == NULL || nwmemcmp(rule->tag, RTABLE_TAG, 4))
		ccode = UNI_HANDLE_BAD;

	/*
		Make sure it matches the operation
	*/
	else if (rule->type != RT_UNICODE_LOCAL)
		ccode = UNI_HANDLE_MISMATCH;

	/*
		Rule handle is OK
	*/
	else
	{
		/*
			Convert until we reach the end of the source string, or the
			target buffer is full, or we get an error
		*/
		ccode = 0;
		while (*in && size > 1 && ccode == 0)
		{
			/*
				See if any translation unit handles the next chunk of input
			*/
			emitted = 0;
			input = *in++;
			for ( i = 0 ; (nuint16) i < rule->numUnits && emitted == 0 ; i++ )
			{
				tran = (TRAN N_FAR *)((char N_FAR *)rule + rule->unitOffset[i]);
				emitted = RunTranUnit(tran, input, out);

				/*
					If the translation unit emitted two bytes and there
					was only room for one, erase the emitted bytes and quit.

					Otherwise, just record how many bytes where emitted
				*/
				if (emitted == 2 && size == 2)
				{
					*out = 0;
					size = 0;
				}
				else
				{
					out += emitted;
					size -= emitted;
					*len += emitted;
				}
			}

			/*
				If no translation unit handled the current source character
				then we need to use a default
			*/
			if (emitted == 0)
			{
				/*
					Decide what to stick in the output
				*/
				if (noMap)
				{
					*out++ = noMap;
					*len += 1;
					size--;
				}
				else if (rule->defaultChar)
				{
					*out++ = (char) rule->defaultChar;
					*len += 1;
					size--;
				}
				else
					ccode = UNI_NO_DEFAULT;
			}
		}

		/*
			Null terminate the result
		*/
		*out = 0;
		*len += 1;
	}

	/*
		Return the result
	*/
	return (ccode);

}

/****************************************************************************/

/*
	Description:	Run a translation unit

	Called by:		NWUnicodeToLocal
*/

static int RunTranUnit(    /* Execute a translation unit				*/
   TRAN N_FAR *tran,       /* Pointer to translation unit				*/
   unicode     input,      /* Unicode character to be converted		*/
   unsigned char N_FAR *output)     /* Buffer for target data					*/
{

	int		 emitted;		/* How many bytes of output were emitted	*/

	/*
		Swap input, if necessary
	*/
	if (tran->swap)
		input = (unicode)((input & 0xFF00) >> 8) + (unicode)((input & 0x00FF) << 8);

	/*
		Call the correct routine, based on translation type and input size
	*/
	if (tran->outputSize == 1)
	{
		if (tran->type == LOOKUP_TREE)
			emitted = ByteTree((TREE N_FAR *)tran, input, (char N_FAR *)output);
		else
			emitted = ByteTable((TABLE N_FAR *)tran, input, (char N_FAR *)output);
	}
	else
	{
		if (tran->type == LOOKUP_TREE)
			emitted = WordTree((TREE N_FAR *)tran, input, (char N_FAR *)output);
		else
			emitted = WordTable((TABLE N_FAR *)tran, input, (char N_FAR *)output);
	}

	/*
		Return the results
	*/
	return (emitted);

}

/****************************************************************************/

/*
	Description:	Do a one byte tree lookup

	Called by:		RunTranUnit
*/

static int ByteTree(       /* Perform a tree lookup for a byte output	*/
   TREE N_FAR *t,          /* Pointer to tree data						*/
   unicode     input,      /* Input to be looked up					*/
   char N_FAR *output)    /* Buffer for the output					*/
{

   int    subs[8],         /* Index values for each slice				*/
			 i,					/* Loop variable							*/
			 size,				/* Size of a slice							*/
          slices,          /* Number of slices							*/
			 emitted = 0;		/* How many bytes did we emit?				*/
   nuint16 N_FAR *index,      /* Pointer to the data for the tree			*/
        N_FAR *treeData;   /* Pointer to tree data						*/

	/*
	 	Copy number of slices for some speed
	*/
	slices = t->numSlices;

	/*
		Figure out what the indexes should be
	*/
	for ( i = slices ; i ; i-- )
	{
		size = t->sliceSizes[i - 1];
		subs[i - 1] = input & ((1 << size) - 1);
		input >>= size;
	}

	/*
		Get a pointer to the tree data
	*/
	treeData = (nuint16 N_FAR *)(t->sliceSizes + slices);

	/*
		First index is at the top of the tree
	*/
	index = treeData;

	/*
		Do lookups till we get to the last index, or a null entry
	*/
	for ( i = 0 ; i < slices - 1 ; i++ )
		index = treeData + index[subs[i]];

	/*
		Now get the value that sits there
	*/
	*output = *((char N_FAR *)index + subs[slices - 1]);

	/*
		Did we get anything?
	*/
	if (*output)
		emitted = 1;

	/*
		Return the results
	*/
	return (emitted);

}

/****************************************************************************/

/*
	Description:	Do a two byte tree lookup

	Called by:		RunTranUnit
*/

static int WordTree(       /* Perform a tree lookup for a word output	*/
   TREE N_FAR *t,          /* Pointer to tree data						*/
   unicode     input,      /* Input to be looked up					*/
   char N_FAR *output)     /* Buffer for the output					*/
{

   int    subs[16],        /* Index values for each slice				*/
			 i,					/* Loop variable							*/
			 size,				/* Size of a slice							*/
          slices,          /* Number of slices							*/
			 emitted = 0;		/* How many bytes did we emit?				*/
   nuint16 N_FAR *index,      /* Pointer to the data for the tree			*/
        N_FAR *treeData,   /* Pointer to tree data						*/
			 value;				/* Value in final index						*/

	/*
	 	Copy number of slices for some speed
	*/
	slices = t->numSlices;

	/*
		Get the subscripts based on the slicing system
	*/
	for ( i = slices ; i ; i-- )
	{
		size = t->sliceSizes[i - 1];
		subs[i - 1] = input & ((1 << size) - 1);
		input >>= size;
	}

	/*
		Get a pointer to the tree data
	*/
	treeData = (nuint16 N_FAR *)(t->sliceSizes + slices);

	/*
		First index is at the top of the tree
	*/
	index = treeData;

	/*
		Do lookups till we get to the last index, or a null entry
	*/
	for ( i = 0 ; i < slices - 1 ; i++ )
		index = treeData + index[subs[i]];

	/*
		Now get the value that sits there
	*/
	value = index[subs[slices - 1]];
	if (value)
	{
		*(nuint16 N_FAR *)output = value;
		emitted = 2;
	}

	/*
		Return the results
	*/
	return (emitted);

}

/****************************************************************************/

/*
	Description:	Perform a byte table lookup

	Called by:		RunTranUnit
*/

static int ByteTable(      /* Perform a table lookup for a byte output	*/
   TABLE N_FAR *t,         /* Table data to be used					*/
   unicode      input,     /* Input to be looked up					*/
   char N_FAR  *output)    /* Buffer for the output					*/
{

   int    top,          /* Top limit of binary search				*/
			 bot,				/* Bottom limit of binary search			*/
			 mid,				/* Current location in binary search		*/
          emitted = 0;  /* How many bytes did we emit?				*/
   char N_FAR *outTable;/* Pointer to the output table				*/
   unicode N_FAR *inTable, /* Pointer to input table					*/
			 midVal;			/* Value form middle						*/

	/*
		Get pointers to the input and output tables
	*/
	inTable = (unicode N_FAR *)(t + 1);
	outTable = (char N_FAR *)(inTable + t->numEntries);

	/*
		Find the entry we are interested in
	*/
	top = 0;
	bot = t->numEntries - 1;
	while (top <= bot && emitted == FALSE)
	{
		mid = (top + bot) / 2;
		midVal = inTable[mid];
		if (midVal == input)
		{
			*output = outTable[mid];
			emitted = TRUE;
		}
		else if (midVal < input)
			top = mid + 1;
		else
			bot = mid - 1;
	}

	/*
		Return whatever we found
	*/
	return (emitted);

}

/****************************************************************************/

/*
	Description:	Perform a word table lookup

	Called by:		RunTranUnit
*/

static int WordTable(      /* Perform a table lookup for a word output	*/
   TABLE N_FAR *t,         /* Table data to be used					*/
   unicode      input,     /* Input to be looked up					*/
   char N_FAR  *output)    /* Buffer for the output					*/
{

   int    top,          /* Top limit of binary search				*/
			 bot,				/* Bottom limit of binary search			*/
			 mid,				/* Current location in binary search		*/
          emitted = 0;  /* How many bytes did we emit?				*/
   unicode N_FAR *inTable, /* Pointer to input table					*/
			 midVal;			/* Value form middle						*/
   nuint16 N_FAR *outTable;/* Pointer to output table					*/

	/*
		Get pointers to the input and output tables
	*/
	inTable = (unicode N_FAR *)(t + 1);
	outTable = (nuint16 N_FAR *)(inTable + t->numEntries);

	/*
		Find the entry we are interested in
	*/
	top = 0;
	bot = t->numEntries - 1;
	while (top <= bot && emitted == FALSE)
	{
		mid = (top + bot) / 2;
		midVal = inTable[mid];
		if (midVal == input)
		{
			*(nuint16 N_FAR *)output = outTable[mid];
			emitted = 2;
		}
		else if (midVal < input)
			top = mid + 1;
		else
			bot = mid - 1;
	}

	/*
		Return whatever we found
	*/
	return (emitted);

}


#else	/* NWNLM */
# include "uniintrn.h"
# include "prstring.h"

char *uniToLocalHandle = 0;
 
int NWUnicodeToLocal(

	void		*rule,			   /* Handle to the rule table					*/
	char		*target,		      /* Buffer for resulting unicode text		*/
	size_t	size,					/* Size of the buffer for resuling unicode*/
	unicode	*source,		      /* Local code to be converted             */
	char	 	noMap,				/* Result for non mapable characters		*/
	size_t	*len)
{
	int err, i = 0;

	rule = rule;
	noMap = noMap;

	err = UnicodeToLocal(source, size, target);
	if (err)
		return err;

	for (; *target; i++)
		target++;
	
	*len = i;
	return 0;
}
#endif /* NWNLM */


#else	/* MACINTOSH */
# include "uniintrn.h"
# include "MacUnico.h"

int NWUnicodeToLocal
(
	char	*rule,
	void	*target,
	size_t	size,
	unicode	*source,
	char	noMap,
	size_t	*len
)
{
	register int				_len = 0;
	register unsigned char		*t;
	register MacToUniChar		*native;

	if (!gMacASCIIToUnicode)
	{
		*len = 0;
		return 0;
	}

	HLock((Handle) gMacASCIIToUnicode);

	t      = (unsigned char *) target;
	native = (*gMacASCIIToUnicode)->table;
	source--;									// (loop preincrements)

	while (_len < size && *++source)
	{
		Boolean			found = FALSE;
		register int	i;

		_len++;

		for (i = 0; i < 0xff; i++)
		{
			if (*source == native[i].UniChar)		// can only happen once per
			{
				*t++  = native[i].MacChar;
				found = TRUE;
				break;
			}
		}

		if (!found)
			*t++ = noMap;
//			*t++ = *source & 0x00ff;				// keep only significant part
	}

	*t = '\0';

	HUnlock((Handle) gMacASCIIToUnicode);

	return *len = _len;
}
#endif /* MACINTOSH */

/****************************************************************************/
/****************************************************************************/

#endif /* Everything but WIN32 */

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/uniloc.c,v 1.1 1994/09/26 17:21:36 rebekah Exp $
*/
