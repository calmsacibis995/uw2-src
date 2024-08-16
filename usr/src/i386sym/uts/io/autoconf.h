/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_AUTOCONF_H	/* wrapper symbol for kernel use */
#define _IO_AUTOCONF_H	/* subject to change without notice */

#ident	"@(#)kern-i386sym:io/autoconf.h	1.6"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h>

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

void	configure(void);
void	cfg_relocate(void);
void	conf_clocks(void);
void	conf_intr(void);
void	conf_ioinit(void);
void	conf_iostart(void);
void	ivec_init(uint, uint, void (*)());
int	ivec_alloc(uint);
void	ivec_free(uint, uint);
int	ivec_alloc_group(uint, uint);
void	ivec_free_group(uint, uint, uint);
void	nullint(int);
void	strayint(int);

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_AUTOCONF_H_ */
