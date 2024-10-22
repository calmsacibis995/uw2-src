/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)xdm:rpcauth.c	1.2"
/*
 * xdm - display manager daemon
 *
 * $XConsortium: rpcauth.c,v 1.2 91/07/18 18:55:01 rws Exp $
 *
 * Copyright 1988 Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  M.I.T. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * Author:  Keith Packard, MIT X Consortium
 */

/*
 * rpcauth
 *
 * generate SecureRPC authorization records
 */

# include   <X11/Xos.h>
# include   <rpc/rpc.h>
# include   <rpc/key_prot.h>
# include   "dm.h"

/*ARGSUSED*/
SecureRPCInitAuth (name_len, name)
    unsigned short  name_len;
    char	    *name;
{
}

Xauth *
SecureRPCGetAuth (namelen, name)
    unsigned short  namelen;
    char	    *name;
{
    char    key[MAXNETNAMELEN+1];
    Xauth   *new;

#ifdef DEBUG
	Debug("SecureRPCGetAuth\n");
#endif
    new = (Xauth *) malloc (sizeof *new);
    if (!new)
	return (Xauth *) 0;
    new->family = FamilyWild;
    new->address_length = 0;
    new->address = 0;
    new->number_length = 0;
    new->number = 0;

    getnetname (key);
    Debug ("System netname %s\n", key);
    new->data_length = strlen(key);
    new->data = (char *) malloc (new->data_length);
    if (!new->data)
    {
	free ((char *) new);
	return (Xauth *) 0;
    }
    new->name = (char *) malloc (namelen);
    if (!new->name)
    {
	free ((char *) new->data);
	free ((char *) new);
	return (Xauth *) 0;
    }
    bcopy (name, new->name, namelen);
    new->name_length = namelen;
    bcopy (key, new->data, new->data_length);
    return new;
}
