/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)lp:cmd/lpadmin/rmdest.c	1.1.5.5"
#ident  "$Header: rmdest.c 1.2 91/06/27 $"

#include "stdio.h"
#include "ctype.h"
#include "errno.h"
#include "sys/types.h"

#include "lp.h"
#include "msgs.h"
#include "access.h"
#include "class.h"
#include "printers.h"

#define	WHO_AM_I	I_AM_LPADMIN
#include "oam.h"

#include "lpadmin.h"

extern void		fromallclasses();

/*
 * Procedure:     rmdest
 *
 * Restrictions:
 *               delprinter: None
 * notes - REMOVE DESTINATION
 */

void			rmdest (aclass, dest)
	int			aclass;
	char			*dest;
{
	int			rc,
				type;


	if (!aclass)
		type = S_UNLOAD_PRINTER;
	else
		type = S_UNLOAD_CLASS;


	send_message(type, dest, "", "");
	rc = output(type + 1);

	switch (rc) {
	case MOK:
	case MNODEST:
		BEGIN_CRITICAL
			if (
				aclass && delclass(dest) == -1
			     || !aclass && delprinter(dest) == -1
			) {
				if (rc == MNODEST && errno == ENOENT)
					LP_ERRMSG1 (
						ERROR,
						E_ADM_NODEST,
						dest
					);
				else
				if (rc != MNODEST)
					LP_ERRMSG2 (
						ERROR,
						E_ADM_DELSTRANGE,
						dest,
						PERROR
					);
				else
				if (aclass)
					LP_ERRMSG2 (
						ERROR,
						E_LP_DELCLASS,
						dest,
						PERROR
					);
				else
					LP_ERRMSG2 (
						ERROR,
						E_LP_DELPRINTER,
						dest,
						PERROR
					);

				done(1);
			}
		END_CRITICAL

		/*
		 * S_UNLOAD_PRINTER tells the Spooler to remove
		 * the printer from all classes (in its internal
		 * tables, of course). So it's okay for us to do
		 * the same with the disk copies.
		 */
		if (!aclass)
			fromallclasses (dest);

		if (STREQU(getdflt(), dest))
			newdflt (NAME_NONE);

		break;

	case MBUSY:
		LP_ERRMSG1 (ERROR, E_ADM_DESTBUSY, dest);
		done (1);

	case MNOPERM:	/* taken care of up front */
		LP_ERRMSG1 (ERROR, E_LP_DSTUNK, dest);
		done (1);
	default:
		LP_ERRMSG1 (ERROR, E_LP_BADSTATUS, rc);
		done (1);
		break;

	}
	return;
}
