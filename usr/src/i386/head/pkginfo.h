/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamhdrs:i386/head/pkginfo.h	1.1"
#ident	"$Header: $"

#define PI_INSTALLED 	0
#define PI_PARTIAL	1
#define PI_PRESVR4	2
#define PI_UNKNOWN	3
#define PI_SPOOLED	4

#define COREPKG	"foundation"

struct pkginfo {
	char	*pkginst;
	char	*name;
	char	*arch;
	char	*version;
	char	*vendor;
	char	*basedir;
	char	*catg;
	char	status;
};

extern char	*pkgdir;

extern char	*pkgparam();
extern int	pkginfo(),
		pkgnmchk();
