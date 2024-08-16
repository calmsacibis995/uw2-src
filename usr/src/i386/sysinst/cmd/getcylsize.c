/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)proto:cmd/getcylsize.c	1.1"
#include <stdlib.h>

#define FIRSTVAR	1000.0
#define ONEMEG		1048576.0

main(int argc, char **argv)
{
	printf("%d\n", (int)((atof(argv[1])) * (atof(argv[2])) *
	  (atof(argv[3])) * FIRSTVAR / ONEMEG));
	return 0;
}
