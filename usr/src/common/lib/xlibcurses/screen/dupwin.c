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

#ident	"@(#)curses:common/lib/xlibcurses/screen/dupwin.c	1.6.2.3"
#ident  "$Header: dupwin.c 1.2 91/06/26 $"
#include	"curses_inc.h"

/*
 * Duplicate a window.
 *
 * SS:	calling makenew to allocate a new window is wastefull, since
 *	makenew initializes all the variables, and then we re-initialize
 *	the desired values to these variables.
 */

WINDOW	*
dupwin(win)
register	WINDOW	*win;
{
    register	WINDOW	*new;
    register	int	i, nlines = win->_maxy, ncolumns = win->_maxx,
			line_size = nlines * sizeof (short);
    register	chtype	**wincp, **newcp;
    int			ncolsav = ncolumns;

    /* allocate storage for new window and do block copy of old one into new */ 

    if ((new = (WINDOW *) malloc(sizeof (WINDOW))) == NULL)
	 goto out0;

    (void) memcpy(new, win, sizeof (WINDOW));

    /* allocate storage for "malloced" fields of the new window */

    if ((new->_firstch = (short *) malloc((unsigned) 2 * line_size)) == NULL)
         goto out1;
    else 
	win->_lastch = win->_firstch + nlines;

    if ((new->_y = (chtype **) malloc(nlines * sizeof (chtype *))) == NULL)
    {
	/*
	 * We put the free's here rather than after the image call, this
	 * is because _image free's all the rest of the malloc'ed areas.
	 */
	free((char *) new->_firstch);
out1:
	free((char *) new);
        goto out0;
    }

    if (_image(new) == ERR)
    {
out0:
	curs_errno = CURS_BAD_MALLOC;
#ifdef	DEBUG
	strcpy(curs_parm_err, "dupwin");
	curserr();
#endif	/* DEBUG */
	return ((WINDOW *) NULL);
    }

    /* copy information from "malloced" areas of the old window into new */

    wincp = win->_y;
    newcp = new->_y;
    ncolumns *= sizeof(chtype);
    for (i = 0; i < nlines; ++i, ++wincp, ++newcp)
	{
	register chtype		*ws, *we, *ns, *ne, wc;
	register int		n;

	ws = *wincp;
	we = ws + ncolsav - 1 ;
	/* skip partial characters */
	for(; ws <= we; ++ws)
		if(!ISCBIT(*ws))
			break;
	for(; we >= ws; --we)
		if(!ISCBIT(*we))
			break;
	if(we >= ws)
		{
		wc = *we;
		n = _scrwidth[TYPE(wc)];
		if((we + n) <= (*wincp + ncolsav))
			we += n;
		ns = *newcp + (ws - *wincp);
		ne = *newcp + (we - *wincp);
		(void)memcpy((char *)ns,(char *)ws,(ne-ns)*sizeof(chtype));
		}
	else
		ns = ne = *newcp + ncolsav;
	/* fill the rest with background chars */
	wc = win->_bkgd;
	for(ws = *newcp; ws < ns ; ++ws)
		*ws = wc;
	for(ws = *newcp+ncolsav-1; ws >= ne; --ws)
		*ws = wc;
    }

    (void) memcpy((char *) new->_firstch, (char *) win->_firstch, 2 * line_size);

    new->_flags |= _WINCHANGED;
    new->_ndescs = 0;
    /*
     * Just like we don't create this window as a subwin if the one
     * sent is, we don't create a padwin.  Besides, if the user
     * calls p*refresh a padwin will be created then.
     */
    new->_padwin = new->_parent = (WINDOW *) NULL;
    new->_pary = new->_parx = -1;

    new->_index = win->_index;
    new->_nbyte = win->_nbyte;
    new->_insmode = win->_insmode;
    (void)memcpy((char *)new->_waitc,(char *)win->_waitc,_csmax*sizeof(char));

    return (new);
}
