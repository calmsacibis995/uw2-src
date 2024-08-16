/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _BOOTLINK_H
#define _BOOTLINK_H

#ident	"@(#)stand:i386at/standalone/boot/boothdr/bootlink.h	1.2.1.6"
#ident  "$Header: $"

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

/*	Structure for BIOS int call requests. Register values set by
/*	the caller and modified to the value after the int call by the
/*	int service routine. */

#ifndef _UTIL_TYPES_H
#include <sys/types.h>
#endif	/* _UTIL_TYPES_H */

struct int_pb {
	ushort	intval, 	/* INT value to make int x instruction */
		ax, bx,		/* input and returned */
		cx, dx, 
		bp, es,
		si,		/* returned only */
		ds;
};

/*	functions defined in boot and sharable by the
 *	initialization programs.
 */
struct bootfuncs {
	int	(*b_printf)();
	char *	(*b_strcpy)();
	char *	(*b_strncpy)();
	char *	(*b_strcat)();
	int	(*b_strlen)();
	char *	(*b_memcpy)();
	int	(*b_memcmp)();
	unchar	(*b_getchar)();
	int	(*b_putchar)();
	int	(*b_bgets)();
	int	(*b_ischar)();
	int	(*b_doint)();
	void	(*b_goany)();
	unchar	(*b_CMOSread)();
	char *	(*b_memset)();
	int	(*b_shomem)();
	int	(*b_read)();
	void	(*b_abort)();
	unsigned int	(*b_malloc)();
	void	(*file_init)();
	void	(*file_open)();
	void	(*file_read)();
	void	(*file_close)();
	void	(*file_lseek)();
	int	(*decompress)();
	int	(*logo)();
	int	(*getfhdr)();
	int	(*checkkbd)();
};
extern	struct bootfuncs	bf;

/* defines for boot/dcmp interface */
#define FILE_OPEN	1
#define FILE_READ	2
#define FILE_LSEEK	3
#define FILE_CLOSE	4
#define FILE_STATSIZE	5
#define DCMP_INIT	6

/* defines for pcx file array ... order and values important (see images.c) */
typedef enum{ DISPLAY_LOGO = 0, DISPLAY_WORKING,CLEAR_WORKING, REMOVE_LOGO} pcx_img_t;

extern struct bootfuncs *bfp;
#endif	/* _BOOT_BOOTLINK_H */
