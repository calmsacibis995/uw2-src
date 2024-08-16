/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:xlate.c	1.1"
/****************************************************************************
 *																			*
 * Library name:	NWLOCALE.LIB												*
 *																			*
 * Filename:		XLATE.C													*
 *																			*
 * Date Created:	October 18, 1991										*
 *																			*
 * Date Recreated:	July 24, 1992											*
 *																			*
 * Seriously																*
 * Optimized:		September 28, 1992										*
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
	Description:	These functions perform various conversions involving
					unicode data.  They use the rule table specified by
					"rule" to perform the conversion.
					
					The "output" buffer will be filled with the converted
					data and will be null terminated with a sixteen bit null.
					The length of "output" will be no more than "size"
					characters, including the terminating null character.

					For the functions:
						NWUnicodeToMonocase
						NWNormalizeUnicode

					If no rule exists for the conversion of any character in
					the input, then the original character is copied into the
					output buffer.

					For the function:
						NWUnicodeToCollation

					If no rule exists for the conversion of any character in
					the input, then the value of "noMap" will be placed in
					the output buffer.  If "noMap" is zero the default
					character from the rule table will be placed in the
					output buffer.  If "noMap" is zero and no default value
					is specified in the rule table, the function returns
					UNI_NO_DEFAULT.

	If successful:	Returns zero.  Results of conversion are stored in "out".
					Number of characters in the output is stored in "len".

	Otherwise:		Returns one of the following errors:

					UNI_HANDLE_BAD:			The rule table passed is not
											valid.

					UNI_HANDLE_MISMATCH:	The rule table type does not
											match the requested operation.

					UNI_NO_DEFAULT:			A character was found that could
											not be converted, the noMap
											character was zero and no default
											rule was found in the rule table.
											In this case, "len" will contain
											the number of characters placed
											in "out" before the error occured.
*/

/****************************************************************************/

#if (defined N_PLAT_WNT3 || defined N_PLAT_WNT4) && defined N_ARCH_32
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

int N_API NWUnicodeToMonocase(

	void N_FAR *ruleBuffer,		   /* Rule table handle						*/
	unicode N_FAR *out,				/* Buffer for resulting monocased Unicode	*/
	size_t size,				      /* Size of buffer for resulting unicode	*/
	unicode N_FAR *in,				/* Buffer with source Unicode				*/
	size_t N_FAR *len)				/* Length of output							*/
{
   ruleBuffer = ruleBuffer;

   return LCMapStringW(GetThreadLocale(), LCMAP_UPPERCASE,
                       (LPCWSTR) in, (int) size, (LPWSTR) out, (int) size);
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

/*
	Local functions
*/
static int XLate(             /* Universal translator						*/
   RULE N_FAR *rule,          /* Pointer to rule table					*/
   unicode N_FAR *out,        /* Buffer for output						*/
   size_t size,               /* Size out output buffer					*/
   unicode N_FAR *in,         /* Buffer for input							*/
   int defaultToSelf,         /* Should characters default to themselves	*/
   unicode noMap,             /* No map character							*/
   size_t N_FAR *len);        /* Length of converted data in bytes		*/

static int RunTranUnit(       /* Execute a translation unit				*/
   TRAN N_FAR *tran,          /* Pointer to translation unit				*/
   unicode N_FAR *source,     /* Buffer for source data					*/
   unicode N_FAR *target,     /* Buffer for target data					*/
   int N_FAR *used);          /* Bytes from source buffer that were used	*/

static int TreeLookup11(      /* Perform a tree lookup.  1 input 1 output	*/
   TREE N_FAR *tree,          /* Pointer to tree data						*/
   unicode input,             /* Input to be looked up					*/
   unicode N_FAR *out);       /* Where to put results						*/

static int TreeLookup12(      /* Perform a tree lookup.  1 input 2 output	*/
   TREE N_FAR *tree,          /* Pointer to tree data						*/
   unicode input,             /* Input to be looked up					*/
   unicode N_FAR *out);       /* Where to put results						*/

static int TableLookup11(     /* Perform a table lookup.  1 input 1 output */
   TABLE N_FAR *table,        /* Table data to be used					*/
   unicode input,             /* Input to lookup							*/
   unicode N_FAR *out);       /* Where to put results						*/

static int TableLookup12(     /* Perform a table lookup.  1 input 2 output*/
   TABLE N_FAR *table,        /* Table data to be used					*/
   unicode input,             /* Input to lookup							*/
   unicode N_FAR *out);       /* Where to put results						*/

static int TableLookup21(     /* Perform a table lookup.  2 input 1 output*/
   TABLE N_FAR *table,        /* Table data to be used					*/
   nuint32 input,                /* Input to lookup							*/
   unicode N_FAR *out);       /* Where to put results						*/

/****************************************************************************/

/*
	Description:	Convert Unicode into Monocase

	Called by:		Applications
*/

int N_API NWUnicodeToMonocase(

	void N_FAR *ruleBuffer,		   /* Rule table handle						*/
	unicode N_FAR *out,				/* Buffer for resulting monocased Unicode	*/
	size_t size,				      /* Size of buffer for resulting unicode	*/
	unicode N_FAR *in,				/* Buffer with source Unicode				*/
	size_t N_FAR *len)				/* Length of output							*/

{

   int         ccode;         /* Error code								*/
   RULE N_FAR *rule;          /* Pointer to rule table					*/

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
	else if (rule->type != RT_UNICODE_MONO)
		ccode = UNI_HANDLE_MISMATCH;

	/*
		Rule handle is OK.  Patch in the Universal Translator, Mr. Spock!
	*/
	else
		ccode = XLate(rule, out, size, in, N_TRUE, 0, len);

	/*
		Return the result
	*/
	return (ccode);

}

/****************************************************************************/

/*
	Description:	Convert Unicode into collation weights

	Called by:		Applications that are too ignorant to do it themselves
*/

int N_API NWUnicodeToCollation(

	void N_FAR *ruleBuffer,		   /* Rule table handle						*/
	unicode N_FAR *out,				/* Buffer for resulting Unicode	weights		*/
	size_t size,				      /* Size of buffer for resulting unicode		*/
	unicode N_FAR *in,				/* Buffer with source Unicode				*/
	unicode noMap,				      /* No map character							*/
	size_t N_FAR *len)				/* Length of output							*/

{

   int         ccode;         /* Error code								*/
   RULE N_FAR *rule;          /* Pointer to rule table					*/

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
	else if (rule->type != RT_UNICODE_COLL)
		ccode = UNI_HANDLE_MISMATCH;

	/*
		Rule handle is OK.  Patch in the Universal Translator, Mr. Spock!
	*/
	else
		ccode = XLate(rule, out, size, in, N_FALSE, noMap, len);

	/*
		Return the result
	*/
	return (ccode);

}

#if defined(EXTRA)
/****************************************************************************/

/*
	Description:	Convert Unicode into Normalized Unicode

	Called by:		Applications that are too ignorant to do it themselves
*/

int NWNormalizeUnicode(

	void	*ruleBuffer,		/* Rule table handle						*/
	unicode	*out,				/* Buffer for resulting normalized unicode	*/
	size_t	 size,				/* Size of buffer for resulting unicode		*/
	unicode	*in,				/* Buffer with source Unicode				*/
	size_t	*len)				/* Length of output							*/

{

	int		 ccode;				/* Error code								*/
	RULE	*rule;				/* Pointer to rule table					*/

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
	if (memcmp(rule->tag, RTABLE_TAG, 4))
		ccode = UNI_HANDLE_BAD;

	/*
		Make sure it matches the operation
	*/
	else if (rule->type != RT_UNICODE_NORMAL)
		ccode = UNI_HANDLE_MISMATCH;

	/*
		Rule handle is OK.  Patch in the Universal Translator, Mr. Spock!
	*/
	else
		ccode = XLate(rule, out, size, in, N_TRUE, 0, len);

	/*
		Return the result
	*/
	return (ccode);

}
#endif

/****************************************************************************/

/*
	Description:	Universal translator

	Called by:		NWUnicodeToMonocase
					NWUnicodeToCollation
					NWNormalizeUnicode
*/

static int XLate(

   RULE N_FAR *rule,          /* Pointer to rule table					*/
   unicode N_FAR *out,        /* Buffer for output						*/
   size_t size,               /* Size out output buffer					*/
   unicode N_FAR *in,         /* Buffer for input							*/
   int defaultToSelf,         /* Should characters default to themselves	*/
   unicode noMap,             /* No map character							*/
   size_t N_FAR *len)         /* Length of converted data in bytes		*/

{

   int    i,               /* Loop variable							*/
			 used,				/* Number of bytes used by translation unit	*/
			 emitted,			/* Number of bytes emitted by tran unit		*/
			 ccode = 0;			/* Error code								*/
   TRAN N_FAR *tran;       /* Pointer to a translation unit			*/

	/*
		Nothing converted so far
	*/
	*len = 0;

	/*
		Convert until we reach the end of the source string, or the target
		buffer is full, of we get an error
	*/
	while (*in && size > 1 && ccode == 0)
	{
		/*
			See if any of the translation units will handle the next
			chunk of input
		*/
		emitted = 0;
		for ( i = 0 ; (nuint16) i < rule->numUnits && emitted == 0 ; i++ )
		{
			tran = (TRAN N_FAR *)((char N_FAR *)rule + rule->unitOffset[i]);
			emitted = RunTranUnit(tran, in, out, &used);
			if (emitted)
			{
				/*
					If the translation unit emitted two characters and
					there was only room for one, erase the emitted characters
					and quit

					Otherwise, just record how many characters were emitted
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
					in += used;
				}
			}
		}

		/*
			If no translation unit handled the current source character
			then we need to use a default
		*/
		if (emitted == 0)
		{
			if (defaultToSelf)
			{
				*out++ = *in++;
				*len += 1;
				size--;
			}
			else if (noMap)
			{
				*out++ = noMap;
				in++;
				*len += 1;
				size--;
			}
			else if (rule->defaultChar)
			{
				*out++ = rule->defaultChar;
				in++;
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

	/*
		Return the results
	*/
	return (ccode);

}

/****************************************************************************/

/*
	Description:	Run a translation unit

	Called by:		XLate
*/

static int RunTranUnit(

   TRAN N_FAR *tran,          /* Pointer to translation unit				*/
   unicode N_FAR *in,         /* Buffer for source data					*/
   unicode N_FAR *out,        /* Buffer for target data					*/
   int N_FAR *used)           /* Bytes from source buffer that were used	*/

{

	int		 emitted;			/* Number of characters emitted				*/
   nuint8 N_FAR *swapPtr,       /* Used for swapping						*/
               swapVal;       /* Used for swapping						*/
	unicode	 inUnicode;			/* Input to lookup							*/
   nuint32 inLong;               /* Input to lookup							*/

	/*
		Decide what sort of translator to call
	*/
	if (tran->inputSize == 2)
	{
		inUnicode = *in;
		*used = 1;
		if (tran->swap)
		{
			swapPtr = (nuint8 N_FAR *)&inUnicode;
			swapVal = swapPtr[0];
			swapPtr[0] = swapPtr[1];
			swapPtr[1] = swapVal;
		}

		if (tran->outputSize == 2)
		{
			if (tran->type == LOOKUP_TREE)
				emitted = TreeLookup11((TREE N_FAR *)tran, inUnicode, out);
			else
				emitted = TableLookup11((TABLE N_FAR *)tran, inUnicode, out);
		}
		else
		{
			if (tran->type == LOOKUP_TREE)
				emitted = TreeLookup12((TREE N_FAR *)tran, inUnicode, out);
			else
				emitted = TableLookup12((TABLE N_FAR *)tran, inUnicode, out);
		}
	}
	else
	{
		inLong = *(nuint32 N_FAR *)in;
		*used = 2;
		if (tran->swap)
		{
			swapPtr = (nuint8 N_FAR *)&inLong;

			swapVal = swapPtr[0];
			swapPtr[0] = swapPtr[3];
			swapPtr[3] = swapVal;

			swapVal = swapPtr[1];
			swapPtr[1] = swapPtr[2];
			swapPtr[2] = swapVal;
		}

		/*
			Tree lookups are pretty useless for 32 bit input
		*/
		emitted = TableLookup21((TABLE N_FAR *)tran, inLong, out);
	}

	/*
		Return the results
	*/
	return (emitted);

}

/****************************************************************************/

/*
	Description:	Do a tree lookup

	Called by:		RunTranUnit
*/

static int TreeLookup11(

   TREE N_FAR *t,             /* Pointer to tree data						*/
   unicode input,             /* Input to be looked up					*/
   unicode N_FAR *out)        /* Where to put results						*/

{

   int    slices,          /* Number of slices							*/
			 size,				/* Size of a slice							*/
			 subs[16],			/* Index values for each slice				*/
			 i,					/* Loop variable							*/
			 emitted = 0;		/* Characters emitted to output buffer		*/
	unicode	 outVal;			/* Output value from last index				*/
   nuint16 N_FAR *index,      /* Pointer to the data for the tree			*/
        N_FAR *treeData;   /* Pointer to tree data						*/

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
	treeData = (nuint16 N_FAR *)(t->sliceSizes + t->numSlices);

	/*
		First index is at the top of the tree
	*/
	index = treeData;

	/*
		Do lookups till we get to the last index, or a null entry
	*/
	for ( i = 0 ; i < t->numSlices - 1 ; i++ )
		index = treeData + index[subs[i]];

	/*
		Now get the value that sits there
	*/
	outVal = (unicode) index[subs[t->numSlices - 1]];
	if (outVal)
	{
		*out++ = outVal;
		emitted = 1;
	}

	/*
		Return the results
	*/
	return (emitted);

}

/****************************************************************************/

/*
	Description:	Do a tree lookup given one unicode character to find
					two unicode output characters

	Called by:		RunTranUnit
*/

static int TreeLookup12(

   TREE N_FAR *t,             /* Pointer to tree data						*/
   unicode input,             /* Input to be looked up					*/
   unicode N_FAR *out)        /* Where to put results						*/

{

   int    slices,          /* Number of slices							*/
			 size,				/* Size of a slice							*/
			 subs[16],			/* Index values for each slice				*/
			 i,					/* Loop variable							*/
			 emitted = 0;		/* Characters emitted to output buffer		*/
   nuint16 N_FAR *index,      /* Pointer to the data for the tree			*/
        N_FAR *treeData;   /* Pointer to tree data						*/

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
	treeData = (nuint16 N_FAR *)(t->sliceSizes + t->numSlices);

	/*
		First index is at the top of the tree
	*/
	index = treeData;

	/*
		Do lookups till we get to the last index, or a null entry
	*/
	for ( i = 0 ; i < t->numSlices - 1 ; i++ )
		index = treeData + index[subs[i]];

	/*
		Now get the value that sits there
	*/
	index += subs[t->numSlices - 1] * 2;
	if (index[0] || index[1])
	{
		out[0] = index[0];
		out[1] = index[1];
		emitted = 2;
	}

	/*
		Return the results
	*/
	return (emitted);

}

/****************************************************************************/

/*
	Description:	Perform a table lookup

	Called by:		RunTranUnit
*/

static int TableLookup11(

   TABLE N_FAR *t,            /* Pointer to tree data						*/
   unicode input,             /* Input to be looked up					*/
   unicode N_FAR *out)        /* Where to put results						*/

{

   int    top,             /* Top limit of binary search				*/
          bot,             /* Bottom limit of binary search			*/
          mid,             /* Current location in binary search		*/
			 emitted = 0;		/* Characters emitted to output buffer		*/
	unicode	 midVal,			/* Value form middle						*/
         N_FAR *inTable,   /* Pointer to input table					*/
         N_FAR *outTable;  /* Pointer to output table					*/

	/*
		Get pointers to the input and output tables
	*/
	inTable = (unicode N_FAR *)(t + 1);
	outTable = (unicode N_FAR *)(inTable + t->numEntries);

	/*
		Find the entry we are interested in
	*/
	top = 0;
	bot = t->numEntries - 1;
	while (top <= bot && emitted == 0)
	{
		mid = (top + bot) / 2;
		midVal = inTable[mid];
		if (midVal == input)
		{
			*out = outTable[mid];
			emitted = 1;
		}
		else if (midVal < input)
			top = mid + 1;
		else
			bot = mid - 1;
	}

	/*
		Let caller know what we did
	*/
	return (emitted);

}

/****************************************************************************/

/*
	Description:	Perform a table lookup

	Called by:		RunTranUnit
*/

static int TableLookup12(

   TABLE N_FAR *t,            /* Table data to be used					*/
   unicode input,             /* Input to lookup							*/
   unicode N_FAR *out)        /* Where to put results						*/

{

   int    top,             /* Top limit of binary search				*/
          bot,             /* Bottom limit of binary search			*/
          mid,             /* Current location in binary search		*/
			 emitted = 0;		/* Characters emitted to output buffer		*/
	unicode	 midVal,			/* Value form middle						*/
         N_FAR *inTable,   /* Pointer to input table					*/
         N_FAR *outTable;  /* Pointer to output table					*/

	/*
		Get pointers to the input and output tables
	*/
	inTable = (unicode N_FAR *)(t + 1);
	outTable = (unicode N_FAR *)(inTable + t->numEntries);

	/*
		Find the entry we are interested in
	*/
	top = 0;
	bot = t->numEntries - 1;
	while (top <= bot && emitted == 0)
	{
		mid = (top + bot) / 2;
		midVal = inTable[mid];
		if (midVal == input)
		{
			out[0] = outTable[mid * 2];
			out[1] = outTable[mid * 2 + 1];
			emitted = 2;
		}
		else if (midVal < input)
			top = mid + 1;
		else
			bot = mid - 1;
	}

	/*
		Let caller know what we did
	*/
	return (emitted);

}

/****************************************************************************/

/*
	Description:	Perform a table lookup

	Called by:		RunTranUnit
*/

static int TableLookup21(

   TABLE N_FAR *t,            /* Table data to be used					*/
   nuint32 input,                /* Input to lookup							*/
   unicode N_FAR *out)        /* Where to put results						*/

{

   int    top,             /* Top limit of binary search				*/
          bot,             /* Bottom limit of binary search			*/
          mid,             /* Current location in binary search		*/
			 emitted = 0;		/* Characters emitted to output buffer		*/
   nuint32   midVal,          /* Value form middle						*/
          N_FAR *inTable;  /* Pointer to input table					*/
   unicode N_FAR *outTable;/* Pointer to output table					*/

	/*
		Get pointers to the input and output tables
	*/
	inTable = (nuint32 N_FAR *)(t + 1);
	outTable = (unicode N_FAR *)(inTable + t->numEntries);

	/*
		Find the entry we are interested in
	*/
	top = 0;
	bot = t->numEntries - 1;
	while (top <= bot && emitted == 0)
	{
		mid = (top + bot) / 2;
		midVal = inTable[mid];
		if (midVal == input)
		{
			*out = outTable[mid];
			emitted = 1;
		}
		else if (midVal < input)
			top = mid + 1;
		else
			bot = mid - 1;
	}

	/*
		Let caller know what we did
	*/
	return (emitted);

}


#else	/* NWNLM */
# include "uniintrn.h"
# include "prstring.h"

char *compareHandle = 0;
char *monoHandle = 0;

int NWUnicodeToMonocase(	/* Convert Unicode to collation              */

	char		*rule,		   /* Rule table handle                         */
	unicode	*target,	      /* Buffer for resulting Unicode	weights		*/
	size_t	size,				/* Size of buffer for resulting unicode		*/
	unicode	*source,	      /* Buffer with source Unicode                */
	size_t	*len)
{

	size_t	 i = 0;		/* Result of conversion.  Assume failure	*/

	rule = rule;

	for (i = 0; size && *source; size--, target++, source++, i++) 
	{
		(*target) =  MonoCase((*source));
	}

	*target = 0;
	*len = i;
	return 0;
}
#endif /* NWNLM */


#else	/* MACINTOSH */
# include "uniintrn.h"
# include "MacUnico.h"

int NWUnicodeToMonocase
(
	char	*rule,
	unicode	*target,
	size_t	size,
	unicode	*source,
	unicode	noMap,
	size_t	*len
)
{
	unicpy(target, source);
	return *len = unilen(target);
}

int NWUnicodeToCollation
(
	char	*rule,
	unicode	*target,
	size_t	size,
	unicode	*source,
	unicode	noMap,
	size_t	*len
)
{
	unicpy(target, source);
	return *len = unilen(target);
}
#endif /* MACINTOSH */

/****************************************************************************/
/****************************************************************************/

#endif /* All but WIN32 */

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/xlate.c,v 1.1 1994/09/26 17:22:01 rebekah Exp $
*/
