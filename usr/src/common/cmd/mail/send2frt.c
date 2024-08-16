/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/send2frt.c	1.5.3.4"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)send2frt.c	1.12 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	send2findright - an auxiliary function used by send2d_p() and send2tran()

    SYNOPSIS
	SendSurgRet send2findright(Msg *pmsg, int batchsize, Recip *curmsg, int wherefrom,
	    int whereexec, int wheretemp, int surr_num,
	    char **lbraslist, char **lbraelist, char *cmdname,
	    const char *pn, SendSurgRet execcmd, int ckrc, int nowait4process);

    DESCRIPTION
	send2findright() fills in the list with recipients who match on the right side
	of a batched command, executes the command, checks the return code, and returns
	SUCCESS, CONTINUE or FAILURE.

	execcmd determines whether the command should actually be executed.
*/

SendSurgRet send2findright(pmsg, batchsize, curmsg, wherefrom, whereexec, wheretemp, surr_num,
	lbraslist, lbraelist, cmdname, pn, execcmd, ckrc, nowait4process)
Msg		*pmsg;
int		batchsize;
Recip		*curmsg;
int		wherefrom;
int		whereexec;
int		wheretemp;
int		surr_num;
char		**lbraslist;
char		**lbraelist;
char		*cmdname;
const char	*pn;
SendSurgRet	execcmd;
int		ckrc;
int		nowait4process;
{
    Recip *l, *r;

    if (batchsize >= 0)
	{
	/* add 2 for the 2 NUL's at the end of each string */
	int curlen = strlen(s_to_c(curmsg->cmdl)) + strlen(s_to_c(curmsg->cmdr)) + 2;
	int rnbra = surrfile[surr_num].recip_nbra;

	for (l = recips_head(pmsg, wherefrom); ((r = l->next) != (Recip*) NULL); )
	    {
	    int len;
	    if (!r->cmdl)
		{
		if (!matchsurr(r->name, surrfile[surr_num].recip_regex, lbraslist, lbraelist, rnbra))
		    {
		    /* move recip to list->surr_recips[i+1] */
		    send2mvrecip(pmsg, wherefrom, surr_num + 1);
		    continue;
		    }

		Tout(pn, "Matched recipient '%s':'%s'\n", s_to_c(surrfile[surr_num].recip_pattern), s_to_c(r->name));
		r->cmdl =
		    cmdexpand(pmsg, r, surrfile[surr_num].cmd_left, lbraslist, lbraelist, r->cmdl);
		r->cmdr =
		    cmdexpand(pmsg, r, surrfile[surr_num].cmd_right, lbraslist, lbraelist, r->cmdr);
		}

	    /* add 1 for the NUL at the end of the string */
	    len = strlen(s_to_c(r->cmdr)) + 1;
	    if ((len + curlen < batchsize) &&
		(strcmp(s_to_c(curmsg->cmdl), s_to_c(r->cmdl)) == SAME))
		{
		s_delete(r->cmdl);
		send2mvrecip(pmsg, wherefrom, whereexec);
		curlen += len;
		}

	    else
		send2mvrecip(pmsg, wherefrom, wheretemp);
	    }
	}

    /* Report the commands being used for delivery */
    if (flgT)
	{
	Tout(pn, "%s '%s", cmdname, s_to_c(curmsg->cmdl));
	for (r = recips_head(pmsg, whereexec)->next; r != (Recip*) NULL && r->cmdr; r = r->next)
	    Tout((char*)0, " %s", s_to_c(r->cmdr));
	Tout((char*)0, "'\n");
	}
    if (flglb || flgd)
	{
	pfmt(stdout, MM_INFO, ":439:%s '%s", cmdname, s_to_c(curmsg->cmdl));
	for (r = recips_head(pmsg, whereexec)->next; r != (Recip*) NULL && r->cmdr; r = r->next)
	    (void) printf(" %s", s_to_c(r->cmdr));
	(void) printf("'\n");
	}

    /* Execute the commands (perhaps) */
    switch (execcmd)
	{
	case SUCCESS:	return execcmd;
	case CONTINUE:	return execcmd;
	case FAILURE:
	default:
	    {
	    Hdrinfo *phdrinfo = 0;
	    Hdrinfo *svhdrinfo = 0;
	    Tmpfile *ptmpfile = 0;
	    Tmpfile *svtmpfile = 0;
	
	    if (surrfile[surr_num].rewrite_cmd && (s_curlen(surrfile[surr_num].rewrite_cmd) > 0) &&
		have_rewrite_function(s_to_c(surrfile[surr_num].rewrite_cmd), rewritefile))
		{
		phdrinfo = new_Hdrinfo();
		ptmpfile = new_Tmpfile();
		invoke_rewrite(s_to_c(surrfile[surr_num].rewrite_cmd), pmsg, pmsg->phdrinfo, phdrinfo, rewritefile, ptmpfile);
		svhdrinfo = pmsg->phdrinfo;
		pmsg->phdrinfo = phdrinfo;
		if (ptmpfile->tmpf)
		    {
		    svtmpfile = pmsg->tmpfile;
		    pmsg->tmpfile = ptmpfile;
		    }
		else
		    {
		    del_Tmpfile(ptmpfile);
		    }
		}

	    fini_Tmpfile(&pmsg->SURRinput);
	    send2exec(pmsg, whereexec, nowait4process);

	    if (phdrinfo)
		{
		pmsg->phdrinfo = svhdrinfo;
		del_Hdrinfo(phdrinfo);
		}

	    if (svtmpfile)
		{
		pmsg->tmpfile = svtmpfile;
		del_Tmpfile(ptmpfile);
		}

	    return ckrc ? cksurg_rc(surr_num, pmsg->surg_rc) : CONTINUE;
	    }
	}
}
