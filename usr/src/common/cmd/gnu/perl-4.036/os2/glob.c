/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Globbing for OS/2.  Relies on the expansion done by the library
 * startup code.
 */

#define PERLGLOB
#include "director.c"

int main(int argc, char **argv)
{
  SHORT i;
  USHORT r;
  CHAR *f;

  for (i = 1; i < argc; i++)
  {
    f = IsFileSystemFAT(argv[i]) ? strlwr(argv[i]) : argv[i];
    DosWrite(1, f, strlen(f) + 1, &r);
  }
  return argc - 1;
}
