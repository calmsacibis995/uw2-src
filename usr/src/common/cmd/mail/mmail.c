/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/mmail.c	1.2.2.2"
#ident "@(#)mmail.c	1.5 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	use_metamail - return true if metamail is to be called
	send_to_metamail - write the message to metmail

    SYNOPSIS
	int use_metamail(Letinfo *pletinfo, int letnum)
	void send_to_metamail(Letinfo *pletinfo, int letnum, int ttyf)

    DESCRIPTION
	use_metamail() checks to see if Mime-Version: is present and if
	$NOMETAMAIL is not set. If so, check Content-Type:. If it is
	not text/plain or charset is present and not us-ascii, then
	metamail should be called.

	send_to_metamail() copies the message to a temp file, using copylet(),
	and then invokes metamail on the file.
*/

extern const char *mail_get_charset();

int use_metamail(pletinfo, letnum)
Letinfo *pletinfo;
int letnum;
{
    const char *content_type, *tmp, *charset;
    int charset_len;

    /* if $NOMETAMAIL, skip everything */
    if (nometamail)
	return 0;

    /* if no Mime-Version: header, skip everything */
    if (!pletinfo->let[letnum].has_mime_version)
	return 0;

    /* If it doesn't start with text/plain, it's not text. */
    content_type = skipspace(s_to_c(pletinfo->let[letnum].content_type));
    if (casncmp(content_type, "text/plain", 10) != 0)
	return 1;

    charset = mail_get_charset();
    charset_len = strlen(charset);

    /* Found text/plain, now check the charset. */
    /* The charset is always found after a ';'. */
    for (tmp = strchr(content_type, ';'); tmp ? tmp++ : 0; tmp = strchr(tmp, ';'))
	{
	/* skip spaces */
	tmp = skipspace(tmp);
	/* are we looking at "charset"? */
	if (casncmp(tmp, "charset", 7) == 0)
	    {
	    /* find the = and look at the value */
	    content_type = strchr(tmp, '=');
	    if (content_type)
		{
		content_type = skipspace(content_type + 1);
		if (*content_type == '"') content_type++;
		/* check for "us-ascii" */
		if (casncmp(content_type, "us-ascii", 8) == 0)
		    return 0;
		if (casncmp(content_type, charset, charset_len) == 0)
		    return 0;
		}
	    return 1;
	    }
	}

    /* no charset, was text/plain */
    return 0;
}

void send_to_metamail(pletinfo, letnum, ttyf)
Letinfo *pletinfo;
int letnum;
int ttyf;
{
    char *Fname = tmpnam((char*)0);
    FILE *fp;
    string *s;

    if (!Fname)
	{
	pfmt(stderr, MM_ERROR, ":530:Cannot create temp file for MIME message\n");
	return;
	}

    fp = fopen(Fname, "w");
    if (!fp)
	{
	pfmt(stderr, MM_ERROR, ":530:Cannot create temp file for MIME message\n");
	return;
	}

    pfmt(stdout, MM_NOSTD, ":665:Invoking metamail...\n");
    copylet(pletinfo, letnum, fp, ttyf, 1, 1);
    fclose(fp);

    s = s_copy("metamail -m mail ");
    s = s_append(s, Fname);
    s_terminate(s);
    system(s_to_c(s));
    s_free(s);
    unlink(Fname);
    return;
}
