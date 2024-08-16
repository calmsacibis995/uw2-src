/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/conssw.c	1.6"
#ident	"$Header: $"

#include <io/conf.h>
#include <io/conssw.h>
#include <mem/hatstatic.h>
#include <mem/kmem.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/emask.h>
#include <util/engine.h>
#include <util/types.h>

extern const char consparam[];	/* Set by the configuration mechanism
				 * to the (optional) console parameter
				 * string.
				 */
/* May be using old configuration tools, so make consparam weak. */
#pragma weak consparam

/*
 * Pointer to console parameter string.
 */
char *consparamp = (char *)consparam;

/*
 * Current physical console device channel (see conssw.h).
 */
conschan_t conschan;

/*
 * Codes for the conssw functions.
 */
enum xcons_func {
	XC_OPEN, XC_CLOSE, XC_PUTC, XC_GETC, XC_SUSPEND, XC_RESUME
};

/*
 * Argument structure for xcall parameter passing.
 */
typedef struct xcons_arg {
	enum xcons_func	xc_func;
	conschan_t	*xc_chanp;
	int		xc_intval;
	volatile boolean_t xc_done;
} xcons_arg_t;

STATIC xcons_arg_t xcons_arg;
STATIC xcons_arg_t xcons_init_arg;

STATIC boolean_t console_init(conschan_t *chanp);


/*
 * STATIC void
 * do_xcons(xcons_arg_t *xcp)
 *	Handle a console operation.
 *
 * Calling/Exit State:
 *	Caller guarantees no other console activity.
 *
 *	The xcp argument points to a control block describing the
 *	console operation to be performed.  Returns with xcp->xc_done
 *	set to B_TRUE if the operation was successful.
 *
 *	WARNING: The structure pointed to by xcp, and the structures
 *		 it points to, must not be in automatic variables, as
 *		 they may be referenced from another CPU, which will
 *		 not be able to access the originating CPU's stack.
 *
 * Description:
 *	This routine determines if the console device is "bound" to
 *	another CPU, and if so, sends the operation to the remote CPU
 *	(via xcall).  The operation is handled on the remote CPU by
 *	another call to do_xcons on that CPU.
 */
STATIC void
do_xcons(xcons_arg_t *xcp)
{
	conschan_t *chanp = xcp->xc_chanp;
	conssw_t *consswp = chanp->cnc_consswp;
#ifndef UNIPROC
	int cpu;
#endif

	xcp->xc_done = (volatile)B_FALSE;

	if (!(chanp->cnc_flags & CNF_INITTED) && xcp->xc_func != XC_OPEN) {
		if (!console_init(chanp))
			return;
	}

#ifndef UNIPROC
	if ((cpu = consswp->cn_cpu) != -1 && cpu != myengnum) {
		emask_t target, resp;

		if (!upyet || engine[cpu].e_flags & E_NOWAY)
			return;

		EMASK_INIT(&target, cpu);
		xcall(&target, &resp, do_xcons, xcp);
		if (!EMASK_TEST1(&resp, cpu))
			return;
		while (!xcp->xc_done)
			;
		return;
	}
#endif /* !UNIPROC */

	switch (xcp->xc_func) {
	case XC_OPEN:
		chanp->cnc_dev = (*consswp->cn_open)(chanp->cnc_minor,
					     (chanp->cnc_flags & CNF_SYSCON),
						     chanp->cnc_params);
		break;
	case XC_CLOSE:
		(*consswp->cn_close)(chanp->cnc_minor,
				     (chanp->cnc_flags & CNF_SYSCON));
		break;
	case XC_PUTC:
		xcp->xc_intval = (*consswp->cn_putc)(chanp->cnc_minor,
						     xcp->xc_intval);
		break;
	case XC_GETC:
		xcp->xc_intval = (*consswp->cn_getc)(chanp->cnc_minor);
		break;
	case XC_SUSPEND:
		(*consswp->cn_suspend)(chanp->cnc_minor);
		break;
	case XC_RESUME:
		(*consswp->cn_resume)(chanp->cnc_minor);
		break;
	}

	xcp->xc_done = (volatile)B_TRUE;
}


/*
 * STATIC boolean_t
 * console_init(conschan_t *chanp)
 *	Initialize a console channel for I/O.
 *
 * Calling/Exit State:
 *	Caller guarantees no other console activity.
 *
 *	The chanp argument points to a console channel to be initialized.
 *	Returns B_TRUE if the initialization succeeds.
 *
 * Remarks:
 *	Since this may be called during another operation (out of do_xcons),
 *	we can't use the global xcons_arg; instead, we use xcons_init_arg,
 *	which is only used by this routine.
 */
STATIC boolean_t
console_init(conschan_t *chanp)
{
	xcons_init_arg.xc_func = XC_OPEN;
	xcons_init_arg.xc_chanp = chanp;
	do_xcons(&xcons_init_arg);

	if (!xcons_init_arg.xc_done || chanp->cnc_dev == NODEV)
		return B_FALSE;

	chanp->cnc_flags |= CNF_INITTED;
	return B_TRUE;
}


/*
 * boolean_t
 * console_openchan(conschan_t *chanp, conssw_t *consswp, minor_t minor,
 *		    const char *params, boolean_t syscon)
 *	Open a console channel.
 *
 * Calling/Exit State:
 *	Caller guarantees no other console activity.
 *
 *	The chanp argument points to a console channel to be opened.
 *	Returns B_TRUE if the open succeeds.
 *	chanp->cnc_dev will be set to the dev_t which can be used to
 *	access the device from user level, or NODEV if the open fails.
 */
boolean_t
console_openchan(conschan_t *chanp, conssw_t *consswp, minor_t minor,
		 const char *params, boolean_t syscon)
{
	ASSERT(chanp->cnc_flags == 0);

	chanp->cnc_consswp = consswp;
	chanp->cnc_minor = minor;
	chanp->cnc_params = params;
	chanp->cnc_dev = NODEV;
	if (syscon)
		chanp->cnc_flags |= CNF_SYSCON;

	return console_init(chanp);
}


/*
 * void
 * console_closechan(conschan_t *chanp)
 *	Close an open console channel.
 *
 * Calling/Exit State:
 *	None.
 */
void
console_closechan(conschan_t *chanp)
{
	xcons_arg.xc_func = XC_CLOSE;
	xcons_arg.xc_chanp = chanp;
	do_xcons(&xcons_arg);

	chanp->cnc_flags = 0;
}


/*
 * int
 * console_putc(conschan_t *chanp, int chr)
 *	Output a character to an open console channel.
 *
 * Calling/Exit State:
 *	Caller guarantees no other console activity.
 *	Returns 0 if the device was busy and the character was not sent;
 *	otherwise returns 1.
 */
int
console_putc(conschan_t *chanp, int chr)
{
	xcons_arg.xc_func = XC_PUTC;
	xcons_arg.xc_chanp = chanp;
	xcons_arg.xc_intval = chr;
	do_xcons(&xcons_arg);

	if (!xcons_arg.xc_done)
		return 1;

	return xcons_arg.xc_intval;
}


/*
 * int
 * console_getc(conschan_t *chanp)
 *	Inputs and returns one character from an open console channel.
 *
 * Calling/Exit State:
 *	Caller guarantees no other console activity.
 *	Returns the first available character; otherwise -1.
 *	Must only be called between a call to console_suspend and
 *	a call to console_resume for the same channel.
 */
int
console_getc(conschan_t *chanp)
{
	ASSERT(chanp->cnc_flags & CNF_SUSPENDED);

	xcons_arg.xc_func = XC_GETC;
	xcons_arg.xc_chanp = chanp;
	do_xcons(&xcons_arg);

	if (!xcons_arg.xc_done)
		return 4;	/* Ctrl-D (EOF) */

	return xcons_arg.xc_intval;
}


/*
 * void
 * console_suspend(conschan_t *chanp)
 *	Suspend normal console activity.
 *
 * Calling/Exit State:
 *	Caller guarantees no other console activity.
 */
void
console_suspend(conschan_t *chanp)
{
	ASSERT(!(chanp->cnc_flags & CNF_SUSPENDED));

	xcons_arg.xc_func = XC_SUSPEND;
	xcons_arg.xc_chanp = chanp;
	do_xcons(&xcons_arg);

	if (xcons_arg.xc_done)
		chanp->cnc_flags |= CNF_SUSPENDED;
}


/*
 * void
 * console_resume(conschan_t *chanp)
 *	Resume normal console activity.
 *
 * Calling/Exit State:
 *	Caller guarantees no other console activity.
 */
void
console_resume(conschan_t *chanp)
{
	ASSERT(chanp->cnc_flags & CNF_SUSPENDED);

	xcons_arg.xc_func = XC_RESUME;
	xcons_arg.xc_chanp = chanp;
	do_xcons(&xcons_arg);

	if (xcons_arg.xc_done)
		chanp->cnc_flags &= ~CNF_SUSPENDED;
}

/*
 * void
 * init_console(void)
 *	Initialize the console subsystem.
 *
 * Calling/Exit State:
 *	Called at system initialization time.
 *	Must be called before calling any console_xxx routines.
 *
 * Description:
 *	One-time initialization of both the generic console mechanism
 *	and the default system console (as used by cmn_err).
 */
void
init_console(void)
{
	struct constab *constabp;
	extern int default_bindcpu;

	for (constabp = &constab[conscnt]; constabp-- != constab;) {
		if (constabp->cn_cpu == -2)
			constabp->cn_cpu = default_bindcpu;
		constabp->cn_consswp->cn_cpu = constabp->cn_cpu;
	}

	if (!console_openchan(&conschan, consswp, consminor, consparamp,
			      B_TRUE)) {
		/* Selected console failed; try minor 0. */
		if (!console_openchan(&conschan, consswp, 0, consparamp,
				      B_TRUE))
			conschan.cnc_minor = consminor;
	}
	if (conschan.cnc_dev == NODEV) {
		/*
		 *+ The configured console device either does not exist or
		 *+ cannot be used as a console.  To correct, change the
		 *+ console device configuration or add the indicated device.
		 */
		cmn_err(CE_NOTE, "!Specified console device unavailable");
	} else if (conschan.cnc_minor != consminor) {
		/*
		 *+ The configured console device either does not exist or
		 *+ cannot be used as a console.  To correct, change the
		 *+ console device configuration or add the indicated device.
		 */
		cmn_err(CE_NOTE, "Specified console unit %d unavailable;"
				 " using %d instead.",
				 consminor, conschan.cnc_minor);
	}
}


/*
 * void *
 * consmem_alloc(size_t size, int flags)
 *	Allocate memory for a console driver
 *
 * Calling/Exit State:
 *	May be called at console init time, or later, when the system
 *	is fully up and running.  This allows the console driver's
 *	cn_open routine to allocate memory without knowing when it is
 *	being invoked.
 *
 *	If flags is CM_REQ_DMA, the allocated memory will be DMAable and
 *	physically contiguous.
 *
 *	Returns NULL if the desired memory is not available.
 *
 * Description:
 *	Calls calloc or kmem_alloc, as appropriate, depending on when
 *	it is invoked.
 */
void *
consmem_alloc(size_t size, int flags)
{
	void	*mem;

	ASSERT((flags & ~CM_REQ_DMA) == 0);

	if (hat_static_callocup) {
		/* It's early; use calloc() */
		if (flags & CM_REQ_DMA)
			mem = calloc_physio(size);
		else
			mem = calloc(size);
	} else {
		/* Now we can use kmem_alloc() */
		mem = kmem_alloc(size,
				 (flags & CM_REQ_DMA) ? KM_REQ_DMA | KM_NOSLEEP
						      : KM_NOSLEEP);
	}
	return mem;
}
