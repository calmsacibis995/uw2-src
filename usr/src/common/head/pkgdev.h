/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamhdrs:common/head/pkgdev.h	1.2.4.2"
#ident  "$Header: pkgdev.h 1.3 91/06/21 $"

struct pkgdev {
	int	rdonly;
	int	mntflg;
	long	capacity; /* number of 512-blocks on device */
	char	*name;
	char	*dirname;
	char	*pathname;
	char	*mount;
	char	*fstyp;
	char	*cdevice;
	char	*bdevice;
	char	*norewind;
};
