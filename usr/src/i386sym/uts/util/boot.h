/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_UTIL_BOOT_H	/* wrapper symbol for kernel use */
#define	_UTIL_BOOT_H	/* subject to change without notice */

#ident	"@(#)kern-i386sym:util/boot.h	1.5"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Defines for the boot/reboot process.
 */

#define	RB_AUTOBOOT	0	/* flags for system auto-booting itself */

#define	RB_ASKNAME	0x01	/* ask for file name to reboot from */
#define	RB_SINGLE	0x02	/* reboot to single user only */
#define	RB_NOSYNC	0x04	/* dont sync before reboot */
#define	RB_HALT		0x08	/* don't reboot, just halt */
#define	RB_INITNAME	0x10	/* name given for /etc/init */

#define RB_NO_CTRL	0x20	/* for FIRMWARE, don't start controller */
#define RB_NO_INIT	0x40	/* for FIRMWARE, don't init system */
#define RB_AUXBOOT	0x80	/* Boot auxiliary boot name */
#define RB_DUMP		RB_AUXBOOT
#define RB_CONFIG	0x100	/* for FIRMWARE, only build cfg table */
#define RB_NO_CACHE	0x200	/* boot with cache turned off */

#define	RB_PANIC	0	/* reboot due to panic */
#define	RB_BOOT		1	/* reboot due to boot() */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_BOOT_H_ */
