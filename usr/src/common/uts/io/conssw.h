/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_CONSSW_H	/* wrapper symbol for kernel use */
#define _IO_CONSSW_H	/* subject to change without notice */

#ident	"@(#)kern:io/conssw.h	1.12"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <util/types.h> /* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h> /* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Console device interfaces.  These provide low-level, non-interrupt-driven,
 * access to console I/O.  They are accessed indirectly through the console
 * switch, since the actual console device may be selected as part of the
 * configuration process.
 *
 * dev_t (*cn_open)(minor_t minor, boolean_t syscon, const char *params) --
 *	Called before any other conssw routines to initialize the device.
 *	May be called multiple times (e.g. when a kernel debugger switches
 *	to a new console).  Returns NODEV if the indicated minor device
 *	does not exist or can't be used as a console, else returns the
 *	major/minor number which can be used to access the device from
 *	user level, e.g. through /dev/systty.  The syscon argument is
 *	non-zero if the device is being used as a system console (as opposed
 *	to use by a kernel debugger); this can be used to control whether
 *	or not special keys, such as a shutdown key, are recognized.
 *	The params argument is an optional driver-specific parameter
 *	string which can be used to initialize modes or set options;
 *	if it is NULL or blank ("") the driver should use its default
 *	settings.  Drivers are free to ignore the params argument.
 *
 * void (*cn_close)(minor_t minor, boolean_t syscon) --
 *	Called when the device, opened by (*cn_open)(), is no longer in
 *	use.  The syscon argument matches that of the corresponding open.
 *	Multiple opens may be outstanding at any time, but at most one
 *	will have syscon non-zero.
 *
 * int (*cn_putc)(minor_t minor, int chr) --
 *	Outputs one character, chr, to the console device, if the device
 *	is not busy.  Returns 1 if the character was output, or 0 if it was
 *	not output due to the device being busy.
 *
 *	For serialization purposes, the driver can ensure that (*cn_putc)()
 *	is not called, by calling console_output_lock().
 *
 * int (*cn_getc)(minor_t minor) --
 *	Inputs and returns one character from the console device, if available.
 *	If no characters are available, returns -1.  Will only be called
 *	between a "cn_suspend"/"cn_resume" pair.
 *
 * void	(*cn_suspend)(minor_t minor) --
 *	Performs any work required to suspend normal console driver input
 *	activity so it can be temporarily taken over by "cn_getc".  This
 *	routine should also do whatever is necessary to make sure that
 *	subsequent output through "cn_putc" is actually visible; e.g. switch
 *	from graphics mode to text mode, make the right virtual terminal
 *	active, etc.  Will be called at plhi.
 *
 * void	(*cn_resume)(minor_t minor) --
 *	Performs any work required to resume normal console driver input
 *	activity after it was temporarily taken over by "cn_suspend".
 *	Will be called at plhi.
 *
 * All serialization for these routines is handled outside of the driver.
 * The sequence "cn_suspend", followed by zero or more "cn_getc" (and "cn_putc")
 * calls, followed by "cn_resume", will be completely serialized, and all
 * interrupts will be disabled for the entire sequence.
 *
 * Also handled outside of the driver (in the console_xxx routines) are
 * asymmetric I/O constraints.  That is, if the driver has been configured
 * through the kernel configuration tools to run only on CPU #n, the cn_xxx
 * routines will only be called on CPU #n.
 */

#if defined _KERNEL || defined _KMEMUSER

typedef struct conssw {
	/* Fields initialized by driver: */
	dev_t	(*cn_open)(minor_t minor, boolean_t syscon, const char *params);
	void	(*cn_close)(minor_t minor, boolean_t syscon);
	int	(*cn_putc)(minor_t minor, int chr);
	int	(*cn_getc)(minor_t minor);
	void	(*cn_suspend)(minor_t minor);
	void	(*cn_resume)(minor_t minor);
	/* Fields initialized by kernel (not for driver use): */
	int	cn_cpu;		/* bind to this CPU, if not -1 */
	void	*cn_filler[4];	/* reserved for future expansion */
} conssw_t;

typedef struct conschan {
	conssw_t	*cnc_consswp;
	minor_t		cnc_minor;
	const char	*cnc_params;
	uint_t		cnc_flags;	/* runtime flags, see below */
	dev_t		cnc_dev;	/* return value from cn_init */
} conschan_t;

/* cnc_flags values */
#define CNF_INITTED	(1 << 0)	/* successfully initialized */
#define CNF_SYSCON	(1 << 1)	/* system console */
#define CNF_SUSPENDED	(1 << 2)	/* suspend active */

#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

/*
 * The following externs, macros and functions are for use by the (kernel)
 * console user, and should not be used by the (driver) console provider.
 */

extern conssw_t *consswp;	/* Set by the configuration mechanism
				 * to point to the conssw structure for the
				 * selected console device.
				 */
extern minor_t consminor;	/* Set by the configuration mechanism
				 * to the minor number of the selected
				 * console device.
				 */
extern char *consparamp;	/* Pointer to console parameter string;
				 * may be overridden by bootarg_parse().
				 */
extern conschan_t conschan;	/* Open console channel used for cmn_err */

#ifdef __STDC__
extern boolean_t console_openchan(conschan_t *chanp, conssw_t *consswp,
				  minor_t minor, const char *params,
				  boolean_t syscon);
extern int console_putc(conschan_t *chanp, int chr);
extern int console_getc(conschan_t *chanp);
extern void console_suspend(conschan_t *chanp);
extern void console_resume(conschan_t *chanp);
extern void console_closechan(conschan_t *chanp);
#endif /* __STDC__ */

#define CONSOLE_PUTC(chr)	console_putc(&conschan, chr)
#define CONSOLE_GETC()		console_getc(&conschan)
#define CONSOLE_SUSPEND()	\
		((conschan.cnc_flags & CNF_SUSPENDED) ? (void)0 : \
				console_suspend(&conschan))
#define CONSOLE_RESUME()	console_resume(&conschan)

/*
 * console_output_lock() and console_output_unlock() may be called
 * by the driver, to prevent CONSOLE_PUTC from being called while
 * the driver is handling user output.
 *
 * consmem_alloc() can be used by the driver (or on behalf of the driver)
 * from its cn_open routine, to allocate any memory it might need.  This
 * should be used instead of kmem_alloc, as cn_open may be called before
 * kmem_alloc is available.  It is not legal to allocate memory from any
 * other console entry point.  Note: physmap() may also be called from
 * cn_open, but not from any other entry point.  No provision is made for
 * freeing these resources (there is no consmem_free, and cn_close is
 * not supposed to call physmap_free); the driver should cache any memory
 * or mapping it allocates and reuse it on a subsequent open (if any).
 */
#ifdef __STDC__
extern void console_output_lock(conssw_t *);
extern void console_output_unlock(conssw_t *, pl_t);
extern void *consmem_alloc(size_t size, int flags);
#else
extern void console_output_lock();
extern void console_output_unlock();
extern void *consmem_alloc();
#endif /* __STDC__ */

/* Flags for consmem_alloc: */
#define CM_REQ_DMA	(1 << 0)	/* DMAable memory required */

#define CONS_REBOOT_OK  (1 << 0)
#define CONS_PANIC_OK   (1 << 1)

extern int console_security;

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_CONSSW_H */
