/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/popper/pop_xmit.c	1.1"
/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char copyright[] = "Copyright (c) 1990 Regents of the University of California.\nAll rights reserved.\n";
static char SccsId[] = "@(#)@(#)pop_xmit.c	2.1  2.1 3/18/91";
#endif not lint

#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>

#if defined NOVELL || defined SVR4
#include <fcntl.h>
#endif

#include "popper.h"

/*
 *  xmit:   POP XTND function to receive a message from 
 *          a client and send it in the mail
 */

pop_xmit (p)
POP     *   p;
{
    FILE                *   tmp;                    /*  File descriptor for 
                                                        temporary file */
    char                    buffer[MAXLINELEN];     /*  Read buffer */
    char                    temp_xmit[MAXDROPLEN];  /*  Name of the temporary 
                                                        filedrop */
#if defined NOVELL || defined SVR4
    int		            stat;
#else
    union   wait            stat;
#endif
    int                     id, pid;

    /*  Create a temporary file into which to copy the user's message */
    (void)mktemp((char *)strcpy(temp_xmit,POP_TMPXMIT));
#ifdef DEBUG
    if(p->debug)
        pop_log(p,POP_DEBUG,
            "Creating temporary file for sending a mail message \"%s\"\n",
                temp_xmit);
#endif DEBUG
    if ((tmp = fopen(temp_xmit,"w+")) == NULL)
        return (pop_msg(p,POP_FAILURE,
            "Unable to create temporary message file \"%s\", errno = %d",
                temp_xmit,errno));

    /*  Tell the client to start sending the message */
    pop_msg(p,POP_SUCCESS,"Start sending the message.");

    /*  Receive the message */
#ifdef DEBUG
    if(p->debug)pop_log(p,POP_DEBUG,"Receiving mail message");
#endif DEBUG
    while (fgets(buffer,MAXLINELEN,p->input)){
        /*  Look for initial period */
#ifdef DEBUG
        if(p->debug)pop_log(p,POP_DEBUG,"Receiving: \"%s\"",buffer);
#endif DEBUG
        if (*buffer == '.') {
            /*  Exit on end of message */
            if (strcmp(buffer,".\r\n") == 0) break;
        }
        (void)fputs (buffer,tmp);
    }
    (void)fclose (tmp);

#ifdef DEBUG
    if(p->debug)pop_log(p,POP_DEBUG,"Forking for \"%s\"",MAIL_COMMAND);
#endif DEBUG
    /*  Send the message */
    switch (pid = fork()) {
        case 0:
            (void)fclose (p->input);
            (void)fclose (p->output);       
            (void)close(0);
            if (open(temp_xmit,O_RDONLY,0) < 0) (void)_exit(1);
            (void)execl (MAIL_COMMAND,"send-mail","-t","-oem",NULLCP);
            (void)_exit(1);
        case -1:
#ifdef DEBUG
            if (!p->debug) (void)unlink (temp_xmit);
#endif DEBUG
            return (pop_msg(p,POP_FAILURE,
                "Unable to execute \"%s\"",MAIL_COMMAND));
        default:
            while((id = wait(&stat)) >=0 && id != pid);
            if (!p->debug) (void)unlink (temp_xmit);
#if defined NOVELL || defined SVR4
            if (WEXITSTATUS(stat))
                return (pop_msg(p,POP_FAILURE,"Unable to send message"));
#else
            if (stat.w_retcode)
                return (pop_msg(p,POP_FAILURE,"Unable to send message"));
#endif
            return (pop_msg (p,POP_SUCCESS,"Message sent successfully"));
    }

}
