/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)stand:i386at/standalone/boot/boothdr/vxvm_boot.h	1.1"

#ifndef _BOOT_VXVM_BOOT_H
#define _BOOT_VXVM_BOOT_H

extern int	vxvm_priv_slice;
extern int vxvm_priv_delta;
int vxvm_get_diskid();

#define	NAME_LEN	14		/* bytes in a name */
#define	NAME_SZ		(NAME_LEN + 1)	/* size of fstype field */

#define VOL_ULONG_BYTES		((size_t)4)
#define VOL_USHORT_BYTES	((size_t)2)
#define VOL_SEQNO_BYTES		(2 * VOL_ULONG_BYTES)
#define VOL_VOFF_BYTES		((size_t)8)

#define	VOL_UUID_LEN	64		/* chars in a unique identifier */
#define	VOL_UUID_SZ	(VOL_UUID_LEN+1)	/* bytes in unique identifier */

#endif /* _BOOT_VXVM_BOOT_H */
