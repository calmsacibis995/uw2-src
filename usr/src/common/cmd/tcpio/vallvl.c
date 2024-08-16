/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)tcpio:vallvl.c	1.1.1.2"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/tcpio/vallvl.c,v 1.1 91/02/28 20:12:13 ccs Exp $"
/* Title:	vallvl.c
   Validates an level, in the form of a LID, alias, or fully qualified name.
   Called by rtcpio shell script.
*/
#include <sys/types.h>
#include <mac.h>
#include <stdlib.h>

main(argc,argv)
int argc;
char **argv;
{
  int result;
  level_t levelid;
  char *levelnamep = argv[1];
  if (isalpha(*levelnamep))
      result=lvlin(levelnamep,&levelid);
  else 
    { 
      levelid=(level_t)atol(levelnamep);
      result=lvlvalid(&levelid);
    }
exit (result);
}

	

	    
