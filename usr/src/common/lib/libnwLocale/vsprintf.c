/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:vsprintf.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/vsprintf.c,v 1.1 1994/09/26 17:21:56 rebekah Exp $"
/****************************************************************************
 *                                                                          *
 * Filename:      vsprintf.c                                                  *
 *                                                                          *
 * Date Created:  Feb. 18, 1992                                             *
 *                                                                          *
 * Version:       1.00                                                      *
 *                                                                          *
 * Modifications: Replaced call to std lib vsprintf with our own enabled    *
 *                version.																	 *
 *                                                                          *
 ****************************************************************************/
/*--------------------------------------------------------------------------
     (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.

 No part of this file may be duplicated, revised, translated, localized or
 modified in any manner or compiled, linked, or uploaded or downloaded to or
 from any computer system without the prior written consent of Novell, Inc.
--------------------------------------------------------------------------*/

#define NWL_EXCLUDE_TIME 1
#define NWL_EXCLUDE_FILE 1

#include <stdarg.h>
#include <stdlib.h>
#include "ntypes.h"
#include "nwlocale.h"
#include "enable.h"

extern LCONV __lconvInfo;

/*****************************************************************************
* BEGIN_MANUAL_ENTRY ( NWvsprintf )
*
* Supported in (marked with X):
* 2.11  2.2   3.0   3.10  3.11  3.2
*  X     X     X     X     X     X
*
* SOURCE MODULE: VSPRINTF.C
*
* API NAME     : NWvsprintf
*
* DESCRIPTION  : Functions exactly like the standard C library function
*                with the added functionality of being enabled for
*                double-byte characters and also allowing for reordering
*                of the parameters as required for translation of the
*                message
*
* INCLUDE      : NWLOCALE.H
*
* SYNTAX       : int N_API NWvsprintf(char N_FAR *buffer,
*                                     char N_FAR *format,
*                                     char N_FAR *arglist);
*
* PARAMETERS   :   -> input          <-output
*
*    -> buffer
*       - the output buffer that receives the message
*
*    -> format
*       - a standard vsprintf control string with the possible
*         addition of reordering information
*
*    -> arglist
*       - a pointer to an array of a variable number of parameters typical
*         of vsprintf usage
*
* RETURN       : the number of bytes output
*
* MODIFICATIONS:
*
* END_MANUAL_ENTRY
*****************************************************************************/

/* Declare local functions                                                  */

#define MINUS_FLAG			1
#define PLUS_FLAG			2
#define SPACE_FLAG			4
#define POUND_FLAG			8
#define ZERO_FLAG			16
#define SHORT_FLAG		32
#define LONG_FLAG			64

#define	PERCENT				37
#define BACKSPACE			8
#define CARRIAGE_RETURN		13
#define LINE_FEED			10
#define SPACE				32
#define TAB					9
#define BELL				7

#define P_NONE				0
#define P_MINUS				1
#define P_PLUS				2
#define P_POUND				3

static void sprintfOutput(char outputChar,
	nuint8 N_FAR * N_FAR * string,
	int N_FAR *count);

#ifdef N_PLAT_UNIX
nuint32 N_FAR PercentFormat();
#endif
static void N_FAR FormatNumberString( char N_FAR *s, nuint32 N_FAR *len);

/****************************************************************************/

int N_API NWvsprintf(
		char N_FAR *destinationString,
		char N_FAR *controlString,
#ifdef N_PLAT_UNIX
		void *arguments)
#else
		va_list arguments)
#endif
/*
	This routine is the main body of the sprintf function.  The third
	parameter is a pointer to the arguments (if any) that followed the
	control string.  The caller is responsible for initializing this
	argument pointer using va_start.  The caller must also clean up the
	pointer using va_end when this routine returns. */

{
	char c;
	int  count = 0;
	char newFormat[600];

	ReorderPrintfParameters(&controlString, newFormat, 
								(unsigned int N_FAR *) arguments);

	while ((c = *controlString) != '\0')
	{
        controlString++;

		if (c != '%')
		{
            *destinationString++ = c;
				count++;
			continue;
		}
        else
        {
            controlString += PercentFormat((nuint8 N_FAR *)controlString, 
						&arguments, sprintfOutput, &destinationString, &count);
        }
	}
   *destinationString = '\0';
	return(count);
}

/****************************************************************************/

/* 
 * PrintDigit - is used by printf and sprintf to output a single 
 * digit number.  If the number is greater than 9, then it is 
 * assumed to be a HEX value.
 */
void N_FAR PrintDigit(
	nuint32 c,
	nuint8 N_FAR *buffer)
{
	if (c > 9)
		*buffer = (nuint8) c + (nuint8) 'A' - (nuint8) 10;  
	else
		*buffer = (nuint8) c + (nuint8) '0';
}

/****************************************************************************/

/* printn - is used by printf and sprintf to output numbers.  It uses
 * recursion to seperate the numbers down to individual digits and then
 * calls PrintDigit to output each digit.
 */
nuint32 N_FAR PrintNumber(
	nuint32 number,
	nuint32 base,
	nuint8 N_FAR *buffer)
{
	nuint32 index;
	nuint8 N_FAR *temp;

	if (number / base)
	{
		index = PrintNumber(number / base, base, buffer);
	}
	else
		index = 0;

	temp = buffer+index;
	PrintDigit(number % base, temp);
	temp++;
	*temp='\0';
	return (index + 1);
}
/****************************************************************************/

/* ProcessFieldInfo - is used to parse out the field width and precision
 * information from format strings.
 */
void N_FAR ProcessFieldInfo(
	nuint8 N_FAR * N_FAR *format,
	nuint32 N_FAR *width,
	nuint32 N_FAR *precision,
	nuint32 N_FAR *flags)
{
	nuint8 N_FAR *ptr;
	
	ptr = *format;

	/* process flags */
	*flags = 0;
	while (*ptr == '-' || *ptr == '+' || *ptr == ' ' ||
		*ptr == '#' || *ptr == '0')
	{
		if (*ptr == '-')
			*flags |= MINUS_FLAG;
		if (*ptr == '+')
			*flags |= PLUS_FLAG;
		if (*ptr == ' ')
			*flags |= SPACE_FLAG;
		if (*ptr == '#')
			*flags |= POUND_FLAG;
		if (*ptr == '0')
			*flags |= ZERO_FLAG;
		ptr++;
	}

	/* process width */
	*width = 0;
	while (*ptr >= '0' && *ptr <= '9')
	{
		*width = (*width * 10) + (*ptr - '0');
		++ptr;
	}

	/* process precision */
	*precision = 0;
	if (*ptr == '.')
	{
		++ptr;
		while (*ptr >= '0' && *ptr <= '9')
		{
			*precision = (*precision * 10) + (*ptr - '0');
			++ptr;
		}
	}

	if (*ptr == 'l' || *ptr == 'L')
		{
			*flags |= LONG_FLAG;
			++ptr;
		}
	else
		if(*ptr == 'h')
		{
			*flags |=SHORT_FLAG;
			++ptr;
		}

	*format = ptr;
}

/****************************************************************************/

/* Turn off optimization for MSC so we can build ... */

#ifdef _MSC_VER
#pragma optimize("aceglnptw", off)
#endif

/* 
 * PercentFormat - is the routine called by all printf family functions. 
 * The only difference is that a different output 
 * procedure is specified for each. This is where the percent 
 * conversion specifications and parameters are processed.
 * 
 * The number of characters in the format string consumed is returned.
 */
nuint32 N_FAR PercentFormat( 
	nuint8 N_FAR *format,                   		/* points just past the "%" */
	va_list N_FAR *parms,
	void (N_FAR *output)(char ch, nuint8 N_FAR * N_FAR * outparms, int N_FAR *count), 
	void N_FAR *outparms,
	int N_FAR *count)
{
	nuint8 N_FAR *fmt;	/* updatable pointer into the format string */
	nuint8 N_FAR *s;   	/* pointer to string characters (%s) */
	nuint8 ch;	/* character retrieved from va_args */
	nuint8 prefix;
	long    arg;   	/* argument being processes */
	nuint32 width;		/* width of field */
	nuint32 precision;	/* field precision */
	nuint32 flags;		/* see MINUS_FLAG, PLUS_FLAG, ... */
	nuint32 length;
	nuint32 count2;
	nuint8 numberBuffer[16];

	fmt = (nuint8 N_FAR *)format;

	ProcessFieldInfo(&fmt, &width, &precision, &flags);

	switch (*fmt++)
	{
		case 'd':
		case 'i':
			if (flags & LONG_FLAG)
				{
					arg = va_arg(*parms, long);
				}
			else if (flags & SHORT_FLAG)
				{
					arg = va_arg(*parms, short);
				}
			else
				{
					arg = va_arg(*parms, int);
				}
			if (arg < 0)
			{
				/* handle negatives */
				prefix = P_MINUS;
				if (width > 0)
					width--;
				arg = - arg;
			}
			else if (flags & PLUS_FLAG)
			{
				prefix = P_PLUS;
				if (width > 0)
					width--;
			}
			else 
				prefix = P_NONE;

			length = PrintNumber(arg, 10, numberBuffer);
			/* Put in thousands separators, if appropriate */
			FormatNumberString((char N_FAR *)numberBuffer, 
									(nuint32 N_FAR *) &length);
			goto StandardNumberPrintEntry;
		case 'o':
			if (flags & LONG_FLAG)
				{
					arg = va_arg(*parms, unsigned long);
				}
			else if (flags & SHORT_FLAG)
				{
					arg = va_arg(*parms, unsigned short);
				}
			else
				{
					arg = va_arg(*parms, unsigned int);
				}
			prefix = P_NONE;
			length = PrintNumber(arg, 8, numberBuffer);
			goto StandardNumberPrintEntry;
		case 'u':
			if (flags & LONG_FLAG)
				{
					arg = va_arg(*parms, unsigned long);
				}
			else if (flags & SHORT_FLAG)
				{
					arg = va_arg(*parms, unsigned short);
				}
			else
				{
					arg = va_arg(*parms, unsigned int);
				}
			prefix = P_NONE;
			length = PrintNumber(arg, 10, numberBuffer);
			FormatNumberString((char N_FAR *)numberBuffer, 
										(nuint32 N_FAR *) &length);
			goto StandardNumberPrintEntry;
		case 'x':
		case 'X':
			if (flags & LONG_FLAG)
				{
					arg = va_arg(*parms, unsigned long);
				}
			else if (flags & SHORT_FLAG)
				{
					arg = va_arg(*parms, unsigned short);
				}
			else
				{
					arg = va_arg(*parms, unsigned int);
				}
			prefix = P_NONE;
			if (flags & POUND_FLAG && arg != 0)
			{
				prefix = P_POUND;
				if (width > 1)
					width -= 2;
			}

			length = PrintNumber(arg, 16, numberBuffer);

StandardNumberPrintEntry:
			
			s = numberBuffer;

			if (flags & ZERO_FLAG)
			{
				/* zero fill */
				precision = width;
			}
			else if ((flags & MINUS_FLAG) == 0)
			{
				/* right justify */
				while (width > length && width > precision)
				{
					(*output)(' ', outparms, count);
					--width;
				}
			}

			/* handle the prefix if any */
			if (prefix != P_NONE)
			{
				switch (prefix)
				{
					case P_MINUS:
						(*output)('-', outparms, count);
						break;
					case P_PLUS:
						(*output)('+', outparms, count);
						break;
					case P_POUND:
						(*output)('0', outparms, count);
						(*output)('X', outparms, count);
						break;
				}
				prefix = P_NONE;
			}

			/* handle the precision */
			while (length < precision)
			{
				(*output)('0', outparms, count);
				--precision;
				--width;
			}

			/* print out the number */
			for (count2 = length; count2 > 0; count2--)
				(*output)(*s++, outparms, count);

			if (flags & MINUS_FLAG)
			{
				/* left justify */
				while (length < width)
				{
					/* left justify */
					(*output)(' ', outparms, count);
					--width;
				}
			}
			break;
		case 'c':
			ch = (char) va_arg(*parms, int);
			(*output)(ch, outparms, count);  
			break;
		case 'S':
#ifdef N_PLAT_UNIX
			s = va_arg(*parms, nuint8 *);
#else
			s = va_arg(*parms, char N_FAR *);
#endif
			if (s == NULL)
				break;
			length = *s++;
			goto StandardStringPrintEntry;
		case 's':
#ifdef N_PLAT_UNIX
			s = va_arg(*parms, nuint8 *);
#else
			s = va_arg(*parms, char N_FAR *);
#endif
			if (s == NULL)
				break;
#ifdef N_PLAT_UNIX
			length = NWstrlen((char *)s);
#else
			length = NWstrlen(s);
#endif
StandardStringPrintEntry:
			if (precision > 0 && length > precision)
				length = (nuint32) NWLTruncateString(s, (nint)(precision+1));
			if ((length < width) && ((flags & MINUS_FLAG) == 0))
			{
				/* right justify */
				for (count2 = width - length; count2 > 0; count2--)
					(*output)(' ', outparms, count);  
			}
			for (count2 = length; count2 > 0; count2--)
				(*output)(*s++, outparms, count);
			if ((length < width) && ((flags & MINUS_FLAG) != 0))
			{
				/* left justify */
				for (count2 = width - length; count2 > 0; count2--)
					(*output)(' ', outparms, count);
			}
			break;
		case '%':
			(*output)('%', outparms, count);
			break;
		default:
			(*output)('%', outparms, count);
			--fmt;
			break;
	}

	return ((nuint32)fmt - (nuint32)format);
}
/* Turn optimization back on now ... */

#ifdef _MSC_VER
#pragma optimize("", on)
#endif

/****************************************************************************/
static void N_FAR FormatNumberString( char N_FAR *s, nuint32 N_FAR *len)
{
	int group;
	div_t chunks;
	int charSize;
	
	/* Make sure we've been initialized */
	if (__lconvInfo.grouping[0] == 0)
	    _Llocaleconv();

	/* Get the grouping size */
	group = (int) __lconvInfo.grouping[0] - (int) '0';
	if (*len <= (nuint32) group)
		return;
			
	/* figure out number of chunks and insert separators */
	chunks = div((int) *len, group);
	if(chunks.rem > 0 && chunks.quot > 0)
		{
			s+= chunks.rem; /* point past 1st section */
			charSize = NWLInsertChar(s, __lconvInfo.thousands_sep);

			/* skip over thousands separator and add to length */
			s+= charSize;
			(*len)+= charSize;
			chunks.quot--;
		}
	else
		/* If we get here, the number is divided into equal chunks of size */
		/* grouping. Since we only put separators between groups, we need */
		/* one less separator than groups. */

		chunks.quot--;

	while (chunks.quot > 0)
		{
			s+=group;
			charSize = NWLInsertChar(s, __lconvInfo.thousands_sep);

			/* skip over thousands separator and add to length */
			s+= charSize;
			(*len)+= charSize;
			chunks.quot--;
		} /* end while */

}

/****************************************************************************/

static void N_FAR sprintfOutput(char outputChar,
	nuint8 N_FAR * N_FAR * string, int N_FAR *count)
{
	**string = outputChar;
	*string += 1;
	(*count)++;
}

/****************************************************************************/

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/vsprintf.c,v 1.1 1994/09/26 17:21:56 rebekah Exp $
*/
