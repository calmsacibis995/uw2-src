/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libetitam:llib-lmenu.c	1.1"
/*LINTLIBRARY*/
#include "tam.h"
#include "menu.h"
#include "wind.h"
#include "message.h"
#include "kcodes.h"
#include <ctype.h>

int	mbegin(m) menu_t *m; {return 0;}
int	mcitems(m, pwidth) menu_t *m; int *pwidth; {return 0;}
void	nctitle(m,pheight,pwidth) menu_t *m; int *pheight,*pwidth; {}
void	mdisplay(m,supress,nitems,maxwidth,titheight,titwidth,prows,pvcols)
	menu_t *m;
	int supress,nitems,maxwidth,titheight,titwidth;
	int *prows, *pvcols;
	{}
void	mend(m) menu_t *m; {}
int	menu(m, op) menu_t *m; int op; {return 0;}
int	minput(m) menu_t *m; {return 0;}
int	mmatch(m) menu_t *m; {return 0;}
void	mscroll(m,nitems,rows,n) menu_t *m; int nitems,rows,n; {}
int	mshape(nitems,maxwidth,prows,pcols) int nitems,maxwidth, *prows,*pcols; {return 0;}
void	mtitle(m,width) menu_t *m; int width; {}
void	mtrunc(s1,s2,n) char *s1, *s2; int n; {}
