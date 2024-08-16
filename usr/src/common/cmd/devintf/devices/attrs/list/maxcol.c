/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)devintf:common/cmd/devintf/devices/attrs/list/maxcol.c	1.1.3.2"
#ident  "$Header: maxcol.c 2.0 91/07/11 $"

#include <stdio.h>

main(argc,argv)
int argc;
char **argv;
{
	int maxcol = 0;
	int linecol = 0;
 	int c;
	while ((c = getchar()) != EOF) {
	   if (c == '\n') {
	      if (linecol > maxcol)
		 maxcol = linecol;
	      linecol = 0;
	      continue;
	   }
	   linecol++;
	}
	printf("%d\n", maxcol);
}
