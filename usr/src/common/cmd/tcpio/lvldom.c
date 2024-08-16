/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)tcpio:lvldom.c	1.1.1.2"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/tcpio/lvldom.c,v 1.1 91/02/28 20:11:55 ccs Exp $"
/* Title:	lvldom.c
   Compares 2 levels.  If the first level dominates the second, returns 0
   Called by rtcpio shell script.
*/
#include <sys/types.h>
#include <mac.h>
#include <stdlib.h>

main(argc,argv)
int argc;
char **argv;
{
  level_t lidarray[2];
  int i;
  for (i=0;i<2;i++)
    {
    if (isalpha(*(argv[i+1]))) 
        lvlin(argv[i+1],&lidarray[i]);
    else 
        lidarray[i]=atol(argv[i+1]);
   }
   if (lvldom(&lidarray[0],&lidarray[1]) > 0)
	exit (0);
   else exit (1);
}

	

	    
