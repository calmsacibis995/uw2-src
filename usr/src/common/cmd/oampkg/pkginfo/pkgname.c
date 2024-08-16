/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)oampkg:common/cmd/oampkg/pkginfo/pkgname.c	1.3.7.2"
#ident  "$Header: pkgname.c 1.2 91/06/27 $"

extern int	pkgnmchk();

main(argc, argv)
int argc;
char *argv[];
{
	while(--argc > 0) {
		if(pkgnmchk(argv[argc], (char *)0, 1))
			exit(1);
	}
	exit(0);
}
