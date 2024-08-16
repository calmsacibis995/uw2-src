/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_F_DDI_H	/* wrapper symbol for kernel use */
#define _IO_F_DDI_H	/* subject to change without notice */

#ident	"@(#)kern-i386:io/f_ddi.h	1.10"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * f_ddi.h -- i386 family-specific DDI definitions.
 * # include'ed by ddi.h .
 */

/*
 * Define _DDI so that interfaces defined in other header files, but not
 * available to drivers, can be hidden.
 */
#if !defined(_DDI) && !defined(_DDI_C)
#define _DDI
#endif

#ifdef _KERNEL

/*
 * Declarations for DDI/DKI functions defined either in ddi.c or elsewhere.
 * Some of these duplicate declarations from other header files; they are
 * included here since drivers aren't supposed to # include those other files.
 */

#define NMI_ATTACH	0x01
#define NMI_DETACH	0x02
#define WATCHDOG_ALIVE_ATTACH	0x04
#define WATCHDOG_ALIVE_DETACH	0x08

/*
 * Return values from the driver NMI handler.
 */
#define NMI_UNKNOWN	0x00
#define NMI_FATAL	0x01
#define NMI_BENIGN	0x02
#define NMI_BUS_TIMEOUT	0x04
#define NMI_REBOOT	0x10

#ifdef __STDC__
extern uchar_t inb(int);
extern ushort_t inw(int);
extern ulong_t inl(int);
extern void outb(int, uchar_t);
extern void outw(int, ushort_t);
extern void outl(int, ulong_t);
extern void repinsb(int, uchar_t *, int);
extern void repinsw(int, ushort_t *, int);
extern void repinsd(int, ulong_t *, int);
extern void repoutsb(int, uchar_t *, int);
extern void repoutsw(int, ushort_t *, int);
extern void repoutsd(int, ulong_t *, int);
extern void drv_callback(int tag, int (*fcn)(), void *arg);
#else
extern uchar_t inb();
extern ushort_t inw();
extern ulong_t inl();
extern void outb();
extern void outw();
extern void outl();
extern void repinsb();
extern void repinsw();
extern void repinsd();
extern void repoutsb();
extern void repoutsw();
extern void repoutsd();
extern int drv_callback();
#endif

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_F_DDI_H */
