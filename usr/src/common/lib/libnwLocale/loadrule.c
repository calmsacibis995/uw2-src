/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:loadrule.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/loadrule.c,v 1.1 1994/09/26 17:20:50 rebekah Exp $"
/****************************************************************************
 *																			*
 * Library name:	NWLOCALE.LIB												*
 *																			*
 * Filename:		LOADRULE.C												*
 *																			*
 * Date Created:	October 17, 1991										*
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

/*
	Description:	Load a rule table from a file into memory

	Called by:		Applications that are too stupid to do Unicode
					conversions themselves

	If successful:	Puts a handle to the rule table in "rule" and returns
					zero.

	Otherwise:		Sets "rule" to NULL and returns one of the following:

					UNI_NO_SUCH_FILE:	The attempt to open the rule file
										failed because there is no such file
										or directory.

					UNI_TOO_MANY_FILES:	The attempt to open the rule file
										failed because there are already too
										many open files.

					UNI_NO_PERMISSION:	The attempt to open the rule file
										failed because access to the file was
										denied.

					UNI_OPEN_FAILED:	The attempt to open the rule file
										failed, but for a reason other than
										those listed above.

					UNI_RULES_CORRUPT:	The header of the file was too short
										or invalid, or the rule table was
										shorter than the length indicated in
										the header, or the rule table header
										was corrupt, or something else was
										screwed up.

					UNI_NO_MEMORY:		The function could not allocate the
										memory needed to load the rule table.
*/
/****************************************************************************/

#define NWL_EXCLUDE_TIME 1

#if defined N_PLAT_MSW && defined N_ARCH_32
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

int N_API NWUnloadRuleTable(

	void N_FAR *rule)
{
	rule = rule;
	return 0;
}

int N_API NWLoadRuleTable(

	char N_FAR *path,			      /* Path to the rule table					*/
	void N_FAR * N_FAR *rule)     /* Handle to rule table returned to caller */
{
	char	N_FAR *dummy;

	dummy = path;
	rule = rule;
	return 0;
}


#else
#ifndef MACINTOSH

#include "locdefs.h"

#if defined(__BORLANDC__)
# include <mem.h>
# include <alloc.h>
#else
# include <memory.h>
# include <malloc.h>
#endif

#include "ntypes.h"
#include "uniintrn.h"			/* Unicode internal definitions				*/
#include "localias.h"
#include "unicode.h"

/****************************************************************************/

/*
	Local functions
*/
static int VerifyRuleTable(		/* Verify file and load rule table			*/
	int		  handle,			   /* Handle to potential rule table file		*/
	void N_FAR * N_FAR *rule);		/* Rule table handle returned				*/

static int SanityCheckRules(	   /* Make sure the rules are somewhat sane	*/
	RULE N_FAR *rule,				   /* Pointer to rule table just read			*/
	nint16		  length);			   /* Length of rule table						*/
	
/****************************************************************************/

int N_API NWLoadRuleTable(

	char N_FAR *path,			      /* Path to the rule table					*/
	void N_FAR * N_FAR *rule)     /* Handle to rule table returned to caller */

{

   int  handle,                  /* Handle to rule table file				*/
        ccode;                   /* Error code								*/

	/*
		Assume none of this is going to work
	*/
	*rule = NULL;

   /*
      Try to open the file and see if it's a valid rules file and read it
      into memory if it is
   */
   if ((ccode = NWUniOpenReadOnly((nuint16 N_FAR *)&handle, path)) == 0)
   {
      ccode = VerifyRuleTable(handle, rule);
      NWUniClose(handle);
   }

	/*
		Return the result to the caller
	*/
	return (ccode);

}

/****************************************************************************/

/*
	Description:	Make sure the file we opened is a valid rules file
					and read it into memory if it is

	Called by:		NWLoadRuleTable
*/

static int VerifyRuleTable(

	int		  handle,			/* Handle to potential rule table file		*/
	void N_FAR * N_FAR *rule)	/* Rule table handle returned				*/

{

	int		  ccode;
	nuint16	  readLength;
	HEADER	  header;			/* Structure we expect at top of file		*/


	/*
		Assume that this rules file is bogus
	*/
	ccode = UNI_RULES_CORRUPT;

	/*
		Read the header and see if its correct
	*/
   readLength = sizeof(HEADER);

   if (NWUniRead(handle, (void N_FAR *)&header, &readLength) == 0
   && readLength == sizeof(HEADER))
	{
		/*
			Read was OK, does the header match?
		*/
		if (nwmemcmp(header.tag, HEADER_TAG, sizeof(HEADER_TAG) - 1) == 0)
		{
			/*
				Header matches.  Read the rules into memory
			*/
			*rule = nwmalloc(header.length);

			/*
				Make sure we got the memory we need
			*/
			if (*rule == NULL)
				ccode = UNI_NO_MEMORY;

			else
			{
				/*
					If we can't read the rules, it must be corrupt
				*/
            readLength = header.length;

				if (NWUniRead(handle, *rule, &readLength) == 0
            && readLength == header.length)
				{
					/*
						Is this table reasonable sane?
					*/
					ccode = SanityCheckRules(*rule, header.length);
				}
			}
		}
	}

	/*
		If we got an error after allocating memory, free it now
	*/
	if (ccode && *rule != NULL)
	{
		nwfree(*rule);
		*rule = NULL;
	}

	/*
		Return the results
	*/
	return (ccode);

}

/****************************************************************************/

/*
	Description:	Make sure the rule table has some level of sanity

	Called by:		VerifyRuleTable
*/

static int SanityCheckRules(

	RULE N_FAR *rule,	 		/* Pointer to rule table just read			*/
	nint16		 length)			/* Length of rule table						*/

{

	int	 ccode,	 		   /* Error code								*/
			 i;					/* Loop variable							*/

	/*
		Assume the rules are screwed up
	*/
	ccode = UNI_RULES_CORRUPT;

	/*
		Is there a valid tag at the start of the rule table?
		Does the length read match the size of the table?
		Is the rule table type valid?
	*/
	if ((nint16) rule->length == length &&
		rule->type >= RT_LOCAL_UNICODE &&
		rule->type <= RT_UNICODE_NORMAL &&
		nwmemcmp(rule, RTABLE_TAG, 4) == 0)
	{
		/*
			Are the offset to the translation units valid?
		*/
		for ( i = 0 ; i < (nint16) rule->numUnits ; i++ )
			if ((nint16) rule->unitOffset[i] >= length)
				break;

		/*
			If all the offsets are valid, I guess the rule table is OK
			(for now)
		*/
		if (i >= (nint16) rule->numUnits)
			ccode = 0;
	}

	/*
		Return the results
	*/
	return (ccode);

}

/****************************************************************************/

/*
	Description:	Unload a unicode conversion rule table from memory

	Called by:		Applications that are too stupid to do Unicode
					conversions themselves

	Returns:		If the rule table is successfully unloaded, returns zero.
					If the handle was invalid it returns UNI_HANDLE_BAD.
*/

int N_API NWUnloadRuleTable(

	void N_FAR *rule)			/* Handle to the rule table					*/

{

	int		 result;			/* Result of this call						*/

	/*
		If the handle is not valid, return an error
	*/
	if (nwmemcmp(rule, RTABLE_TAG, 4))
		result = UNI_HANDLE_BAD;

	/*
		Otherwise, overwrite the tag to make sure they don't use this
		handle again, then free the memory for the rule table
	*/
	else
	{
		*(nuint32 N_FAR *)rule = 0;
		nwfree(rule);
		result = 0;
	}

	return (result);

}

#else /* MACINTOSH */
# include "uniintrn.h"
# include "MacUnico.h"

char *NWLoadRuleTable
(
	char	*path
)
{
	Size			size;
	Handle			h;
	InchWorm		iW;
	register int	i, recordCount;

	if (gMacASCIIToUnicode)			// already loaded
		goto Exit;

	sMacASCIIToUnicode = (MacToUniTableHandle) GetResource(kMacToUniRSRC, kMacToUniID);

	if (!sMacASCIIToUnicode)
		goto ErrorExit;

Exit :
	return (char *) 0xffffffff;			// dummy Macintosh table value for now

ErrorExit :
	return (char *) NULL;
}

#endif
/****************************************************************************/
/****************************************************************************/

#endif /* All but WIN32 */

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/loadrule.c,v 1.1 1994/09/26 17:20:50 rebekah Exp $
*/
