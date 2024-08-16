/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* $RCSfile: util.h,v $$Revision: 1.1.1.1 $$Date: 1993/10/11 20:27:12 $
 *
 *    Copyright (c) 1991, Larry Wall
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 * $Log: util.h,v $
 * Revision 1.1.1.1  1993/10/11  20:27:12  ram
 * NUC code from 1.1d release
 *
 * Revision 4.0.1.2  91/11/05  19:21:20  lwall
 * patch11: various portability fixes
 * 
 * Revision 4.0.1.1  91/06/07  12:20:43  lwall
 * patch4: new copyright notice
 * 
 * Revision 4.0  91/03/20  01:58:29  lwall
 * 4.0 baseline.
 * 
 */

/* is the string for makedir a directory name or a filename? */

#define fatal Myfatal

#define MD_DIR 0
#define MD_FILE 1

void	util_init();
int	doshell();
char	*safemalloc();
char	*saferealloc();
char	*safecpy();
char	*safecat();
char	*cpytill();
char	*cpy2();
char	*instr();
#ifdef SETUIDGID
    int		eaccess();
#endif
char	*getwd();
void	cat();
void	prexit();
char	*get_a_line();
char	*savestr();
int	makedir();
void	setenv();
int	envix();
void	notincl();
char	*getval();
void	growstr();
void	setdef();
