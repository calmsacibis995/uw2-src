/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sccs:lib/comobj/del_ba.c	6.3"
# include	"../../hdr/defines.h"


char *
del_ba(dt,str)
register struct deltab *dt;
char *str;
{
	register char *p;
	char *sid_ba(), *date_ba();

	p = str;
	*p++ = CTLCHAR;
	*p++ = BDELTAB;
	*p++ = ' ';
	*p++ = dt->d_type;
	*p++ = ' ';
	p = sid_ba(&dt->d_sid,p);
	*p++ = ' ';
	date_ba(&dt->d_datetime,p);
	while (*p++)
		;
	--p;
	*p++ = ' ';
	copy(dt->d_pgmr,p);
	while (*p++)
		;
	--p;
	*p++ = ' ';
	sprintf(p,"%d",dt->d_serial);
	while (*p++)
		;
	--p;
	*p++ = ' ';
	sprintf(p,"%d",dt->d_pred);
	while (*p++)
		;
	--p;
	*p++ = '\n';
	*p = 0;
	return(str);
}
