/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_IO_GSD_GSD_H	/* wrapper symbol for kernel use */
#define	_IO_GSD_GSD_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:io/gsd/gsd.h	1.2"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <io/ldterm/eucioctl.h>
#include <io/ws/mb.h>

#elif defined(_KERNEL)

#include <sys/eucioctl.h>
#include <sys/mb.h>

#endif

#ifdef _KERNEL

extern void gcl_bs(kdcnops_t *, channel_t *, termstate_t *);
extern void gcl_bht(kdcnops_t *, channel_t *, termstate_t *);
extern void gcl_ht(kdcnops_t *cnops, channel_t *chp, termstate_t *tsp);
extern void gcl_cursor(kdcnops_t *, channel_t *);
extern void gcl_escr(kdcnops_t *, channel_t *, termstate_t *);
extern void gcl_eline(kdcnops_t *, channel_t *, termstate_t *);
extern void gcl_norm(struct kdcnops *, channel_t *, termstate_t *, ushort);
extern void gcl_handler(wstation_t *, mblk_t *, termstate_t *, channel_t *);
extern void gcl_ichar(kdcnops_t *, channel_t *, termstate_t *);
extern void gcl_dchar(kdcnops_t *, channel_t *, termstate_t *);
extern void gcl_iline(kdcnops_t *, channel_t *, termstate_t *);
extern void gcl_dline(kdcnops_t *, channel_t *, termstate_t *);
extern void gcl_scrlup(struct kdcnops *, channel_t *, termstate_t *);
extern void gcl_scrldn(struct kdcnops *, channel_t *, termstate_t *);
extern void gcl_sfont(wstation_t *, channel_t *, termstate_t *);
extern void gdv_scrxfer(channel_t *, int);
extern int  gdv_mvword(channel_t *, ushort, ushort, int, char);
extern void gdv_stchar(channel_t *, ushort, wchar_t, uchar_t, int, int);
extern int  gv_cursortype(wstation_t *, channel_t *, termstate_t *);
extern int  gdclrscr(channel_t *, ushort, int);
extern int  gs_alloc(struct kdcnops *, channel_t *, termstate_t *);
extern void gs_free(struct kdcnops *, channel_t *, termstate_t *);
extern void gs_chinit(wstation_t *, channel_t *);
extern int  gs_setcursor(channel_t *, termstate_t *);
extern int  gs_unsetcursor(channel_t *, termstate_t *);
extern int  gs_seteuc(channel_t *, struct eucioc *);
extern void gs_writebitmap(channel_t *, int, unsigned char *, int, uchar_t);
extern int  gs_ansi_cntl(struct kdcnops *, channel_t *, termstate_t *, ushort);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_GSD_GSD_H */
