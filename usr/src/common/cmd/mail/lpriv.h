/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/lpriv.h	1.2.2.4"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)lpriv.h	1.4 'attmail mail(1) command'"
/*
    "lpriv.h" is used by the mail programs which use trusted functions
*/

#ifndef LPRIV_H
#define LPRIV_H

#include <dirent.h>

#ifdef __STDC__
extern int check4mld(const char *dir);
extern DIR *realmode_opendir(const char *dir);
extern FILE *realmode_fopen(const char *f, const char *mode);
#else
extern int check4mld();
extern DIR *realmode_opendir();
extern FILE *realmode_fopen();
#endif
extern const char *progname;

#endif /* LPRIV_H */
