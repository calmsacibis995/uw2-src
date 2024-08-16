/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_IO_FNT_FNT_H	/* wrapper symbol for kernel use */
#define	_IO_FNT_FNT_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:io/fnt/fnt.h	1.1"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <io/ws/ws.h>
#include <io/ws/mb.h>
#include <util/types.h>

#elif defined(_KERNEL)

#include <sys/ws/ws.h>
#include <sys/mb.h>
#include <sys/types.h>

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL)

extern int fnt_init_flg; /* indicate fnt has been loaded and initialized */
extern unsigned char	*altcharsetdata[];
extern unsigned char	*codeset0data[];
extern unsigned char	*codeset1data[];
extern unsigned char	*codeset2data[];
extern unsigned char	*codeset3data[];
extern char		*codeset0registry;
extern char		*codeset1registry;
extern char		*codeset2registry;
extern char		*codeset3registry;
extern int		codeset0width;
extern int		codeset1width;
extern int		codeset2width;
extern int		codeset3width;

extern unsigned char	*fnt_getbitmap(channel_t *, wchar_t);
extern void		fnt_unlockbitmap(channel_t *, wchar_t);
extern int		fnt_setfont(channel_t *, fontioc_t *);
extern int		fnt_getfont(channel_t *, fontioc_t *);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_FNT_FNT_H */
