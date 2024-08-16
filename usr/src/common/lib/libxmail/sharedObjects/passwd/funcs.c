/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libmail:sharedObjects/passwd/funcs.c	1.2"
#ident	"$Header: /SRCS/esmp/usr/src/nw/lib/libmail/sharedObjects/passwd/funcs.c,v 1.3 1994/03/08 17:29:13 sharriso Exp $"

#include	<stdio.h>
#include	<dlfcn.h>
#include	<sys/param.h>
#include	<ctype.h>
#include	<stdio.h>
#include	<errno.h>
#include	<pwd.h>

#define	LIBC	"/usr/lib/libc.so.1"

extern char
    *_dlerror();

static int
    CLibraryHandle = 0;

struct passwd
    *fgetpwent(FILE *fp)
	{
	static struct passwd
	    *(*func)();

	if(func != NULL)
	    {
	    }
	else if(CLibraryHandle != 0)
	    {
	    func = (struct passwd *(*)())_dlsym(CLibraryHandle, "_abi_fgetpwent");
	    }
	else if((CLibraryHandle = _dlopen(LIBC, RTLD_LAZY)) != 0)
	    {
	    func = (struct passwd *(*)())_dlsym(CLibraryHandle, "_abi_fgetpwent");
	    }
        else
	    {
	    (void) printf("%s\n", _dlerror());
	    }

	if(func != NULL)
	    {
	    return((*func)(fp));
	    }
	else
	    {
	    (void) printf("%s\n", _dlerror());
	    return(NULL);
	    }
	}

struct passwd
    *getpwent()
	{
	static struct passwd
	    *(*func)();

	if(func != NULL)
	    {
	    }
	else if(CLibraryHandle != 0)
	    {
	    func = (struct passwd *(*)())_dlsym(CLibraryHandle, "_abi_getpwent");
	    }
	else if((CLibraryHandle = _dlopen(LIBC, RTLD_LAZY)) != 0)
	    {
	    func = (struct passwd *(*)())_dlsym(CLibraryHandle, "_abi_getpwent");
	    }
        else
	    {
	    (void) printf("%s\n", _dlerror());
	    }

	if(func != NULL)
	    {
	    return((*func)());
	    }
	else
	    {
	    (void) printf("%s\n", _dlerror());
	    return(NULL);
	    }
	}

void
    endpwent()
	{
	static void
	    (*func)();

	if(func != NULL)
	    {
	    }
	else if(CLibraryHandle != 0)
	    {
	    func = (void(*)())_dlsym(CLibraryHandle, "_abi_endpwent");
	    }
	else if((CLibraryHandle = _dlopen(LIBC, RTLD_LAZY)) != 0)
	    {
	    func = (void(*)())_dlsym(CLibraryHandle, "_abi_endpwent");
	    }
        else
	    {
	    (void) printf("%s\n", _dlerror());
	    }

	if(func != NULL)
	    {
	    (*func)();
	    }
	}
