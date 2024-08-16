/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/sendmail.c	1.14.7.7"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)sendmail.c	2.45 'attmail mail(1) command'"
#include "mail.h"
#include "r822.h"
/*
    NAME
	sendmail - High level sending routine

    SYNOPSIS
	void sendmail(int argc, char **argv)

    DESCRIPTION
	Generate a list of recipients based on the argument list and send
	mail to the list. Argc and argv point to the list of names AFTER
	all options have been removed.

	Note that no message is sent if there is absolutely no input.
	If there is ANY input, even a blank line, then a message will be
	sent.
*/

static int lookupheader ARGS((string *s));
/*static void save_from_information ARGS((Msg *pmsg, string *line, char *RFC822datestring, string **pfromU, string **pfromS, string **preceived));*/
static void save_from_information ARGS((Msg *pmsg, string *line, char *RFC822datestring, string **pfromU, string **pfromS));
static void encode_header_to_quoted_printable ARGS((string *in, string *out));
static void memrepl ARGS((char *p, int len, int oldc, int newc));

void sendmail(argc, argv)
char **argv;
{
    static const char pn[] = "sendmail";
    char	*tp;
    char	last1c;
    char	*lastBang;
    char	*lastAt;
    char	*lastColon;
    string	*tmpstr = s_new();
    int		i;
    int		ttyf = FALSE;
    int		hdrtyp = 0;
    int		ctf = FALSE;
    Hdrs	*hptr;
    Msg		msg;
    int		msgseen;
    string	*fromS = s_new(), *fromU = s_new();
    char	datestring[60];		/* Date in mail(1) format */
    char	RFC822datestring[60];	/* Date in RFC822 format */
    int		add_to;
    t_Content	hdrbinflag = C_Text;
#ifdef JALE
    int		force_7bit_headers = isyesno(mgetenv("FORCE_7BIT_HEADERS"), Default_False);
#else
    int		force_7bit_headers = isyesno(mgetenv("FORCE_7BIT_HEADERS"), Default_True);
#endif
    string	*sline = s_new();
    char	*haveline = 0;

    Dout(pn, 0, "entered\n");

    /* Check for invalid user names. */
    for (i = 1; i < argc; ++i)
	{
	/* No names like -user allowed. */
	if (argv[i][0] == '-')
	    {
	    if (argv[i][1] == '\0')
		errmsg(E_SYNTAX,":410:Hyphens MAY NOT be followed by spaces\n");
	    else
		errmsg(E_SYNTAX,":411:Options MUST PRECEDE persons\n");
	    done(0);
	    }

	/* Ensure no NULL names in list. */
	if (argv[i][0] == '\0' || argv[i][strlen(argv[i])-1] == '!')
	    {
	    errmsg(E_SYNTAX,":412:Null names are not allowed\n");
	    done(0);
	    }
	}

    initsurrfile();
    init_Msg(&msg);
    /* By default, assume local message. */
    msg.localmessage = 1;

    if(ReturnPath != NULL) {
	msg.Rpath = s_append(msg.Rpath, ReturnPath);
	msg.localmessage = 0;
	}

    saveint = setsig(SIGINT, savdead);
    mailR_set_recipients(argv);

    mktmp(msg.tmpfile);
    /* Format time */
    mkdate(datestring);
    rfc822date(datestring, RFC822datestring);

    /* Write out the from line header for the letter */
    if(ReturnPath != NULL)
	{
	tmpstr = s_xappend(tmpstr, ReturnPath, " ", datestring, (char*)0);
	if((lastBang = strrchr(ReturnPath, '!')) != NULL)
	    {
	    fromU = s_append(fromU, lastBang + 1);
	    }
	else
	    {
	    if((lastColon = strrchr(ReturnPath, ':')) == NULL) lastColon = ReturnPath - 1;
	    if((lastAt = strrchr(lastColon, '@')) != NULL) *lastAt = '\0';

	    fromU = s_append(fromU, lastColon + 1);

	    if(lastAt != NULL) *lastAt = '@';
	    }
	}
    else
	{
	tmpstr = s_xappend(tmpstr, my_name, " ", datestring, (char*)0);
	fromU = s_append(fromU, my_name);
	}

    save_a_hdr_n(msg.phdrinfo, s_to_c(tmpstr), s_curlen(tmpstr), H_FROM, (char*)0);
    tmpstr = s_xappend(tmpstr, " remote from ", remotefrom, (char*)0);
    save_a_hdr_n(msg.phdrinfo, s_to_c(tmpstr), s_curlen(tmpstr), H_RFROM, (char*)0);
    /*
    s_reset(tmpstr);
    */

    /*
     * Read mail message, allowing for lines of infinite
     * length. This is tricky: have to watch for newlines.
     */
    ttyf = isatty(fileno(stdin));
    /*fromU = s_append(fromU, my_name);*/

    /* If debugging, forget the message */
    /* and pretend a message has been seen. */
    if (flgT || flglb)
	msgseen = 1;

    /* scan header & save relevant info. */
    else
	{
	string *curheader;
	int emptyline = 0;

	/* read first line */
	haveline = s_read_line(stdin, sline);
	if (!haveline) goto cleanup;

	/* rmail allows one From and 0 or more >From lines */
	if (!ismail)
	    {
	    /* be extra forgiving and allow either From or >From on all lines */
	    while (haveline &&
		   ((strncmp(s_to_c(sline), ">From ", 6) == 0) ||
		    (strncmp(s_to_c(sline), "From ", 5) == 0)))
		{
		/*save_from_information(&msg, sline, RFC822datestring, &fromU, &fromS, &tmpstr);*/
		save_from_information(&msg, sline, RFC822datestring, &fromU, &fromS);
		haveline = s_read_line(stdin, s_restart(sline));
		msgseen = 1;
		}
	    }

	/* read the RFC 822 headers */
	while (haveline)
	    {
	    int c;

	    /* empty sline signals end of headers */
	    if (((s_curlen(sline) == 1) && (s_to_c(sline)[0] == '\n')) ||
		((s_curlen(sline) == 2) && (s_to_c(sline)[0] == '\r') && (s_to_c(sline)[1] == '\n')))
		{
		emptyline = 1;
		break;
		}

	    /* is this a header? */
	    hdrtyp = lookupheader(sline);
	    if (hdrtyp <= 0)
		break;

	    /* continuation lines? */
	    for (c = peekc(stdin); (c == ' ') || (c == '\t'); c = peekc(stdin))
		haveline = s_read_line(stdin, sline);

	    if (hdrtyp == H_RVERS)
		{
		/* Are we dealing with a delivery report? */
		/* ret_on_error = 0 ==> do not return on failure */
		msg.ret_on_error = 0;
		Dout(pn, 0, "Report-Version Header seen: ret_on_error = 0\n");
		}

	    hdrbinflag = istext((unsigned char*)s_to_c(sline), s_curlen(sline), C_Text);

	    if (flgm && (hdrtyp == H_MTYPE))
		{
		/* suppress the header if there is a message-type argument */
		}

	    /* do we have to convert 8-bit headers to 7-bit? */
	    else if (force_7bit_headers && (hdrbinflag != C_Text))
		switch (hdrtyp)
		    {
		    case H_FROM2:	/* convert to 7-bit, possibly changing type to illegal */
		    case H_TO:
		    case H_CC:
		    case H_BCC:
		    case H_SENDER:
		    case H_REPLYTO:
		    case H_ERRTO: {
			/* for address in header
			      if 8-bit in address part instead of comment part
				  move-to-illegal = yes
			      copy to new header
			*/

			int move_to_illegal = 0;
			r822_address *alist = 0, *ap;
			string *new_header = s_new();
			long flags = r822_SKIP_NAME | r822_LOOSE_CRLF | r822_LOOSE_LWSP | r822_LOOSE_DOMLIT | r822_LOOSE_FNAME;
			const char *colon = strchr(s_to_c(sline), ':') + 1;

			memrepl(s_to_c(sline), s_curlen(sline), '\0', ' ');
			r822_addr_parse(sline, flags, &alist);
			new_header = s_nappend(new_header, s_to_c(sline), colon - s_to_c(sline));
			s_putc(new_header, ' ');

			/* now loop through the addresses, converting each one */
			for (i = 0, ap = alist; ap; ap = ap->next, i++)
			    {
			    if (s_curlen(ap->group_phrase) > 0)
				{
				if (istext((unsigned char*)s_to_c(ap->group_phrase), s_curlen(ap->group_phrase), C_Text) != C_Text)
				    {
				    string *new_group = s_new();
				    encode_header_to_quoted_printable(ap->group_phrase, new_group);
				    new_header = s_nappend(new_header, s_to_c(new_group), s_curlen(new_group));
				    s_free(new_group);
				    move_to_illegal = 1;
				    }
				else
				    new_header = s_nappend(new_header, s_to_c(ap->group_phrase), s_curlen(ap->group_phrase));
				new_header = s_append(new_header, ": ");
				}

			    /* combine the route, local-part and domain-part fields together into a single route string */
			    {
				string *addr = s_new();
				r822_domain *dp = ap->route;

				/* start with the domain routes */
				for ( ; dp; dp = dp->next)
				    {
				    s_putc(addr, '@');
				    addr = s_append(addr, s_to_c(ap->route->dotted));
				    if (dp->next)
					s_putc(addr, ',');
				    else
					s_putc(addr, ':');
				    }

				/* Add to the address the route local_part and domain_part. */
				addr = s_append(addr, s_to_c(ap->local_part));
				if (ap->domain_part)
				    addr = s_xappend(addr, "@", s_to_c(ap->domain_part->dotted), (char*)0);

				/* now copy the address to our header */
				if (istext((unsigned char*)s_to_c(addr), s_curlen(addr), C_Text) != C_Text)
				    {
				    string *new_addr = s_new();
				    encode_header_to_quoted_printable(ap->group_phrase, new_addr);
				    new_header = s_nappend(new_header, s_to_c(new_addr), s_curlen(new_addr));
				    s_free(new_addr);
				    move_to_illegal = 1;

				    }
				else
				    new_header = s_nappend(new_header, s_to_c(addr), s_curlen(addr));
				s_free(addr);
			    }

			    /* make sure an error string exists if there is an error */
			    if ((ap->flags & r822_IS_ERROR) && (s_curlen(ap->error) == 0))
				ap->error = s_append(ap->error, "unknown addressing error");

			    /* recreate the comment */
			    if ((s_curlen(ap->error) > 0) || ap->comment || (s_curlen(ap->name_phrase) > 0))
				{
				r822_subthing *sp;
				string *comment = s_new();
				new_header = s_append(new_header, "(");
				if (s_curlen(ap->error) != 0)
				    comment = s_xappend(comment, "(", s_to_c(ap->error), ") ", (char*)0);
				if (s_curlen(ap->name_phrase) != 0)
				    comment = s_xappend(comment, "(", s_to_c(ap->name_phrase), ") ", (char*)0);
				for (sp=ap->comment; sp; sp=sp->next)
				    comment = s_append(comment, s_to_c(sp->element));
				if (istext((unsigned char*)s_to_c(comment), s_curlen(comment), C_Text) != C_Text)
				    {
				    string *newcomment = s_new();
				    encode_header_to_quoted_printable(comment, newcomment);
				    new_header = s_nappend(new_header, s_to_c(newcomment), s_curlen(newcomment));
				    s_free(newcomment);
				    }
				else
				    new_header = s_nappend(new_header, s_to_c(comment), s_curlen(comment));
				new_header = s_append(new_header, ")");
				s_free(comment);
				}

			    /* end the group phrase */
			    if (s_curlen(ap->group_phrase) > 0)
				{
				new_header = s_append(new_header, "; ");
				}

			    /* because we're expanding the size of the header, */
			    /* split each one onto its own line */
			    if (ap->next)
				{
				s_putc(new_header, '\n');
				s_putc(new_header, ' ');
				}
			    }

			delete_r822_address(alist);

			/*
			    if move-to-orig
				Illegal-T/C/B = T/C/B
			*/
			switch (hdrtyp)
			    {
			    case H_FROM2:	hdrtyp = H_ILL_FROM2; break;
			    case H_TO:		hdrtyp = H_ILL_TO; break;
			    case H_CC:		hdrtyp = H_ILL_CC; break;
			    case H_BCC:		hdrtyp = H_ILL_BCC; break;
			    case H_SENDER:	hdrtyp = H_ILL_SENDER; break;
			    case H_REPLYTO:	hdrtyp = H_ILL_REPLYTO; break;
			    case H_ERRTO:	hdrtyp = H_ILL_ERRTO; break;
			    }

			save_a_txthdr_n(msg.phdrinfo, s_to_c(new_header), s_curlen(new_header), hdrtyp);
			s_free(new_header);
			break;
			}

		    default: {
			string *newline = s_new();
			encode_header_to_quoted_printable(sline, newline);
			save_a_txthdr_n(msg.phdrinfo, s_to_c(newline), s_curlen(newline), hdrtyp);
			s_free(newline);
			}
		    }

	    else
		save_a_txthdr_n(msg.phdrinfo, s_to_c(sline), s_curlen(sline), hdrtyp);

	    if (msg.phdrinfo->fnuhdrtype == 0)
		msg.phdrinfo->fnuhdrtype = hdrtyp;


	    haveline = s_read_line(stdin, s_restart(sline));
	    msgseen = 1;
	    }

	/* blank sline? -- suppress */
	if (emptyline)
	    haveline = s_read_line(stdin, s_restart(sline));
	}

    /* If there are no >From headers, then consider this message local. */
    /*msg.localmessage = (msg.phdrinfo->hdrs[H_FROM1] == 0);*/

    /*
    if (s_to_c(tmpstr)[0])
	save_mta_hdr_n(msg.phdrinfo, s_to_c(tmpstr), s_curlen(tmpstr), H_RECEIVED, (char*)0, 1);
    */

    /* Add our user names to the recipient list and possibly in To: headers. */
    add_to = flgt ||
	(msg.localmessage && (msg.phdrinfo->hdrs[H_TO] == 0) && isyesno(mgetenv("ADD_TO"), Default_True));
    for (i = 1; i < argc; ++i)
	{
	/* Copy to list in mail entry? */
	if (add_to)
	    save_mta_hdr(msg.phdrinfo, argv[i], H_TO, (char*)0, 0);

	/* Add the recipient, but don't check for duplication. */
	add_recip(&msg, argv[i], FALSE, (Recip*)0, FALSE, TRUE, 0, 0, 0);
	}

    /* Force a From:, Date: & Message-ID: header, if desired, on LOCAL MAIL only. */
    if (msg.localmessage)
	{
	if (msg.phdrinfo->hdrs[H_FROM2] == 0 && isyesno(mgetenv("ADD_FROM"), Default_True))
	    {
	    s_restart(tmpstr);
	    tmpstr = s_xappend(tmpstr, my_name, "@", thissys, maildomain(), (char*)0);
	    save_mta_hdr_n(msg.phdrinfo, s_to_c(tmpstr), s_curlen(tmpstr), H_FROM2, (char*)0, 0);
	    }

	if (msg.phdrinfo->hdrs[H_DATE] == 0 && isyesno(mgetenv("ADD_DATE"), Default_True))
	    save_mta_hdr(msg.phdrinfo, RFC822datestring, H_DATE, (char*)0, 0);

	if (msg.phdrinfo->hdrs[H_MESSAGE_ID] == 0 && isyesno(mgetenv("ADD_MESSAGE_ID"), Default_True))
	    {
	    string *msgid = getmessageid(1);
	    save_mta_hdr_n(msg.phdrinfo, s_to_c(msgid), s_curlen(msgid), H_MESSAGE_ID, (char*)0, 0);
	    s_free(msgid);
	    }
	}

    /* Force a local Received header */
    if (msg.phdrinfo->hdrs[H_RECEIVED] == 0 && isyesno(mgetenv("ADD_RECEIVED"), Default_False))
	{
	s_restart(tmpstr);
	tmpstr = s_xappend(tmpstr, " by ", mailsystem(1),
	    maildomain(), "; ", RFC822datestring, (char*)0);
	save_mta_hdr_n(msg.phdrinfo, s_to_c(tmpstr), s_curlen(tmpstr), H_RECEIVED, (char*)0, 1);
	}

    /* determine the return path */
    /*
    if (s_to_c(msg.Rpath)[0] != '\0')
	msg.Rpath = s_append(msg.Rpath, "!");
    msg.Rpath = s_append(msg.Rpath, s_to_c(fromU));
    */
    if(ReturnPath == NULL) {
	if(s_to_c(msg.Rpath)[0] != '\0')
	    msg.Rpath = s_append(msg.Rpath, ":");
	msg.Rpath = s_append(msg.Rpath, s_to_c(fromU));
	}

    msg.orig = s_append(msg.orig, s_to_c(msg.Rpath));
    save_a_hdr_n(msg.phdrinfo, s_to_c(msg.Rpath), s_curlen(msg.Rpath), H_RETURN_PATH, (char*)0);

    /* Decide if this message came from someone who */
    /* shouldn't see errors. */
    if ((cascmp(s_to_c(fromU), "postmaster") == 0) ||
	(cascmp(s_to_c(fromU), "mailer-daemon") == 0) ||
	(cascmp(s_to_c(fromU), "mailer-demon") == 0) ||
	(cascmp(s_to_c(fromU), "uucp") == 0) ||
	(cascmp(s_to_c(fromU), "mmdf") == 0) ||
	(cascmp(s_to_c(fromU), "mail") == 0) ||
	(cascmp(s_to_c(fromU), "smtp") == 0))
	msg.ret_on_error = 0;

    /* push out message type if so requested */
    if (flgm)	/* message-type */
	{
	trimnl(flgm);
	save_mta_hdr(msg.phdrinfo, flgm, H_MTYPE, (char*)0, 0);
	}

    if (!haveline || ((s_curlen(sline) == 2) && ttyf && (strncmp(s_to_c(sline), ".\n", 2) == SAME)) )
	{
	if (!msgseen)
	    {
	    /* no input whatsoever */
	    goto cleanup;
	    }
	else
	    {
	    /*
	     * No body: write content-type and content-length headers
	     * only if explicitly present. (see below....)
	     */
	    s_restart(sline);
	    }
	}

    if (s_curlen(sline) > 0)
	{
	if (debug > 0)
	    {
	    Dout(pn, 0, "header scan complete, readahead %d = \"%*s\"\n", s_curlen(sline), s_curlen(sline), s_to_c(sline));
	    Dout(pn, 0, "beginning body processing\n");
	    }

	/*
	 *	Are we returning mail from a delivery failure of an old-style
	 *	(SVR3.1 or SVR3.0) rmail? If so, we won't return THIS on failure
	 *	[This line should occur as the FIRST non-blank non-header line]
	 */
	if (strncmp("***** UNDELIVERABLE MAIL sent to",s_to_c(sline),32) == SAME)
	    {
	    msg.ret_on_error = 0; /* 0 says do not return on failure */
	    Dout(pn, 0, "found old-style UNDELIVERABLE line. ret_on_error => 0\n");
	    }
	}

    /* scan body of message */
    if (msg.binflag != C_Binary)
	msg.binflag = istext((unsigned char *)s_to_c(sline), s_curlen(sline), msg.binflag);

    (void)fwrite(s_to_c(sline), sizeof(char), s_curlen(sline), msg.tmpfile->tmpf);
    msg.msgsize += s_curlen(sline);

    if (ttyf)
	{
	/* have to read a line at a time so we can check for "." */
	for (;;)
	    {
	    char line[LSIZE];	/* holds a line of a letter */
	    int n = getline (line, sizeof line, stdin);
	    if ((n <= 0) || ((n == 2) && (strcmp(line, ".\n") == SAME)))
		break;
	    if (msg.binflag != C_Binary)
		msg.binflag = istext((unsigned char *)line, n, msg.binflag);

	    (void) fwrite(line, sizeof(char), n, msg.tmpfile->tmpf);
	    msg.msgsize += n;
	    }
	}

    else
	{
	/* read a block at a time */
	for (;;)
	    {
	    char line[LSIZE];
	    int n = fread (line, 1, sizeof line, stdin);
	    if (n <= 0)
		break;
	    if (msg.binflag != C_Binary)
		msg.binflag = istext((unsigned char *)line, n, msg.binflag);

	    (void) fwrite(line, sizeof(char), n, msg.tmpfile->tmpf);
	    msg.msgsize += n;
	    }
	}

    if (ferror(msg.tmpfile->tmpf))
	{
	errmsg(E_TMP,"");
	done(0);
	}

    /* Polish it off. */
    Dout(pn, 0, "body copy complete, msg size=%ld\n", msg.msgsize);

    /* if not MIME and necessary, convert to MIME */
    if (((hptr = msg.phdrinfo->hdrs[H_MIME_VERSION]) == (Hdrs*)NULL) &&
	isyesno(mgetenv("FORCE_MIME"), Default_True))
	{
	if (msg.msgsize > 0)
	    {
	    Hdrs *hcte = msg.phdrinfo->hdrs[H_CTE];
	    Hdrs *hct = msg.phdrinfo->hdrs[H_CTYPE];
	    const char *cte = 0;
	    string *ct = 0;

	    /* save any existing non-MIME-compliant values of content-type and content-transfer-encoding */
	    if (hcte != (Hdrs*)NULL)
		save_mta_hdr_n(msg.phdrinfo, s_to_c(hcte->value), s_curlen(hcte->value), H_NAMEVALUE, "Original-Content-Transfer-Encoding", 0);
	    if (hct != (Hdrs*)NULL)
		save_mta_hdr_n(msg.phdrinfo, s_to_c(hct->value), s_curlen(hct->value), H_NAMEVALUE, "Original-Content-Type", 0);

	    /* do we have to force to 7-bit mime? */
	    if (isyesno(mgetenv("FORCE_7BIT_MIME"), Default_False))
		{
		if (msg.binflag == C_Binary)
		    {
		    cte = "base64";
		    ct = s_copy("application/octet-stream");
		    encode_message_body(&msg, "base64");
		    }
		else if (msg.binflag == C_GText)
		    {
		    cte = "quoted-printable";
		    ct = s_xappend(ct, "text/plain; charset=", mail_get_charset(), (char*)0);
		    encode_message_body(&msg, "quoted-printable");
		    msg.binflag = C_Text;
		    }
		else
		    ct = s_copy("text/plain");
		}

	    else	/* tag content-type and content-transfer-encoding as whatever it is */
		{
		if (msg.binflag == C_Binary)
		    {
		    cte = "binary";
		    ct = s_copy("application/octet-stream");
		    }
		else if (msg.binflag == C_GText)
		    {
		    cte = "8bit";
		    ct = s_xappend(ct, "text/plain; charset=", mail_get_charset(), (char*)0);
		    }
		else
		    ct = s_copy("text/plain");
		}

	    /* change the existing content-transfer-encoding value */
	    if (hcte)
		{
		s_reset(hcte->value);
		if (cte)
		    hcte->value = s_append(hcte->value, cte);
		else
		    hcte->value = s_append(hcte->value, "7bit");
		}

	    /* create a new content-transfer-encoding value */
	    else if (cte)
		save_mta_hdr(msg.phdrinfo, cte, H_CTE, (char*)0, 0);

	    /* change the existing content-type value */
	    if (hct)
		{
		s_reset(hct->value);
		hct->value = s_append(hct->value, s_to_c(ct));
		}

	    /* create a new content-type value */
	    else
		save_mta_hdr_n(msg.phdrinfo, s_to_c(ct), s_curlen(ct), H_CTYPE, (char*)0, 0);
	    s_free(ct);
	    }
	}

    /* Modify or create value of H_CTYPE. */
    if ((hptr = msg.phdrinfo->hdrs[H_CTYPE]) != (Hdrs*)NULL)
	{
	/* If there is nothing associated with the Content-Type header already, */
	/* force it to an appropriate value. */
	if (s_curlen(hptr->value) == 0)
	    {
	    s_free(hptr->value);
	    hptr->value = s_copy((msg.binflag == C_Text)  ? Text :
				 (msg.binflag == C_GText) ? GenericText :
							    Binary);
	    if (!hptr->value)
		{
		errmsg(E_MEM,":407:malloc failed in %s(): %s\n", pn, Strerror(errno));
		done(1);
		}
	    }

	/* We have a Content-Type header already, but it may be wrong. */
	/* If it's already anything OTHER than 'text', don't change it. */
	else if ((msg.binflag != C_Text) && (strcmp(s_to_c(hptr->value), Text) == SAME))
	    {
	    s_free(hptr->value);
	    hptr->value = s_copy((msg.binflag == C_GText) ? GenericText : Binary);
	    if (!hptr->value)
		{
		errmsg(E_MEM,":407:malloc failed in %s(): %s\n", pn, Strerror(errno));
		done(1);
		}
	    }
	}

    /* Create the Content-Type header with an appropriate value. */
    else if (msg.msgsize > 0)
	save_mta_hdr(msg.phdrinfo, (msg.binflag == C_Text)  ? Text :
				   (msg.binflag == C_GText) ? GenericText :
							      Binary, H_CTYPE, (char*)0, 0);

    /* Set 'place-holder' value of content length to true value */
    if ((hptr = msg.phdrinfo->hdrs[H_CLEN]) != (Hdrs*)NULL)
	{
	char buf[30];
	sprintf(buf,"%ld", msg.msgsize);
	s_free(hptr->value);
	hptr->value = s_copy(buf);
	if (!hptr->value)
	    {
	    errmsg(E_MEM,":407:malloc failed in %s(): %s\n", pn, Strerror(errno));
	    done(1);
	    }
	}
    else if (msg.msgsize > 0)
	{
	char buf[30];
	sprintf(buf,"%ld", msg.msgsize);
	save_mta_hdr(msg.phdrinfo, buf, H_CLEN, (char*)0, 0);
	}

    /* If Content-Type == Generic-Text, add locale header */
    if ((msg.binflag == C_GText) && (msg.phdrinfo->hdrs[H_ENCDTYPE] != (Hdrs*)NULL))
	{
	string *locale = s_copy("euc/locale=");
	char *ctype_env = getenv("LC_CTYPE");
	char *lang_env = getenv("LANG");
	locale = s_append(locale, ctype_env ? ctype_env : lang_env ? lang_env : "unknown");
	s_terminate(locale);
	save_mta_hdr_n(msg.phdrinfo, s_to_c(locale), s_curlen(locale), H_ENCDTYPE, (char*)0, 0);
	s_free(locale);
	}

    if (fclose(msg.tmpfile->tmpf) == EOF)
	{
	errmsg(E_TMP,"");
	done(0);
	}

    msg.tmpfile->tmpf = 0;
    if (have_rewrite_function("main", rewritefile))
	{
	Tmpfile *ptmpfile = new_Tmpfile();
	invoke_rewrite("main", &msg, msg.phdrinfo, msg.phdrinfo, rewritefile, ptmpfile);
	if (ptmpfile->tmpf)
	    {
	    del_Tmpfile(msg.tmpfile);
	    msg.tmpfile = ptmpfile;
	    }
	else
	    del_Tmpfile(ptmpfile);
	}
    msg.tmpfile->tmpf = doopen(msg.tmpfile->lettmp, "r+", E_TMP);

    (void) nw_sendlist(&msg);

cleanup:
    fini_Msg(&msg);
    s_free(fromS);
    s_free(fromU);
    s_free(sline);
    (void) setsig(SIGINT, saveint);
    done(0);
    /* NOTREACHED */
}

/*
    save the user and remote-from information from a From_/>From_ header
*/
/*static void save_from_information(pmsg, line, RFC822datestring, pfromU, pfromS, preceived)*/
static void save_from_information(pmsg, line, RFC822datestring, pfromU, pfromS)
Msg *pmsg;
string *line;
char *RFC822datestring;
string **pfromU;
string **pfromS;
/*string **preceived;*/
{
    if (substr(s_to_c(line), "forwarded by") == -1)
	{
	/* If ">From " line, check for '...remote from...' */
	/* and put its information into a Received: header. */
	int rf = substr(s_to_c(line), " remote from ");
	if (rf >= 0)
	    {
	    static string *receivedBy = NULL;
	    string	*receivedString = s_new();
	    string	*fromAddr = s_new();
	    register char *s = s_to_c(line) + rf + 13;

	    /* This is not a local message */
	    pmsg->localmessage = 0;
	    if(receivedBy == NULL) {
		receivedBy = s_new();
		receivedBy =  s_append(receivedBy, thissys);
		receivedBy =  s_append(receivedBy, maildomain());
		}

	    receivedString = s_append(receivedString, "from ");
	    for ( ; *s && *s != '\n' && *s != '\r'; s++)
		s_putc(fromAddr, *s);
	    receivedString = s_append(receivedString, s_to_c(fromAddr));
	    receivedString = s_xappend
		(
		receivedString,
		" by ",
		s_to_c(receivedBy),
		"; ",
		RFC822datestring,
		(char*)0
		);

	    save_mta_hdr_n
		(
		pmsg->phdrinfo,
		s_to_c(receivedString),
		s_curlen(receivedString),
		H_RECEIVED,
		(char*)0,
		1
		);

	    s_reset(receivedBy);
	    receivedBy = s_append(receivedBy, s_to_c(fromAddr));
	    s_free(fromAddr);
	    s_free(receivedString);
	    }

	pickFrom(s_to_c(line), pfromU, pfromS);
	if (ReturnPath == NULL) {
	    if (s_to_c(pmsg->Rpath)[0] != '\0')
		pmsg->Rpath = s_append(pmsg->Rpath, ",@");
	    else
		pmsg->Rpath = s_append(pmsg->Rpath, "@");
	    pmsg->Rpath = s_append(pmsg->Rpath, s_to_c(*pfromS));
	    }
	}
    /*save_a_txthdr_n(pmsg->phdrinfo, s_to_c(line), s_curlen(line), H_FROM1);*/
}

/* determine the type (using the H_ values) of a header */
static int lookupheader(s)
string *s;
{
    register char *p = s_to_c(s), *endp = p + s_curlen(s);

    if (strncmp(s_to_c(s), header[H_FROM].tag, 5) == 0)
	return H_FROM;
    if (strncmp(s_to_c(s), header[H_FROM1].tag, 6) == 0)
	return H_FROM1;

    /*
     * Check if it's a name: value pair.
     * RFC 822 says a header is a series of non-control and
     * non-space characters followed by a :.
     */
    for ( ; p < endp; p++)
	{
	/* can't be a header; no ":" before the white space */
	if (Iscntrl(*p) || Isspace(*p))
	    return 0;

	if (*p == ':')
	    {
	    register int i;
	    for (i = H_MVERS; i < H_CONT; i++)
		if (casncmp(s_to_c(s), header[i].tag, p - s_to_c(s) + 1) == 0)
		    return i;
	    return H_NAMEVALUE;
	    }
	}

    /* hit end of string with no ":", so can't be a header */
    return 0;
}

/*
    convert a header into quoted-printable form. For example,
	Xyz: foo au!byz bar
    where : represents a non-ASCII character, becomes
	Xyz: foo ?=charset?Q?au=CDbyz?= bar
*/
static void encode_header_to_quoted_printable(in, out)
string *in;
string *out;
{
    const char *p = s_to_c(in);
    const char *endp = p + s_curlen(in);
    const char *cs = mail_get_charset();
    const char *linestart = s_ptr_to_c(out);

    /* copy everything up to the ":" */
    while (p < endp)
	{
	s_putc(out, *p);
	if (*++p == ':')
	    break;
	}

    /* now split apart on sp,\t,\n, encode each piece separately, and put into out */
    while (p < endp)
	{
	/* copy any white space */
	for ( ; p < endp && ((*p == ' ') || (*p == '\t') || (*p == '\n')); p++)
	    s_putc(out, *p);

	if (p < endp)
	    {
	    string *n = s_new();

	    /* split long lines */
	    if ((s_ptr_to_c(out) - linestart) >= 72)
		{
		s_putc(out, '\n');
		s_putc(out, ' ');
		linestart = s_ptr_to_c(out);
		}

	    /* copy any non-white space into temp area */
	    for ( ; p < endp && ((*p != ' ') && (*p != '\t') && (*p != '\n')); p++)
		{
		s_putc(n, *p);
		}
	    s_terminate(n);

	    /* if it's a binary segment, encode it */
	    if (istext((unsigned char*)s_to_c(n), s_curlen(n), C_Text) != C_Text)
		{
		s_putc(out, '?');
		s_putc(out, '=');
		out = s_append(out, cs);
		s_append(out, "?Q?");
		encode_string_to_quoted_printable(n, out, 1);
		s_putc(out, '?');
		s_putc(out, '=');
		}

	    /* otherwise copy it straight */
	    else
		s_nappend(out, s_to_c(n), s_curlen(n));
	    s_free(n);
	    }
	}

    s_terminate(out);
}

static void memrepl(p, len, oldc, newc)
register char *p;
register int len;
register int oldc;
register int newc;
{
    for ( ; len-- > 0; p++)
	if (*p == oldc)
	    *p = newc;
}
