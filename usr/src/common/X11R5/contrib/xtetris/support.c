/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)r4xtetris:support.c	1.2"
#endif

#include "defs.h"

block_can_drop(shape_no, xpos, ypos, rot)
        int     shape_no, xpos, ypos, rot;
{
  rotshape_ptr r = &shape[shape_no].forms[rot];
  int     x, y, c;

  /* Find highest non-zero ycoordinate for each x */

  for (x = 0; x < 4; x++)
    {
      y = ypos + (c = r->highesty[x]);
      if ((c != 0) && (y >= 0))
	if ((y >= resources.boxes_high) || (grid[xpos+x][y] != NULL))
	  return (FALSE);
    }
  return TRUE;
}

block_can_left(shape_no, xpos, ypos, rot)
  int     shape_no, xpos, ypos, rot;
{
  int x, y, yg, c;
  rotshape_ptr r = &shape[shape_no].forms[rot];
  
  /* get the lowest x value for y, in (3-c) */
  

  for (y = 0, yg = ypos; y < 4; y++, yg++) 
    {
      x = xpos + (c = r->lowestx[y]);
      if (c != -2)
	if ((x < 0) || ((y >= 0) && (grid[x][yg] != NULL)))
	  return (FALSE);
    }
  return TRUE;
}

block_can_right(shape_no, xpos, ypos, rot)
  int     shape_no, xpos, ypos, rot;
{
  int     x, y, yg, c;
  rotshape_ptr r = &shape[shape_no].forms[rot];

  for (y = 0, yg = ypos; y < 4; y++, yg++)
    {
      x = xpos + (c = r->highestx[y]);
      if ((c != 0) && (x >= 0))
	if ((x == resources.boxes_wide) || ((y >= 0) && (grid[x][yg] != NULL)))
	  return (FALSE);
    }
  return TRUE;
}

remove_full_lines(starty)
  int     starty;
{
  int     y, y2, ymax, x;
  unsigned char linefull[MAXBOXESHIGH], foundfull;

  ymax = starty + 4;
  if (resources.boxes_high < ymax) ymax = resources.boxes_high;
  
  foundfull = FALSE;
  for (y = starty; y < ymax; y++) {
    linefull[y] = TRUE;
    for (x = 0; x < resources.boxes_wide; x++)
      if (grid[x][y] == NULL) {
	linefull[y] = False;
	break;
      }
    foundfull |= linefull[y];
    if (linefull[y]) 
      XFillRectangle( XtDisplay(canvas), XtWindow(canvas), erasegc,
		 0, y*resources.boxsize, resources.boxsize*resources.boxes_wide, resources.boxsize );
  }
  if (!foundfull) return;
  
  /* Wait a bit for the user to see it. */
      
  XFlush(XtDisplay(toplevel));
  sleep(1);  /* cannot use usleep because incompatible
		with hpux. */
  
  /* Now change the data. */

  for (y = starty; y < ymax; y++) 
    if (linefull[y]) 
      {
	for (y2 = y; y2 > 0; y2--)
	  for (x = 0; x < resources.boxes_wide; x++) 
	    grid[x][y2] = grid[x][y2 - 1];
	for (x = 0; x < resources.boxes_wide; x++)
	  grid[x][0] = NULL;
      
	XCopyArea(XtDisplay(toplevel),
		  XtWindow(canvas),XtWindow(canvas),gc,
		  0,0, resources.boxsize*resources.boxes_wide, y*resources.boxsize,0, resources.boxsize);
	XClearArea(XtDisplay(toplevel),XtWindow(canvas),
		   0, 0, resources.boxsize*resources.boxes_wide, resources.boxsize, False );
	rows++;
      }

  XFlush(XtDisplay(toplevel));
}

check_rot(shape_no, xpos, ypos, newrot)
        int     shape_no, xpos, ypos, newrot;
{
  rotshape_ptr sh = &shape[shape_no].forms[newrot];
  int x, y, i;

  for (i = 0; i < sh->nrect; i++)
    {
      XRectangle *r = &sh->urect[i];

      int ymax = ypos+r->y+r->height;
      for (y = ypos+r->y; y < ymax; y++) 
	if (y >= 0)
	  {
	    int xmax = xpos+r->x+r->width;
	    
	    if (y >= resources.boxes_high) return FALSE;
	    for (x = xpos+r->x; x < xmax; x++)
	      {
		if ((x < 0) || (x >= resources.boxes_wide) || (grid[x][y] != NULL)) return FALSE;
	      }
	    
	  }
    }
  return TRUE;
}
