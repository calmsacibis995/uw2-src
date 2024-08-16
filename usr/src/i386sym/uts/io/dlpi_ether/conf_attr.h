/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_DLPI_ETHER_CONF_ATTR_H	/* wrapper symbol for kernel use */
#define _IO_DLPI_ETHER_CONF_ATTR_H	/* subject to change without notice */

#ident	"@(#)kern-i386sym:io/dlpi_ether/conf_attr.h	1.5"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(_KERNEL) || defined(_KMEMUSER)

/* struct for config egl board */
struct	egl_conf {
	uint	tag ;
	int	ssmid ;			
	paddr_t	vme_sio_addr ;		/* vme short I/O address */
	paddr_t	vme_gl_addr ;		/* vme gather list address */
	int	lvl ;			/* vme interrupt level */
	major_t egl_major ;		/* device major number */
} ;

#endif /* _KERNEL || _KMEMUSER */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_DLPI_ETHER_CONF_ATTR_H */
