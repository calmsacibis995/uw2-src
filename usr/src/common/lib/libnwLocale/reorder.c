/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:reorder.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/reorder.c,v 1.1 1994/09/26 17:21:17 rebekah Exp $"
/****************************************************************************
 *                                                                          *
 * Filename:      reorder.c                                                 *
 *                                                                          *
 * Date Created:  May 15, 1991                                              *
 *                                                                          *
 * Version:       1.00                                                      *
 *                                                                          *
 * Programmers:   Lloyd Honomichl                                           *
 *                                                                          *
 ****************************************************************************/
/*--------------------------------------------------------------------------
     (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.

 No part of this file may be duplicated, revised, translated, localized or
 modified in any manner or compiled, linked, or uploaded or downloaded to or
 from any computer system without the prior written consent of Novell, Inc.
--------------------------------------------------------------------------*/

# include <string.h>
#include <ctype.h>

#define NWL_EXCLUDE_TIME 1
#define NWL_EXCLUDE_FILE 1

#include "ntypes.h"
#include "nwlocale.h"
#include "enable.h"

/* Parsing error conditions */
#define MISSING_PARM_NUMBER         1
#define INVALID_FORMAT_ENDING       2
#define INVALID_PARM_NUMBER         3
#define DUPLICATE_PARM_NUMBER       4

#define POINTER_SIZE       2
#define LONG_DOUBLE_SIZE   4

#if (defined WIN32 || N_PLAT_UNIX)
#define _fmemcmp memcmp
#endif

static void GetOrderVector(char N_FAR *format,
                           char N_FAR *orderVector,
                           int N_FAR *stackCount,
                           char N_FAR *newFormat);

static void CalcParameterSize(char convChar,
                              char prevChar,
                              int parmCount,
                              char N_FAR *parmSize);

static void CheckValidOrderIndices(int parmCount,
                                   char N_FAR *newOrder,
                                   int N_FAR *bypass,
                                   int N_FAR *stackCount);

static void SetOrderVector(int parmCount,
                           char N_FAR *parmSize,
                           char N_FAR *newOrder,
                           char N_FAR *orderVector);

/****************************************************************************/

#if defined(N_PLAT_UNIX)
void ReorderPrintfParameters(

   char N_FAR * N_FAR *format,	/* Format control string */
   char N_FAR *newFormat,		/* New format control string */
   unsigned int N_FAR *parms)	/* Other parameters follow */
#else
void ReorderPrintfParameters(

   char N_FAR * N_FAR *format,/* Format control string               */
   unsigned int N_FAR *parms) /* Other parameters follow             */
#endif

{
   int index;
   int stackCount;      /* Number of stack elements for parms  */
   int newLocation;     /* New location of a word on the stack */
   char orderVector[100];
#if !defined(N_PLAT_UNIX)
   static char newFormat[600];
#endif
   unsigned int N_FAR *stack;
   unsigned int temp;
   unsigned int stackValue;

   /*
     The following parameters should be set:
       orderVector = parameter count + reorder offsets
       stackCount = parameter count
       format = real format string
   */
   if (_fmemcmp(*format, "LDH!", 4) == 0)
   {
     /*
       There is a special string that allows for fast processing
       of strings.  If it starts with LDH! the number of stack
       entries and their position immediate follow before the string.
     */
     stackCount = (*format) [4];
     for (index = 0; index <= stackCount; index++)
       orderVector[index] = (*format) [4 + index];
     *format += stackCount + 5;
   }
   else
   {
     /*
       Calculate the order vector for possible reordering
       and conflate the format string
     */
     GetOrderVector(*format, (char N_FAR *)&orderVector[0],
         (int N_FAR *)&stackCount, (char N_FAR *)&newFormat[0]);
     *format = newFormat;
   }

  /*
    Get a pointer to the parameters
  */
  stack = parms;
  stack--;

  /*
    If there is reordering to be done, do it now
  */
  if (stackCount > 0)
  {
    /*
      Put the words on the stack into their new order
    */
    for (index = 1 ; index < stackCount ; index++ )
    {
      /*
        Does this word need to be moved?
      */
      if ((orderVector[index] & 0x80) == 0)
      {
        /*
          Where does this word need to go to?
        */
        newLocation = orderVector[index];
        orderVector[index] |= 0x80;
        stackValue = stack[index];

        /*
          If the word we are about to overwrite needs moving
          then move it too
        */
        while ((orderVector[newLocation] & 0x80) == 0)
        {
          temp = stack[newLocation];
          stack[newLocation] = stackValue;
          orderVector[newLocation] |= 0x80;
          newLocation = orderVector[newLocation] & 0x7F;
          stackValue = temp;
        }

        /*
          Put the stack value into its final location
        */
        stack[newLocation] = stackValue;
      }
    }
  }
}

/********************************************************************/

static void GetOrderVector(char N_FAR *format,
                           char N_FAR *orderVector,
                           int N_FAR *stackCount,
                           char N_FAR *newFormat)
{
   int parmCount = 0;
   int bypass = 0;
   int orderIndex;
   int digitCount;
   char prevChar;
   char parmSize[100];
   char newOrder[100];
   char N_FAR *newPtr;

   *stackCount = 0;
   newPtr = newFormat;

   while (*format)
   {
      /* Check for a positioning parameter %nn%... */
      bypass = 0;
      if (format[0] == '%')
      {

         /* Check for "%%" which is just a "%" request */
         if (format[1] == '%')
         {
            *newPtr++ = *format++;
         }
         else
         if (format[1])
         {

            /* Check for one or two digit position parameter */
            if (isdigit(format[1]) && format[2] == '%')
              digitCount = 1;
            else
            if (isdigit(format[1]) &&
                isdigit(format[2]) &&
                format[3] == '%')
              digitCount = 2;
            else
              digitCount = 0;

            /* '%' not followed by 1 or 2 digits & '%' */
            if (!digitCount)
            {
              bypass = MISSING_PARM_NUMBER;
            }
            else
            {
              /* setup positioning  arrays */
              format++;
              orderIndex = *format++ - '0';
              if (digitCount == 2)
                 orderIndex = (orderIndex * 10) + (*format++ - '0');

              newOrder[parmCount] = (char) orderIndex;
              *newPtr++ = *format++;

              while (NWLstrchr((char N_FAR *)"cdieEfgGnopsSuxX",
                  *format) == NULL)
                if (*format && *format != ' ' && *format != '%')
                {
                  *newPtr++ = *format++;
                }
                else
                {
                  bypass = INVALID_FORMAT_ENDING;
                  break;
                }

              if (!bypass)
              {
                format--;
                prevChar = *format++;
                CalcParameterSize(*format, prevChar, parmCount,
                   (char N_FAR *)&parmSize[0]);
                *stackCount += parmSize[parmCount];
                parmCount++;
              }
            }
         }
      }

      *newPtr++ = *format++;
   }

   *newPtr = '\0';
   if (!bypass)
   {
      CheckValidOrderIndices(parmCount, (char N_FAR *)&newOrder[0],
         (int N_FAR *)&bypass, stackCount);
      if (!bypass && *stackCount)
         SetOrderVector(parmCount, (char N_FAR *)&parmSize[0],
               (char N_FAR *)&newOrder[0], orderVector);
   }

   if (bypass)
   {
      *stackCount = 0;
      *newPtr++ = ':';
      *newPtr++ = 'E';
      *newPtr++ = 'R';
      *newPtr++ = 'R';
      *newPtr++ = (char) (bypass + '0');
      *newPtr = '\0';
   }
}

/********************************************************************/

static void CalcParameterSize(char convChar,
                              char prevChar,
                              int parmCount,
                              char N_FAR *parmSize)
{
   switch (convChar)
   {
      case 'd':
      case 'i':
      case 'o':
      case 'u':
      case 'x':
      case 'X':
         if (prevChar == 'l')
            parmSize[parmCount] = 2;
         else
            parmSize[parmCount] = 1;
         break;
      case 'e':
      case 'E':
      case 'f':
      case 'g':
      case 'G':
         if (prevChar == 'L')
            parmSize[parmCount] = LONG_DOUBLE_SIZE;
         else
            parmSize[parmCount] = 4;
         break;
      case 'p':
      case 's':
      case 'S':
         if (prevChar == 'F')
            parmSize[parmCount] = 2;
         else
            parmSize[parmCount] = POINTER_SIZE;
         break;
      case 'n':
      case 'c':
         parmSize[parmCount] = 1;
         break;
   }
}

/****************************************************************************/

static void CheckValidOrderIndices(int parmCount,
                                   char N_FAR *newOrder,
                                   int N_FAR *bypass,
                                   int N_FAR *stackCount)
{
   int i;
   int j;

   for (i = 0; i < parmCount; i++)
   {
      if (newOrder[i] < 1 || newOrder[i] > (char) parmCount)
      {
         *bypass = INVALID_PARM_NUMBER;
         return;
      }

      for (j = i + 1; j < parmCount; j++)
         if (newOrder[j] == newOrder[i])  /* Are there duplicates? */
         {
            *bypass = DUPLICATE_PARM_NUMBER;
            return;
         }
   }

   for (i = 0; i < parmCount; i++)
      if (newOrder[i] != (char) (i + 1))
         return;

   *stackCount = 0;  /* parameters are already in order */
}

/****************************************************************************/

static void SetOrderVector(int parmCount,
                           char N_FAR *parmSize,
                           char N_FAR *newOrder,
                           char N_FAR *orderVector)
{
   int i;
   int parm;
   int oldOffset;
   int newOffset = 1;

   for (parm = 0; parm < parmCount; parm++)
   {
      oldOffset = 1;

      for (i = 0; i < parmCount; i++)
         if (newOrder[parm] > newOrder[i])
            oldOffset += parmSize[i];

      for (i = 0; i < (int)parmSize[parm]; i++, oldOffset++, newOffset++)
         orderVector[oldOffset] = (char) newOffset;
   }
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/reorder.c,v 1.1 1994/09/26 17:21:17 rebekah Exp $
*/
