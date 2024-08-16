/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_SPECFS_DEVMAC_H	/* wrapper symbol for kernel use */
#define	_FS_SPECFS_DEVMAC_H	/* subject to change without notice */

#ident	"@(#)kern:fs/specfs/devmac.h	1.6"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h> /* REQUIRED */
#include <acc/mac/mac.h> /* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h> /* REQUIRED */
#include <sys/mac.h> /* REQUIRED */

#else

#include <sys/types.h> /* SVR4.0COMPAT */
#include <sys/mac.h> /* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

/******************************************************/
/*                                                   */
/*   Device Security Structure used by SPECFS		*/	
/*                                                   */
/*****************************************************/

/* 
 * struct devmac contains the security attributes of a device 
 */

struct devmac {
	ushort	d_relflag;	/* dev release flags */
	ushort	d_pad;		/* pad reserved for future extensions */
	lid_t	d_hilid;	/* maximum level of device */
	lid_t	d_lolid;	/* minimum level of device */
};

/*
 * Macro to get the state of a  block or character device special file
 */
#define	 STATE(sp)	 ((sp)->s_dstate) 

/*
 * Macro to get the mode of a  block or character device special file
 */

#define	 MODE(sp)	 ((sp)->s_dmode)

/*
 * Macro to get the release flag  of a block or character device special file 
 */

#define	 REL_FLAG(sp)	 (((sp)->s_dsecp == NULL) ? DEV_SYSTEM: (sp)->s_dsecp->d_relflag) 

/* 
 * Macro to get the maximum level of a device 
 */

#define	 HI_LEVEL(sp)	 (((sp)->s_dsecp == NULL) ? 0: (sp)->s_dsecp->d_hilid) 

/* 
 * Macro to get the minimum level of a device 
 */

#define	 LO_LEVEL(sp)	 (((sp)->s_dsecp == NULL) ? 0: (sp)->s_dsecp->d_lolid)

#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_SPECFS_DEVMAC_H */
