/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/nsl/_import.h	1.4.5.2"
#ident	"$Header: $"

#ifndef NO_IMPORT
#define free		(*__free)
#define calloc 		(*__calloc)
#define perror		(*__perror)
#define strlen		(*__strlen)
#define write		(*__write)
#define ioctl		(*__ioctl)
#define getmsg		(*__getmsg)
#define putmsg		(*__putmsg)
#define getpmsg		(*__getpmsg)
#define putpmsg		(*__putpmsg)
#define errno		(*__errno)
#define memcpy		(*__memcpy)
#define fcntl		(*__fcntl)
#define sigemptyset	(*__sigemptyset)
#define sigaddset	(*__sigaddset)
#define sigprocmask	(*__sigprocmask)
#define open		(*__open)
#define close		(*__close)
#define ulimit		(*__ulimit)
#endif
