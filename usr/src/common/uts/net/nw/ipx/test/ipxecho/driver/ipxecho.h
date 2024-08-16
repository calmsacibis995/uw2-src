/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ipx-test:common/uts/net/nw/ipx/test/ipxecho/driver/ipxecho.h	1.1"
/*  Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.  */
/*  Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Righ
ts Reserved.    */
/*    All Rights Reserved   */

/*  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.  */
/*  The copyright notice above does not evidence any    */
/*  actual or intended publication of such source code. */

#ident  "$Id"

/*
 * Copyright 1991, 1992 Unpublished Work of Novell, Inc.
 * All Rights Reserved.
 *
 * This work is an unpublished work and contains confidential,
 * proprietary and trade secret information of Novell, Inc. Access
 * to this work is restricted to (I) Novell employees who have a
 * need to know to perform tasks within the scope of their
 * assignments and (II) entities other than Novell who have
 * entered into appropriate agreements.
 *
 * No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 */

#ifndef _NCPIPX_H
#define _NCPIPX_H

#ifdef _KERNEL_HEADERS
#include <net/nw/nwcommon.h>
#include <net/tihdr.h>
#include <net/tiuser.h>
#include <util/sysmacros.h>
#include <net/nw/ipx_app.h>
#else
#include "nwcommon.h"
#include "spx_opt.h"
#include <sys/tihdr.h>
#include <sys/tiuser.h>
#include <sys/sysmacros.h>
#include "sys/ipx_app.h"
#endif /* _KERNEL_HEADERS */


#ifdef OLD
#include <errno.h>
#include <stropts.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/stream.h>
#include <sys/sysmacros.h>
#include <sys/strlog.h>
#include <sys/ddi.h>
#include "portable.h"
#include "common.h"
#include "ipx_app.h"
#include "const.h"
#endif

#ifdef NTR_TRACING
#define	NTR_ModMask		NTRM_ipx
#define	IPXECHOID		IPXID
#endif

#endif /* _NCPIPX_H */
