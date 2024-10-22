/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libetitam:mshape.c	1.3"
#include "tam.h"
#include "menu.h"

/****************************************************************************

  MSHAPE(nitems,maxwidth,&rows,&cols)

  This routine goes through possible row/column combinations, evaluating
  the SHAPE(width,height) metric each time, recording the result.  The
  row/column which produces a metric arithmetically closest to IDEAL
  is the one we choose.

****************************************************************************/

#define SHAPE(w,h)	((1000*h)/(2*w))
#define IDEAL		(1000)

int
mshape(nitems,maxwidth,prows,pcols)
int nitems,maxwidth;
int *prows,*pcols;
{
  register int rows,cols,width,height;
  int metric;
  int bestmetric;
  char found = 0;
  int maxlines = LINES-2;    /* LINES and COLS are */
  int maxcols  = COLS-3;     /* extern in tam.h    */

  for (cols=1 ; cols<=nitems ; cols++) {
    rows = nitems / cols;
    if (rows * cols < nitems) {
      rows++;
    }
    width = cols * (maxwidth + M_CSPACE) + 2*M_LRMARGIN;
    height = rows + 2*M_TBMARGIN + 2;

    if (width > maxcols || height > maxlines) {
      continue;
    }
    metric = SHAPE(width,height);
    metric -= IDEAL;
    if (metric < 0) {
      metric = -metric;
    }
    if (metric < bestmetric || !found) {
      bestmetric = metric;
      *prows = rows;
      *pcols = cols;
      found = 1;
    }
  }
  if (found) {
    return(1);
  }
  else {
    *prows = maxlines - 2*M_TBMARGIN;
    *pcols = 1;
    return(0);
  }
}
