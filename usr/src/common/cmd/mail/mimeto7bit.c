/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/mimeto7bit.c	1.4"
#ident "@(#)mimeto7bit.c	1.4 'attmail mail(1) command'"
/*
    NAME
	mimeto7bit - convert MIME message to quoted-printable or base64

    SYNOPSIS
	mimeto7bit < mime-message > encoded-mime-message

    DESCRIPTION
	mimeto7bit is used when you have a MIME message which must be
	converted into a completely 7-bit stream.
*/
#include "libmail.h"
#include "mimecomm.h"

const char *progname = "mimeto7bit";

static void get_message_headers_and_body ARGS((string *line, int require_mime_version, FILE *fpin, FILE *fpout, Check_EOF eofOK));
static void encode_single_body ARGS((const char *new_encoding, FILE *fpin, FILE *fpout));
static void encode_multipart_body ARGS((string *content_type, string *line, FILE *fpin, FILE *fpout));
static const char *new_encoding_type ARGS((string *content_transfer_encoding));

/* print a usage message and exit */
static void usage()
{
    (void) pfmt(stderr, MM_ACTION, ":663:Usage: %s < mime-message > mime-message\n", progname);
    exit(1);
}

int main(argc, argv)
int argc;
char **argv;
{
    int c;
    string *line = s_new();

    (void) setlocale(LC_ALL, "");
    (void) setcat("uxemail");
    (void) setlabel("UX:mimeto7bit");

    /* allow for options to be added in the future */
    while ((c = getopt(argc, argv, "")) != -1)
	switch (c)
	    {
	    default: usage();
	    }

    if (argc != optind)
	usage();

    /* read From headers */
    s_read_new_line(stdin, line, EOF_Allowed);
    while ((strncmp(s_to_c(line), "From ", 5) == 0) || (strncmp(s_to_c(line), ">From ", 6) == 0))
	{
	s_write(line, stdout);
	s_read_new_line(stdin, line, EOF_Allowed);
	}

    /* we have a non->From header, get the rest */
    get_message_headers_and_body(line, 1, stdin, stdout, EOF_Allowed);

    /* All errors in writing are caught here. */
    checkoutput(stdout);
    return 0;
}

/* Get an rfc 822 set of headers and decide what to do with the body. */
/* Precondition: the first line of the message is in line. */
static void get_message_headers_and_body(line, require_mime_version, fpin, fpout, eofOK)
string *line;
int require_mime_version;
FILE *fpin;
FILE *fpout;
Check_EOF eofOK;
{
    string *content_transfer_encoding = 0;
    string *content_type = 0;
    string *mime_version = 0;
    string *cur_header = 0;
    int in_header_block = 1;
    const char *content_type_word = 0;
    const char *new_encoding = 0;

    while (in_header_block)
	{
	/* continuation line? */
	if ((s_to_c(line)[0] == ' ') || (s_to_c(line)[0] == '\t'))
	    {
	    s_write(line, fpout);
	    /* append to current header, if any */
	    if (cur_header)
		cur_header = s_append(cur_header, s_to_c(line));
	    }

	/* beginning of a header? */
	else if (isgraph(s_to_c(line)[0]))
	    {
	    /* remember any special headers we're interested in */
	    /* Content-Type: */
	    if (casncmp(s_to_c(line), "content-type:", 13) == 0)
		{
		/* only keep first content-type header */
		if (!content_type)
		    {
		    content_type = s_dup(line);
		    cur_header = content_type;
		    }
		s_write(line, fpout);
		}

	    /* Content-Transfer-Encoding: */
	    else if (casncmp(s_to_c(line), "content-transfer-encoding:", 26) == 0)
		{
		/* only keep first content-transfer-encoding header */
		if (!content_transfer_encoding)
		    {
		    content_transfer_encoding = s_dup(line);
		    cur_header = content_transfer_encoding;
		    }
		new_encoding = new_encoding_type(content_transfer_encoding);
		if (new_encoding)
		    (void) fprintf(fpout, "Content-Transfer-Encoding: %s\n", new_encoding);
		else
		    s_write(line, fpout);
		}

	    /* Mime-Version: */
	    else if (casncmp(s_to_c(line), "mime-version:", 13) == 0)
		{
		/* only keep first mime-version header */
		if (!mime_version)
		    {
		    mime_version = s_dup(line);
		    cur_header = mime_version;
		    }
		s_write(line, fpout);
		}

	    else
		{
		cur_header = 0;
		s_write(line, fpout);
		}
	    }

	/* end of headers */
	else
	    {
	    in_header_block = 0;
	    if (s_curlen(line) > 0)
		s_write(line, fpout);
	    }

	/* if we're still in the headers, get another line */
	if (in_header_block)
	    s_read_new_line(fpin, line, eofOK);
	}

    /* can we continue? we can't if we're not dealing with MIME messages */
    if (require_mime_version && !mime_version)
	{
	(void) pfmt(stderr, MM_ERROR, ":563:Cannot encode non-MIME messages\n");
	exit(1);
	}

    /* extract the Content-Type: value */
    if (content_type)
	content_type_word = skipspace(s_to_c(content_type) + 13);

    /* are we a simple message? */
    if (!content_type_word ||	/* assume text/plain if no Content-Type: header */
	(casncmp(content_type_word, "text/", 5) == 0) ||
	(casncmp(content_type_word, "audio/", 6) == 0) ||
	(casncmp(content_type_word, "video/", 6) == 0) ||
	(casncmp(content_type_word, "image/", 6) == 0) ||
	(casncmp(content_type_word, "application/", 12) == 0) ||
	(casncmp(content_type_word, "x-", 2) == 0))
	encode_single_body(new_encoding, fpin, fpout);

    /* a multipart message? */
    else if (casncmp(content_type_word, "multipart/", 10) == 0)
	encode_multipart_body(content_type, line, fpin, fpout);

    /* a recursive message? */
    else if (casncmp(content_type_word, "message/", 8) == 0)
	{
	s_read_new_line(fpin, line, eofOK);
	get_message_headers_and_body(line, 0, fpin, fpout, EOF_EndsPart);
	}

    else
	{
	(void) pfmt(stderr, MM_ERROR, ":564:Unknown MIME content type: %s\n", content_type_word);
	exit(1);
	}

    if (content_transfer_encoding) s_free(content_transfer_encoding);
    if (content_type) s_free(content_type);
    if (mime_version) s_free(mime_version);
}

/* determine the new encoding type for this content-type */
static const char *new_encoding_type(content_transfer_encoding)
string *content_transfer_encoding;
{
    const char *encoding_word = skipspace(s_to_c(content_transfer_encoding) + 26);
    if (casncmp(encoding_word, "binary", 6) == 0)
	return "base64";
    else if (casncmp(encoding_word, "8bit", 4) == 0)
	return "quoted-printable";
    else
	return 0;
}

/* We've found a simple message. Encode it. */
static void encode_single_body(new_encoding, fpin, fpout)
const char *new_encoding;
FILE *fpin;
FILE *fpout;
{
    if (strcmp(new_encoding, "base64") == 0)
	(void) encode_file_to_base64(fpin, fpout, 0);

    else if (strcmp(new_encoding, "quoted-printable") == 0)
	(void) encode_file_to_quoted_printable(fpin, fpout);

    else	/* the remaining encodings are 7-bit compatible */
	(void) copystream(fpin, fpout);
}

/* we've found a multi-part message. Recursively encode the pieces separately. */
static void encode_multipart_body(content_type, line, fpin, fpout)
string *content_type;
string *line;
FILE *fpin;
FILE *fpout;
{
    FILE *holdpart;
    string *boundary1 = 0;
    string *boundary2 = 0;
    string *endboundary = 0;
    string *anotherline = s_new();
    const char *boundarypart = 0;
    char *p = s_to_c(content_type) + 13;

    /* determine the boundary word */
    for (p += strcspn(p, "Bb"); *p; p += strcspn(p, "Bb"))
	{
	if (casncmp(p, "boundary=", 9) == 0)
	    {
	    p += 9;
	    if (*p == '"')
		boundarypart = strtok(p, "\"");
	    else
		boundarypart = strtok(p, " \t\n\r");
	    break;
	    }
	}

    if (!boundarypart)
	{
	(void) pfmt(stderr, MM_ERROR, ":594:MIME multipart message requires a boundary specification\n");
	exit(1);
	}

    boundary1 = s_xappend((string*)0, "--", boundarypart, '\n', (char*)0);
    boundary2 = s_xappend((string*)0, "--", boundarypart, '\r', (char*)0);
    endboundary = s_xappend((string*)0, "--", boundarypart, "--", (char*)0);

    /* a multipart consists of
	stuff before a boundary
	--boundary
	message_header_and_body
	--boundary
	message_header_and_body
	--boundary--
	trailer
    */

    /* copy stuff before boundary */
    for (;;)
	{
	s_read_new_line(fpin, line, EOF_Disallowed);
	if ((strncmp(s_to_c(line), s_to_c(boundary1), s_curlen(boundary1)) == 0) ||
	    (strncmp(s_to_c(line), s_to_c(boundary2), s_curlen(boundary2)) == 0))
	    /* found boundary marker */
	    break;
	else
	    s_write(line, fpout);
	}

    /* Loop until we hit the end boundary. For each part found, create a */
    /* temp file with it and recursively treat the part as a message. */
    for (;;)
	{
	int havenewline;

	/* write the boundary marker */
	s_write(line, fpout);

	/* is it the end of the multipart? */
	if (strncmp(s_to_c(line), s_to_c(endboundary), s_curlen(endboundary)) == 0)
	    break;

	/* create a temporary file to hold the part */
	holdpart = xtmpfile();
	if (!holdpart)
	    {
	    (void) pfmt(stderr, MM_ERROR, ":530:Cannot create temp file for MIME message\n");
	    exit(1);
	    }

	/*
	    copy the part to the temp file. Note that the boundary marker is always preceded by
	    a newline, which is NOT part of the part. So we have to do our writes without
	    the newlines, and delay the newline writing until the subsequent iteration.
	*/
	havenewline = 0;
	for (;;)
	    {
	    s_read_new_line(fpin, line, EOF_Disallowed);
	    /* end of the part? */
	    if ((s_to_c(line)[0] == '-') &&
		((strncmp(s_to_c(line), s_to_c(boundary1), s_curlen(boundary1)) == 0) ||
		 (strncmp(s_to_c(line), s_to_c(boundary2), s_curlen(boundary2)) == 0) ||
		 (strncmp(s_to_c(line), s_to_c(endboundary), s_curlen(endboundary)) == 0)))
		break;

	    /* restore the newline to where it belongs */
	    if (havenewline)
		putc('\n', holdpart);

	    /* zap any new trailing newline */
	    if (s_to_c(line)[s_curlen(line)-1] == '\n')
		{
		havenewline = 1;
		s_skipback(line);
		}
	    else
		havenewline = 0;
	    s_write(line, holdpart);
	    }

	checkoutput(holdpart);

	/* encode the part. treate it as an embedded message */
	rewind(holdpart);
	s_read_new_line(holdpart, anotherline, EOF_EndsPart);
	get_message_headers_and_body(anotherline, 0, holdpart, fpout, EOF_EndsPart);
	fclose(holdpart);

	/* put out the newline before the marker */
	if (havenewline)
	    putc('\n', fpout);
	}

    /* copy trailer */
    copystream(fpin, fpout);

    s_free(anotherline);
    s_free(boundary1);
    s_free(boundary2);
    s_free(endboundary);
}
