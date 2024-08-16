/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)lp:cmd/lpadmin/default.c	1.6.2.5"
#ident  "$Header: default.c 1.2 91/06/27 $"

#include "stdio.h"
#include "errno.h"
#include "sys/types.h"

#include "lp.h"
#include "printers.h"

#define	WHO_AM_I	I_AM_LPADMIN
#include "oam.h"

#include "lpadmin.h"


/*
 * Procedure:     getdflt
 *
 * Restrictions:
 *               getdefault: None
 * Notes:
 *---------------------------PRINTER DEFAULT----------------
 *   RETURN DEFAULT DESTINATION
 */

char			*getdflt ()
{
	char			*name;

	if ((name = getdefault()))
		return (name);
	else
		return ("");
}

/*
 * Procedure:     newdflt
 *
 * Restrictions:
 *               putdefault: None
 *               deldefault: None
 * notes - ESTABLISH NEW DEFAULT DESTINATION
 */

void			newdflt (name)
	char			*name;
{
	BEGIN_CRITICAL
		if (name && *name && !STREQU(name, NAME_NONE)) {
			if (putdefault(name) == -1) {
				LP_ERRMSG1 (ERROR, E_ADM_WRDEFAULT, PERROR);
				done (1);
			}

		} else {
			if (deldefault() == -1) {
				LP_ERRMSG1 (ERROR, E_ADM_WRDEFAULT, PERROR);
				done (1);
			}

		}
	END_CRITICAL

	return;
}

/*
 * Procedure:     getcopy
 *
 * Restrictions:
 *               getcpdefault: None
 * Notes
 *---------------------------COPY DEFAULT----------------
 ** RETURN DEFAULT DESTINATION
 */

char			*getcopy ()
{
	char *name,*getcpdefault();

	if ((name = (char *)getcpdefault()))
		return (name);
	else
		return ("");
}
/*
 * Procedure:     newcopy
 *
 * Restrictions:
 *               putcpdefault: None
 *
 * notes - ESTABLISH NEW DEFAULT DESTINATION
 */

void			newcopy (name)
	char			*name;
{
	BEGIN_CRITICAL
		if (name && *name && !STREQU(name, NAME_NONE)) {
			if (putcpdefault(name) == -1) {
				LP_ERRMSG1 (ERROR, E_ADM_WRCPDEFLT, PERROR);
				done (1);
			}

		} else {
				LP_ERRMSG1 (ERROR, E_ADM_WRCPDEFLT, PERROR);
				done (1);

		}
	END_CRITICAL

	return;
}
