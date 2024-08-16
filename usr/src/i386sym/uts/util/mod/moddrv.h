/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_MOD_MODDRV_H	/* wrapper symbol for kernel use */
#define _UTIL_MOD_MODDRV_H	/* subject to change without notice */

#ident	"@(#)kern-i386sym:util/mod/moddrv.h	1.3"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <io/conf.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/conf.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Private data for device drivers.
 */

struct	mod_drv_data	{
	/*
	 * Block device info.
	 */
	struct	bdevsw		drv_bdevsw;	/* bdevsw[] table image */
	int	bmajor_0;			/* the first block major number */
	int	bmajors;			/* number of block majors */
	/*
	 * Character device info.
	 */
	struct	cdevsw		drv_cdevsw;	/* cdevsw[] table image */
	int	cmajor_0;			/* the first character major number */
	int	cmajors;			/* the number of character majors */
};


struct	mod_drvintr	{
	struct	intr_info	*drv_intrinfo;	/* points to an array of structures */
	void	(*ihndler)();			/* the drivers interrupt handler */
};

struct	intr_info	{
	unsigned int	bin_no;	/* SLIC bin number */
	int		nvects;	/* number of vectors */
};

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_MOD_MODDRV_H */
