/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/sel_disp.c	1.8.3.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)sel_disp.c	2.9 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	sel_disp - selectively display header lines

    SYNOPSIS
	int sel_disp (int type, int hdrtype, const char *s, int Pflg);

    DESCRIPTION
	If in default display mode from printmail(), selectively output
	header lines. Any recognized header lines will have flag stored in
	header[] structure. Other header lines which should be displayed in
	the default output mode will be listed in the seldisp[] array.
	This can all be overridden via the 'P' command at the ? prompt.

	Return:  0 - display
		-1 - don't display
*/

int sel_disp (type, hdrtype, s, Pflg)
CopyLetFlags	type;
int		hdrtype;
const char	*s;
int		Pflg;
{
	static const char pn[] = "sel_disp";
	register const char	*p;
	static	int	sav_lastrc = 0;
	int		rc = 0;
	const Hdr *ph;

	if (Pflg || (type != TTY)) {
		return (0);
	}

	switch (hdrtype) {
	case H_CONT:
		rc = sav_lastrc;
		break;
	case H_NAMEVALUE:
		ph = seldisp;
		for ( ; ph->tag; ph++) {
			if (casncmp(s, ph->tag, ph->length) == 0) {
				break;
			}
		}
		if (ph->tag == (char *)NULL) {
			rc = -1;
		}
		break;
	default:
		if (header[hdrtype].default_display == FALSE) {
			rc = -1;
			break;
		}
	}

	Dout(pn, 2, "type = %d, hdrtype = %d/'%s', rc = %d\n",
		(int)type, hdrtype, header[hdrtype].tag, rc);
	sav_lastrc = rc;	/* In case next one is H_CONT... */
	return (rc);
}
