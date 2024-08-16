/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/send2rwrt.c	1.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)send2rwrt.c	1.1 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	send2rewrite - check list of users for header rewriting

    SYNOPSIS
	void send2rewrite(Msg *pmsg, int surr_num, int level)

    DESCRIPTION
	send2rewrite() will traverse the current recipient list and
	do header rewriting processing for each user which matches.
*/

void send2rewrite(pmsg, surr_num, level)
Msg *pmsg;
int surr_num;
int level;
{
    static const char pn[] = "send2rewrite";
    Recip *l, *r;
    int origmatch = matchsurr(pmsg->orig, surrfile[surr_num].orig_regex, (char**)0, (char**)0, 0);

    Tout(pn, "Rewrite\n");
    /* Can't do anything else if we don't match here! */
    if (!origmatch)
	{
	Tout(pn, "No match on originator '%s':'%s'!\n", s_to_c(surrfile[surr_num].orig_pattern), s_to_c(pmsg->orig));
	send2move(pmsg, surr_num, surr_num + 1);
	return;
	}
    Tout(pn, "Matched originator '%s':'%s'!\n", s_to_c(surrfile[surr_num].orig_pattern), s_to_c(pmsg->orig));

    /* print a single message now for all names passed through here */
    if (flgT || flglb)
	{
	if (flgT) Tout(pn, "Suppressing header rewriting (-T)\n");
	if (flglb) Dout(pn, 0, "Suppressing header rewriting (-#)\n");
	send2move(pmsg, surr_num, surr_num + 1);
	return;
	}

    /* move matching recipients to a temp list */
    for (l = recips_head(pmsg, surr_num); ((r = l->next) != (Recip*) NULL); )
	{
	if (origmatch &&
	    matchsurr(r->name, surrfile[surr_num].recip_regex, (char**)0, (char**)0, 0))
	    {
	    if (flgd) pfmt(stdout, MM_INFO, ":668:Rewriting message for %s\n", s_to_c(r->name));
	    Tout(pn, "Matched recipient '%s'!\n", s_to_c(surrfile[surr_num].recip_pattern));
	    Tout((char*)0, "%s being rewritten\n", s_to_c(r->name));
	    send2mvrecip(pmsg, surr_num, surr_len + RECIPS_TEMP);
	    }

	else
	    {
	    /* move other recipients to pmsg->preciplist[surr_num+1] */
	    send2mvrecip(pmsg, surr_num, surr_num + 1);
	    }
	}

    /* we have something to rewrite, so do it */
    if (recips_exist(pmsg, surr_len + RECIPS_TEMP))
	if (have_rewrite_function(s_to_c(surrfile[surr_num].rewrite_cmd), rewritefile))
	    {
	    Msg msg;
	    Tmpfile *ptmpfile = new_Tmpfile();
	    int using_parents_tmpfile = 0;
	    init_Msg(&msg);
	    del_Tmpfile(msg.tmpfile);
	    msg.tmpfile = 0;
	    invoke_rewrite(s_to_c(surrfile[surr_num].rewrite_cmd), pmsg, pmsg->phdrinfo, msg.phdrinfo, rewritefile, ptmpfile);
	    if (ptmpfile->tmpf)
		msg.tmpfile = ptmpfile;
	    else
		{
		del_Tmpfile(ptmpfile);
		using_parents_tmpfile = 1;
		msg.tmpfile = pmsg->tmpfile;
		}
	    send2move2(pmsg, &msg, surr_len + RECIPS_TEMP, surr_num + 1);
	    sendlist(&msg, level);
	    if (using_parents_tmpfile)
		msg.tmpfile = 0;
	    send2move2(&msg, pmsg, surr_len + RECIPS_DONE, surr_len + RECIPS_DONE);
	    send2move2(&msg, pmsg, surr_len + RECIPS_SUCCESS, surr_len + RECIPS_DONE);
	    send2move2(&msg, pmsg, surr_len + RECIPS_FAILURE, surr_len + RECIPS_DONE);
	    fini_Msg(&msg);
	    }

	else
	    {
	    send2move(pmsg, surr_len + RECIPS_TEMP, surr_num + 1);
	    }
}

