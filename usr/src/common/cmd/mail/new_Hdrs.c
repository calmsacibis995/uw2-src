/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/new_Hdrs.c	1.4.4.2"
#ident "@(#)new_Hdrs.c	1.8 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	new_Hdrs - allocate and initialize a header

    SYNOPSIS
	Hdrs *new_Hdrs(int hdrtype, const char *name, const char *val, int len)

    DESCRIPTION
	Allocate and initialize a header.
*/

static char MAnomem[] = ":407:malloc failed in %s(): %s\n";

Hdrs *new_Hdrs(hdrtype, name, val, len)
int hdrtype;
const char *name;
const char *val;
int len;
{
    static const char pn[] = "new_Hdrs";
    Hdrs *nhp;

    Dout(pn, 0, "entered\n");

    if ((nhp = (Hdrs*)malloc(sizeof(Hdrs))) == (Hdrs*)NULL)
	{
	errmsg(E_MEM, MAnomem, pn, Strerror(errno));
	done(1);
	}

    nhp->next = nhp->prev = 0;
    if ((nhp->value = s_copy_reserve(val, len+1, len+1)) == (string*)NULL)
	{
	errmsg(E_MEM, MAnomem, pn, Strerror(errno));
	done(1);
	}

    if ((hdrtype == H_NAMEVALUE) && name)
	{
	if ((nhp->name = strdup(name)) == (char*)NULL)
	    {
	    errmsg(E_MEM, MAnomem, pn, Strerror(errno));
	    done(1);
	    }
	}

    else
	nhp->name = header[hdrtype].tag;

    nhp->hdrtype = hdrtype;
    return nhp;
}
