/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/r822_getc.c	1.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)r822_getc.c	1.2 93/06/16 13:01:37"
#include "mail.h"
#include "r822.h"
#include "r822t.h"
/*
** Like fgetc(), ungetc(), but supporting arbitrary amounts of pushback.
** By no small coincidence, these functions are perfect for use with
** r822_find_field_name() and r822_find_field_body().
*/

int
r822_fgetc(rfp)
r822_FILE *rfp;
{
	char ch;
	int chint;
	if (!rfp)
	{
		return (EOF);  /* hey, cut it out! */
	}
	
	if (s_curlen(rfp->pushback) == 0)
	{
		/*
		** Might get interrupted by a signal, but let's not be ridiculous.
		*/
		int edex;
		for (edex=0; edex<1000; ++edex)
		{
			chint = fgetc(rfp->fp);
			if (chint == EOF  &&  !feof(rfp->fp))
			{
				continue;
			}
			break;
		}
	}
	else
	{
		s_skipback(rfp->pushback);
		ch = s_ptr_to_c(rfp->pushback)[0];
		chint = ch;
	}
	return (chint);
}

int
r822_ungetc(chint, rfp)
int chint;
r822_FILE *rfp;
{
	if (!rfp)
	{
		return (EOF);  /* hey, cut it out! */
	}
	if (chint != EOF)
	{
		s_putc(rfp->pushback,chint);
	}
	return (chint);
}
