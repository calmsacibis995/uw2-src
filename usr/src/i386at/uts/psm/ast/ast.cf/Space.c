/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/ast/ast.cf/Space.c	1.3"
#ident	"$Header: $"

#include <sys/types.h>
#include <sys/ebi.h>

/*
 * set this to the number of processors you wish to report to 
 * the kernel.  If set to 0, the number of processors installed
 * on the system is reported.
 */
int ast_numproc_override = 0;

int ast_cache_on = 1;

/*
 * set this to the mode you want the panel to come up in.  The
 * legal values can be found in <sys/ebi.h>.  Legal values are:
 *	PANEL_MODE_HISTOGRAM
 *          Display a graph of CPU usage
 *      PANEL_MODE_STATUS
 *          Display processor activity
 *      PANEL_MODE_OVERRIDE
 *          Panel graph shows user defined contents
 */
int ast_default_panel_mode = PANEL_MODE_STATUS;
