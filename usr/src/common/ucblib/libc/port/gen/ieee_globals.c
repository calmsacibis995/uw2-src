/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)ucb:common/ucblib/libc/port/gen/ieee_globals.c	1.2"
#ident	"$Header: $"

/*******************************************************************

		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 

/*
 * contains definitions for variables for IEEE floating-point arithmetic
 * modes; IEEE floating-point arithmetic exception handling; 
 */

#include <fp.h>

enum fp_direction_type fp_direction;
/*
 * Current rounding direction. Updated by ieee_flags. 
 */

enum fp_precision_type fp_precision;
/*
 * Current rounding precision. Updated by ieee_flags. 
 */

sigfpe_handler_type ieee_handlers[N_IEEE_EXCEPTION];
/*
 * Array of pointers to functions to handle SIGFPE's corresponding to IEEE
 * fp_exceptions. sigfpe_default means do not generate SIGFPE. An invalid
 * address such as sigfpe_abort will cause abort on that SIGFPE. Updated by
 * ieee_handler. 
 */
fp_exception_field_type fp_accrued_exceptions;
/*
 * Sticky accumulated exceptions, updated by ieee_flags. In hardware
 * implementations this variable is not automatically updated as the hardware
 * changes and should therefore not be relied on directly. 
 */
