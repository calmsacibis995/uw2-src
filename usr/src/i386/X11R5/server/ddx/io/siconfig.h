/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5server:ddx/io/siconfig.h	1.6"

/*
 *	Copyright (c) 1991 1992 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */


#ifndef	SICONFIG_H
#define	SICONFIG_H

#include	"../si/sidep.h"

extern	void		config_setfile( /* filename */ );
extern	char		*config_getfile();
extern	int		config_setent();
extern	void		config_endent();
extern char *GetXWINHome();
extern int xwin_verbose_level;

#define TIMESHARE	0
#define FIXED		1
#define REALTIME	2

#define	MAX_LINESIZE	256	/* maximum length of a single line in file */
#define	MAXARGS	16	/* maximum number of arguments on one line */

#ifndef	CONFIG_FILE
#define	CONFIG_FILE	"Xwinconfig"
#endif

#ifndef	COLOR_FILE
#define	COLOR_FILE	"Xwincmaps"
#endif

#define KBD_US		0
#define KBD_KANA	1
#define KBD_UK		2
#define KBD_GERMAN	3
#define KBD_FRENCH	4
#define KBD_ITALIAN	5
#define KBD_BELGIAN	6
#define KBD_EUROPEAN	7
/*
 * Make sure KBD_MAXIMUM is same as the previous max number
 */
#define KBD_MAXIMUM	7

#endif	/* SICONFIG_H */
