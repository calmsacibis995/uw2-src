/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/popper/pop_pass.c	1.1"
/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char copyright[] = "Copyright (c) 1990 Regents of the University of California.\nAll rights reserved.\n";
static char SccsId[] = "@(#)@(#)pop_pass.c	2.3  2.3 4/2/91";
#endif not lint

#include <stdio.h>
#include <sys/types.h>
#if defined NOVELL || defined SVR4
#include <shadow.h>
#include <string.h>
#include <crypt.h>
#else
#include <strings.h>
#endif
#include <pwd.h>
#include "popper.h"


/* 
 *  pass:   Obtain the user password from a POP client
 */

int pop_pass (p)
POP     *   p;
{
    struct spwd	*pw ;
    struct passwd *pwrd ;
    char 	*salt ;


    /*  Look for the user in the password file */
    if ((pw = getspnam(p->user)) == NULL)
        return (pop_msg(p,POP_FAILURE,
            "Can't locate \"%s\" in Shadow file",p->user));

    /*  We don't accept connections from users with null passwords */
    if (pw->sp_pwdp == NULL)
        return (pop_msg(p,POP_FAILURE,
            "No Password supplied for \"%s\"" ,p->user));

    /*  Compare the supplied password with the password file entry */
    salt = &pw->sp_pwdp[0] ; 		/* Salt is the first two characters */
    if (strcmp (crypt (p->pop_parm[1], salt), pw->sp_pwdp) != 0)
        return (pop_msg(p,POP_FAILURE,
            "Incorrect Password supplied \"%s\"",p->user));

    /*  Get the password entry info into a passwd struct so everyone
        else will be happy with it 				     */
    pwrd = getpwnam (p->user) ;
    /*  Build the name of the user's maildrop */
    (void)sprintf(p->drop_name,"%s/%s",POP_MAILDIR,p->user);

    /*  Make a temporary copy of the user's maildrop */
    /*    and set the group and user id */
    if (pop_dropcopy(p,pwrd) != POP_SUCCESS) return (POP_FAILURE);

    /*  Get information about the maildrop */
    if (pop_dropinfo(p) != POP_SUCCESS) return(POP_FAILURE);

    /*  Initialize the last-message-accessed number */
    p->last_msg = 0;

    /*  Authorization completed successfully */
    return (pop_msg (p,POP_SUCCESS,
        "%s has %d message(s) (%d octets).",
            p->user,p->msg_count,p->drop_size));
}
