/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/astm/astm.cf/Space.c	1.1"
#ident	"$Header: $"
/*
 * Configuration file for "astm" driver:
 * AST Manhattan front panel support
 */
#include <sys/ebi.h>

/*
 * Default: Rate for alpha display utilization updates 
 * (and histogram updates in UP version).
 *		UTIL_SLOW_RATE   -- every 100 ticks (1000 msec)
 *		UTIL_MEDIUM_RATE -- every 50 ticks (500 msec)
 *		UTIL_FAST_RATE	 -- every 25 ticks (250 msec)
 */
int		astm_util_rate = 	UTIL_MEDIUM_RATE;

/*
 * Default: utilization display in alpha window
 *		ALPHA_MODE_TEXT -- off
 *		ALPHA_MODE_UTIL -- on
 */
int		astm_alpha_util_mode = ALPHA_MODE_UTIL;

/*
 * Default: bar graph LED display mode
 * 	PANEL_MODE_HISTOGRAM	-- bar graph of CPU utilization
 * 	PANEL_MODE_STATUS		-- LED on when CPU busy (MP kernel only)
 * 	PANEL_MODE_OVERRIDE		-- manual mode
 * 	PANEL_MODE_ONLINE		-- LED on when CPU is present
 */
#ifdef UNIPROC
int 	astm_graph_mode = PANEL_MODE_ONLINE;
#else
int 	astm_graph_mode = PANEL_MODE_ONLINE;
#endif
