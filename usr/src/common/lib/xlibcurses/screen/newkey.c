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

#ident	"@(#)curses:common/lib/xlibcurses/screen/newkey.c	1.4.2.4"
#ident  "$Header: newkey.c 1.2 91/06/26 $"
#include	"curses_inc.h"

/*
 * Set a new key or a new macro.
 *
 * rcvchars: the pattern identifying the key
 * keyval: the value to return when the key is recognized
 * macro: if this is not a function key but a macro,
 * 	tgetch() will block on macros.
 */

newkey(rcvchars, keyval, macro)
char	*rcvchars;
short	keyval;
bool	macro;
{
    register	_KEY_MAP	**keys, *key_info,
				**prev_keys = cur_term->_keys;
    short	*numkeys = &cur_term->_ksz, len;
    unsigned	char	*str;

    if ((!rcvchars) || (*rcvchars == '\0') || (keyval < 0) ||
	(((keys = (_KEY_MAP **) malloc(sizeof (_KEY_MAP *) * (*numkeys + 1))) == NULL)))
    {
	goto bad;
    }

    len = strlen(rcvchars) + 1;

    if ((key_info = (_KEY_MAP *) malloc(sizeof (_KEY_MAP) + len)) == NULL)
    {
	free (keys);
bad :
	term_errno = TERM_BAD_MALLOC;
#ifdef	DEBUG
	strcpy(term_parm_err, "newkey");
#endif	/* DEBUG */
	return (ERR);
    }

    if (macro)
    {
	(void) memcpy((char *) keys, (char *) prev_keys, (int) (*numkeys * sizeof (_KEY_MAP *)));
	keys[*numkeys] = key_info;
    }
    else
    {
	short	*first = &(cur_term->_first_macro);

	(void) memcpy((char *) keys, (char *) prev_keys, (int) (*first * sizeof (_KEY_MAP *)));
	(void) memcpy((char *) &(keys[*first + 1]), (char *) &(prev_keys[*first]),
	    (int) ((*numkeys - *first) * sizeof (_KEY_MAP *)));
	keys[(*first)++] = key_info;
	cur_term->_lastmacro_ordered++;
    }
    if (prev_keys != NULL)
	free(prev_keys);
    cur_term->_keys = keys;

    (*numkeys)++;
    key_info->_sends = str = (unsigned char *) key_info + sizeof(_KEY_MAP);
    (void) memcpy(str, rcvchars, len);
    key_info->_keyval = keyval;
    cur_term->funckeystarter[*str] |= (macro ? _MACRO : _KEY);

    return (OK);
}
