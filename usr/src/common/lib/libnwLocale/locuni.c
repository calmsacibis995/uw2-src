/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:locuni.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/locuni.c,v 1.1 1994/09/26 17:20:53 rebekah Exp $"
/****************************************************************************
 *																			*
 * Library name:	NWLOCALE.LIB												*
 *																			*
 * Filename:		LOCUNI.C												*
 *																			*
 * Date Created:	October 18, 1991										*
 *																			*
 * Date Recreated:	July 24, 1992											*
 *																			*
 * Seriously																*
 * Optimized:		September 23, 1992										*
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
	Description:	This function performs conversions from local code pages
					to Unicode.  It uses the rule table specified by "rule"
					to perform the conversion.
					
					The "output" buffer will be filled with the converted
					data and will be terminated by a unicode null character.
					The length of "output" will be no more than "size"
					unicode characters, including the terminating null
					character.

					If no rule exists for the conversion of any character in
					the input, (highly unlikely) then the value of "noMap"
					will be placed in the output buffer.  If "noMap" is zero
					the default character from the rule table will be placed
					in the output buffer.  If "noMap" is zero and no default
					value is specified in the rule table, the function
					returns UNI_NO_DEFAULT.

	If successful:	Returns zero.  Results of conversion are stored in "out".
					Number of unicode characters in the output (including the
					ending null) is stored in "len".

	Otherwise:		Returns one of the following errors:

					UNI_HANDLE_BAD:			The rule handle doesn't point to
											a valid rule table.

					UNI_HANDLE_MISMATCH:	The rule handle points to a rule
											table that is not for local to
											unicode conversion.

					UNI_NO_DEFAULT:			A character was found that could
											not be converted, the noMap
											character was zero and no default
											rule was found in the rule table.
											In this case, "len" will contain
											the number of characters placed
											in "out" before the error occured.
*/

/****************************************************************************/

#define NWL_EXCLUDE_TIME

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

int N_API NWLocalToUnicode(

	void N_FAR *ruleBuffer,		/* Handle to the rule table					*/
	unicode N_FAR *out,			/* Buffer for resulting unicode	text		*/
	size_t size,				   /* Size of the buffer for resulting unicode	*/
	unsigned char N_FAR *in,	/* Local code to be converted				*/
	unicode noMap,				   /* Result for non mapable characters		*/
	size_t N_FAR *len)			/* Length of output							*/
{
   ruleBuffer = ruleBuffer;
   noMap = noMap;

   *len = (size_t) MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
                         (LPCSTR) in, -1, (LPWSTR) out, (int) size);

   return 0;
}

#else /* everything but WIN32 */

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
# define	TRUE	1
#endif

#ifndef FALSE
# define	FALSE	0
#endif

/****************************************************************************/

/*
	Local functions
*/
static int RunTranUnit(    /* Execute a translation unit				*/
   TRAN    N_FAR *tran,    /* Pointer to translation unit				*/
   nuint8    N_FAR *source,  /* Buffer for source data					*/
   int     N_FAR *used,    /* Bytes from source buffer that were used	*/
   unicode N_FAR *target); /* Buffer for target data					*/

static unicode ByteTree(   /* One byte tree lookup						*/
   TREE N_FAR *tree,       /* Pointer to tree data						*/
   nuint8        inByte);    /* Input byte								*/

static unicode WordTree(   /* Two byte tree lookup						*/
   TREE N_FAR *tree,       /* Pointer to tree data						*/
   nuint16        inWord);    /* Input word								*/

static unicode ByteTable(  /* One byte table lookup					*/
   TABLE N_FAR *table,     /* Table data to be used					*/
   nuint8         inByte);   /* Input byte								*/

static unicode WordTable(  /* Two byte table lookup					*/
   TABLE N_FAR *table,     /* Table data to be used					*/
   nuint16         inWord);   /* Input word								*/

/****************************************************************************/

/*
	Description:	Convert local code into Unicode

	Called by:		Applications
*/

int N_API NWLocalToUnicode(

	void N_FAR *ruleBuffer,		/* Handle to the rule table					*/
	unicode N_FAR *out,			/* Buffer for resulting unicode	text		*/
	size_t size,				   /* Size of the buffer for resulting unicode	*/
	unsigned char N_FAR *in,	/* Local code to be converted				*/
	unicode noMap,				   /* Result for non mapable characters		*/
	size_t N_FAR *len)			/* Length of output							*/

{

	int	 ccode,				/* Error code								*/
			 i,					/* Loop variable							*/
			 didTran,			/* Did the translation unit do anything?	*/
			 used;				/* Number of bytes used by translation unit	*/
	RULE	N_FAR *rule;      /* Pointer to rule table					*/
	TRAN	N_FAR *tran;      /* Pointer to a translation unit			*/

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
	else if (rule->type != RT_LOCAL_UNICODE)
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
			didTran = FALSE;
			for ( i = 0 ; (nuint16) i < rule->numUnits && !didTran ; i++ )
			{
				tran = (TRAN N_FAR *)((char N_FAR *)rule + rule->unitOffset[i]);
				didTran = RunTranUnit(tran, (nuint8 N_FAR *)in, &used, out);
				if (didTran)
				{
					/*
						Translation unit converted something
					*/
				   	out++;
				   	in += used;
					size--;
					*len += 1;
				}
			}

			/*
				If no translation unit handled the current source character
				then we need to use a default
			*/
			if (!didTran)
			{
				/*
					Advance past the offending character
				*/
				in++;

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
					*out++ = rule->defaultChar;
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

	Called by:		NWLocalToUnicode
*/
static int RunTranUnit(    /* Execute a translation unit				*/
   TRAN    N_FAR *tran,    /* Pointer to translation unit				*/
   nuint8    N_FAR *source,  /* Buffer for source data					*/
   int     N_FAR *used,    /* Bytes from source buffer that were used	*/
   unicode N_FAR *target)  /* Buffer for target data					*/
{

	nuint8	 inByte;			/* Input byte								*/
	nuint16	 inWord;			/* Input word								*/
   unicode  output;     /* Output of conversion, if any				*/

	/*
		Call the correct routine, based on translation type and input size
	*/
	if (tran->inputSize == 1)
	{
		inByte = *source;
		*used = 1;
		if (tran->type == LOOKUP_TREE)
			output = ByteTree((TREE N_FAR *)tran, inByte);
		else
			output = ByteTable((TABLE N_FAR *)tran, inByte);
	}
	else
	{
		if (tran->swap)
			inWord = (source[0] << 8) + source[1];
		else
			inWord = *(nuint16 N_FAR *)source;
		*used = 2;
		if (tran->type == LOOKUP_TREE)
			output = WordTree((TREE N_FAR *)tran, inWord);
		else
			output = WordTable((TABLE N_FAR *)tran, inWord);
	}

	/*
		If translation unit did something, save the result
	*/
	if (output)
		*target = output;

	/*
		Return the results
	*/
	return (output);

}

/****************************************************************************/

/*
	Description:	Do a one byte tree lookup

	Called by:		RunTranUnit
*/

static unicode ByteTree(   /* One byte tree lookup						*/
   TREE N_FAR *t,          /* Pointer to tree data						*/
   nuint8        input)      /* Input byte								*/
{

	int	 subs[8],			/* Index values for each slice				*/
			 i,					/* Loop variable							*/
			 size,				/* Size of a slice							*/
			 slices;          /* Number of slices							*/
   nuint16 N_FAR *index,      /* Pointer to the data for the tree			*/
        N_FAR *treeData;   /* Pointer to tree data						*/
   unicode N_FAR *data,    /* Pointer to data in last index			*/
			 outputValue = 0;	/* Output value								*/

	/*
	 	Copy number of slices for some speed
	*/
	slices = t->numSlices;

	/*
		Is this a simple index operation?
	*/
	if (slices == 1)
	{
		data = (unicode N_FAR *)(t->sliceSizes + 1);
		outputValue = data[input];
	}
	else
	{
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
		data = index + subs[slices - 1];
		outputValue = *data;
	}

	/*
		Return the results
	*/
	return (outputValue);

}

/****************************************************************************/

/*
	Description:	Do a two byte tree lookup

	Called by:		RunTranUnit
*/

static unicode WordTree(   /* Two byte tree lookup						*/
   TREE N_FAR *t,          /* Pointer to tree data						*/
   nuint16        input)      /* Input word								*/
{

	int 	 subs[16],			/* Index values for each slice				*/
			 i,					/* Loop variable							*/
			 size,				/* Size of a slice							*/
			 slices;          /* Number of slices							*/
   nuint16 N_FAR *index,      /* Pointer to the data for the tree			*/
        N_FAR *treeData;   /* Pointer to tree data						*/
   unicode outputValue = 0;/* Output value								*/

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
	outputValue = index[subs[slices - 1]];

	/*
		Return the results
	*/
	return (outputValue);

}

/****************************************************************************/

/*
	Description:	Perform a byte table lookup

	Called by:		RunTranUnit
*/

static unicode ByteTable(  /* One byte table lookup					*/
   TABLE N_FAR *t,         /* Table data to be used					*/
   nuint8         input)     /* Input byte								*/
{

	int 	 top,				/* Top limit of binary search				*/
			 bot,				/* Bottom limit of binary search			*/
			 mid;				/* Current location in binary search		*/
   nuint8 N_FAR *inTable, /* Pointer to input table					*/
			 midVal;			/* Value form middle						*/
   unicode N_FAR *outTable, /* Pointer to output table					*/
			 output = 0;	/* Output from lookup						*/

	/*
		Get pointers to the input and output tables
	*/
	inTable = (nuint8 N_FAR *)(t + 1);
	outTable = (unicode N_FAR *)(inTable + t->numEntries);

	/*
		Find the entry we are interested in
	*/
	top = 0;
	bot = t->numEntries - 1;
	while (top <= bot && output == 0)
	{
		mid = (top + bot) / 2;
		midVal = inTable[mid];
		if (midVal == input)
			output = outTable[mid];
		else if (midVal < input)
			top = mid + 1;
		else
			bot = mid - 1;
	}

	/*
		Return whatever we found
	*/
	return (output);

}

/****************************************************************************/

/*
	Description:	Perform a word table lookup

	Called by:		RunTranUnit
*/

static unicode WordTable(  /* Two byte table lookup					*/
   TABLE N_FAR *t,         /* Table data to be used					*/
   nuint16         input)     /* Input word								*/
{

	int	 top,				/* Top limit of binary search				*/
			 bot,				/* Bottom limit of binary search			*/
			 mid;				/* Current location in binary search		*/
	nuint16 N_FAR *inTable,	/* Pointer to input table					*/
			 midVal;			/* Value form middle						*/
   unicode N_FAR *outTable, /* Pointer to output table					*/
			 output = 0;	/* Output from lookup						*/

	/*
		Get pointers to the input and output tables
	*/
	inTable = (nuint16 N_FAR *)(t + 1);
	outTable = (unicode N_FAR *)(inTable + t->numEntries);

	/*
		Find the entry we are interested in
	*/
	top = 0;
	bot = t->numEntries - 1;
	while (top <= bot && output == 0)
	{
		mid = (top + bot) / 2;
		midVal = inTable[mid];
		if (midVal == input)
			output = outTable[mid];
		else if (midVal < input)
			top = mid + 1;
		else
			bot = mid - 1;
	}

	/*
		Return whatever we found
	*/
	return (output);

}


#else	/* NWNLM */
# include "uniintrn.h"
# include "prstring.h"

char *localToUniHandle = 0;
 
int NWLocalToUnicode(

	void		*rule,				   /* Handle to the rule table					*/
	unicode	*target,			      /* Buffer for resulting unicode text		*/
	size_t	size,				      /* Size of the buffer for resuling unicode*/
	char		*source,			      /* Local code to be converted             */
	unicode	noMap,				   /* Result for non mapable characters		*/
	size_t	*len)
{
	int err;

	rule = rule;
	noMap = noMap;
 
	err = LocalToUnicode(source, size, target);
	if (err)
		return err;

	*len = (unilen(target) + 1);
	return 0;
}
#endif /* NWNLM */


#else	/* MACINTOSH */
# include "uniintrn.h"
# include "MacUnico.h"

int NWLocalToUnicode
(
	char	*rule,
	unicode	*target,
	size_t	size,
	void	*source,
	unicode	noMap,
	size_t	*len
)
{
	register int			_len = 0;
	register unsigned char	*s;
	register MacToUniChar	*native;

	if (!gMacASCIIToUnicode)
	{
		*len = 0;
		return 0;
	}

	HLock((Handle) gMacASCIIToUnicode);

	s      = (unsigned char *) source;
	native = (*gMacASCIIToUnicode)->table;
	s--;										// (loop preincremented)

	while (_len++ < size && *++s)
	{
		unicode	UniChar = native[*s].UniChar;

		if (!UniChar && *s)					// nonnull translated to null, use TnoMapU
			*target++ = noMap;
		else
			*target++ = UniChar;
	}

	*target = 0x0000;						// Unicode strings are null-terminated

	HUnlock((Handle) gMacASCIIToUnicode);

	return *len = _len;
}
#endif /* MACINTOSH */

/****************************************************************************/
/****************************************************************************/
#endif /* Everything but WIN32 */

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/locuni.c,v 1.1 1994/09/26 17:20:53 rebekah Exp $
*/
