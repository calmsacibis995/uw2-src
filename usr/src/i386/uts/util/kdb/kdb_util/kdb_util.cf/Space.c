/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:util/kdb/kdb_util/kdb_util.cf/Space.c	1.7"
#ident	"$Header: $"

#include <config.h>

#include <sys/kdebugger.h>
#include <sys/conssw.h>


/*
 * dbg_init_ioswpp, dbg_init_iominorp, and dbg_init_paramp point to variables
 * containing the initial conssw pointer, minor number, and parameter string
 * to use for the debugger until (and if) a debugger command is used to switch
 * to a different device.
 *
 * To use whichever device is selected for the "console" at sysinit time,
 * set these to &consswp, &consminor, and &consparamp; i.e.:
 *
 *	struct conssw **dbg_init_ioswpp = &consswp;
 *	minor_t *dbg_init_iominorp = &consminor;
 *	char **dbg_init_paramp = &consparamp;
 *
 * To select a specific device, create appropriately initialized variables
 * and point dbg_init_ioswpp and dbg_init_iominorp to them; e.g.:
 *
 *	extern struct conssw iasyconssw;
 *	struct conssw *dbg_init_ioswp = &iasyconssw;
 *	minor_t dbg_init_iominor = 0;
 *	const char dbg_init_param[] = "<put iasy param string here>";
 *	struct conssw **dbg_init_ioswpp = &dbg_init_ioswp;
 *	minor_t *dbg_init_iominorp = &dbg_init_iominor;
 *	char **dbg_init_paramp = dbg_init_param;
 */
struct conssw **dbg_init_ioswpp = &consswp;
minor_t *dbg_init_iominorp = &consminor;
char **dbg_init_paramp = &consparamp;


/*
 * As a security feature, the kdb_security flag (set by the KDBSECURITY
 * tuneable) is provided.  If it is non-zero, the debugger should ignore
 * attempts to enter from a console key sequence.
 */
int kdb_security = KDBSECURITY;
