/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/getlogin.c	1.20"
/*LINTLIBRARY*/
#ifdef __STDC__
	#pragma weak getlogin_r = _getlogin_r
	#pragma weak getlogin = _getlogin
#endif
#include "synonyms.h"
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include "utmp.h"
#include <fcntl.h>
#include <unistd.h>

char *
getlogin_r(char *buf, size_t len)
{
	register uf;
	pid_t sid;
	struct utmp ubuf;

	if(buf == 0)
		return NULL;

	if((uf = open((const char *)UTMP_FILE, 0)) < 0)
		return(NULL);

	if((sid = getsid(0)) < 0)
		return(NULL);

	while(read(uf, (char*)&ubuf, sizeof(ubuf)) == sizeof(ubuf)) {
		if(    (ubuf.ut_type == INIT_PROCESS ||
			ubuf.ut_type == LOGIN_PROCESS ||
			ubuf.ut_type == USER_PROCESS ) &&
			ubuf.ut_pid == sid) {
			(void) close(uf);
			goto found;
		}
	}
	(void) close(uf);
	return (NULL);

found:
	if(ubuf.ut_user[0] == '\0')
		return(NULL);
	if (sizeof(ubuf.ut_user) > len) {
		errno = ERANGE;
		return NULL;
	}
	strncpy(buf,&ubuf.ut_user[0],sizeof(ubuf.ut_user)) ;
	buf[sizeof(ubuf.ut_user)] = '\0' ;
	return(buf);
}

char *
getlogin()
{
	struct utmp ubuf;
	static char answer[sizeof(ubuf.ut_user)+1];

	return(getlogin_r(answer, sizeof(ubuf.ut_user)+1));
}
