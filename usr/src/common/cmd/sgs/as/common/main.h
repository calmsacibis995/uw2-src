/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nas:common/main.h	1.1"
/*
* common/main.h - common assembler interface for main
*			to implementation-dependent stuff
*/

extern const char impdep_optstr[];	/* getopt() options */
extern const char impdep_usage[];	/* usage string */
extern const char impdep_cmdname[];	/* "as" */

		/* implementation provides */
#ifdef __STDC__
void	impdep_option(int);		/* handle imp-dep option */
void	impdep_init(void);		/* initialize imp-dep stuff */
#else
void	impdep_option(), impdep_init();
#endif
