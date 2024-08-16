/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/mimetomta.c	1.4"
#ident "@(#)mimetomta.c	1.3 'attmail mail(1) command'"
/*
    NAME
	mimetomta - convert MIME message to MTA format

    SYNOPSIS
	mimetomta < mime-message > mta-message

    DESCRIPTION
	mimetomta is used when you have a MIME message which must be
	converted into an AT&T MTA message.
*/
#include "libmail.h"
#include "mimecomm.h"

const char *progname = "mimetomta";

static void get_message_headers_and_body ARGS((string *line, int require_mime_version, FILE *infp, FILE *outfp, Check_EOF eofOK));
static void convert_single_body ARGS((rfc822headers *phdrs,string *content_transfer_encoding,FILE *infp,FILE *outfp));
static void convert_multipart_body ARGS((rfc822headers *phdrs,string *content_type, string *line, FILE *infp,FILE *outfp));
static void convert_message_body ARGS((rfc822headers *phdrs,FILE *infp,FILE *outfp));
static void decode_quoted_printable_to_file ARGS((FILE *infp, FILE *outfp));
static void decode_base64_to_file ARGS((FILE *infp, FILE *outfp));

static const char *mappingfile = "/usr/lib/mail/mtatomime.map";

/* print a usage message and exit */
static void usage()
{
    (void) pfmt(stderr, MM_ACTION, ":661:Usage: %s [-m mapping-file] < mime-message > mta-message\n", progname);
    (void) pfmt(stderr, MM_NOSTD, ":669:-m\tmappings between mta and mime content-types\n");
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
    (void) setlabel("UX:mimetomta");

    /* allow for options to be added in the future */
    while ((c = getopt(argc, argv, "m:")) != -1)
	switch (c)
	    {
	    case 'm': mappingfile = optarg; break;
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
    get_message_headers_and_body(line, 1, stdin, stdout, 1);

    /* All errors in writing are caught here. */
    checkoutput(stdout);
    return 0;
}

/* Get an rfc 822 set of headers and decide what to do with the body. */
/* Precondition: the first line of the message is in line. */
/* Note: line is s_free()'d in this routine. */
static void get_message_headers_and_body(line, require_mime_version, infp, outfp, eofOK)
string *line;
int require_mime_version;
FILE *infp;
FILE *outfp;
Check_EOF eofOK;
{
    rfc822headers hdrs;
    string *content_transfer_encoding = 0;
    string *content_type = 0;
    string *mime_version = 0;
    string *content_type_word = 0;
    string *message_id = 0;
    string *ua_message_id = 0;

    rfc822headers_init(&hdrs);

    /* while reading headers */
    while (is_start_of_a_header(line))
	{
	add_continuation_lines(infp, line);
	rfc822headers_add(&hdrs, line);

	/* remember any special headers we're interested in */
	/* Content-Type: */
	if (casncmp(s_to_c(line), "content-type:", 13) == 0)
	    {
	    /* only keep first such header */
	    if (!content_type)
		content_type = line;
	    }

	/* Content-Transfer-Encoding: */
	else if (casncmp(s_to_c(line), "content-transfer-encoding:", 26) == 0)
	    {
	    /* only keep first such header */
	    if (!content_transfer_encoding)
		content_transfer_encoding = line;
	    }

	/* Mime-Version: */
	else if (casncmp(s_to_c(line), "mime-version:", 13) == 0)
	    {
	    /* only keep first such header */
	    if (!mime_version)
		mime_version = line;
	    }

        /* Message-ID: */
	else if (casncmp(s_to_c(line), "message-id:", 11) == 0)
	    {
	    /* only keep first such header */
	    if (!mime_version)
		mime_version = line;
	    }

        /* UA-Message-ID: */
	else if (casncmp(s_to_c(line), "ua-message-id:", 14) == 0)
	    {
	    /* only keep first such header */
	    if (!mime_version)
		mime_version = line;
	    }

	line = s_new();
	s_read_new_line(infp, line, eofOK);
	}

    /* can we continue? we can't if we're not dealing with MIME messages */
    if (require_mime_version && !mime_version)
	{
	(void) pfmt(stderr, MM_ERROR, ":563:Cannot encode non-MIME messages\n");
	exit(1);
	}

    /* copy Message-ID into MTA's UA-Message-ID */
    if (!ua_message_id && message_id)
	{
	string *msgid = s_xappend((string*)0, "UA-Message-ID: ", skipspace(s_to_c(message_id) + 11), (char*)0);
	rfc822headers_add(&hdrs, msgid);
	}

    /* extract the Content-Type: value */
    if (content_type)
	content_type_word = extract_word(content_type);

    /* are we a simple message? */
    if (!content_type ||	/* assume text/plain if no Content-Type: header */
	(casncmp(s_to_c(content_type_word), "audio/", 6) == 0) ||
	(casncmp(s_to_c(content_type_word), "video/", 6) == 0) ||
	(casncmp(s_to_c(content_type_word), "image/", 6) == 0) ||
	(casncmp(s_to_c(content_type_word), "application/", 12) == 0) ||
	(casncmp(s_to_c(content_type_word), "x-", 2) == 0))
	convert_single_body(&hdrs, content_transfer_encoding, infp, outfp);

    /* special case on text/plain to handle character set */
    else if (casncmp(s_to_c(content_type_word), "text/", 5) == 0)
	{
	convert_single_body(&hdrs, content_transfer_encoding, infp, outfp);
	}

    /* a multipart message? */
    else if (casncmp(s_to_c(content_type_word), "multipart/", 10) == 0)
	convert_multipart_body(&hdrs, content_type, line, infp, outfp);

    /* a recursive message? */
    else if (casncmp(s_to_c(content_type_word), "message/", 8) == 0)
	convert_message_body(&hdrs, infp, outfp);

    else
	{
	(void) pfmt(stderr, MM_ERROR, ":564:Unknown MIME content type: %s\n", content_type_word);
	exit(1);
	}

    rfc822headers_fini(&hdrs);
}

const char *scantype(infp)
FILE *infp;
{
    unsigned char buf[5120];
    int nread;
    t_Content curcontent = C_Text;

    rewind(infp);
    while ((nread = fread(buf, sizeof(char), sizeof(buf), infp)) > 0)
	if ((curcontent = istext(buf, nread, curcontent)) == C_Binary)
	    return "binary";

    switch (curcontent)
	{
	case C_Text: return "text";
	case C_GText: return "generic-text";
	default:	/* default can't happen */
	case C_Binary: return "binary";
	}
}

/* We've found a simple message. Convert it. */
static void convert_single_body(phdrs, content_transfer_encoding, infp, outfp)
rfc822headers *phdrs;
string *content_transfer_encoding;
FILE *infp;
FILE *outfp;
{
    string *cte_word = content_transfer_encoding ?
		       extract_word(content_transfer_encoding) :
		       s_copy("8bit");
    string *line;
    FILE *tmpfp = xtmpfile();
    int i;

    if (!tmpfp)
	{
	(void) pfmt(stderr, MM_ERROR, ":530:Cannot create temp file for MIME message\n");
	exit(1);
	}

    /* convert encoded text back to straight bytes */
    if (cascmp(s_to_c(cte_word), "base64") == 0)
	decode_base64_to_file(infp, tmpfp);

    else if (strcmp(s_to_c(cte_word), "quoted-printable") == 0)
	decode_quoted_printable_to_file(infp, tmpfp);

    else	/* the remaining encodings are 7-bit compatible */
	(void) copystream(infp, tmpfp);

    fflush(tmpfp);
    if (ferror(tmpfp))
	{
	(void) pfmt(stderr, MM_ERROR, ":530:Cannot create temp file for MIME message\n");
	exit(1);
	}

    /*
	copy headers to outfp
	    leave out Content-Transfer-Encoding, Mime-Version
	    change Content-Type to Original-Content-Type
	    Add Content-Length
	Copy newline
	Copy temp file
    */
    for (i = 0; i < rfc822headers_current_count(phdrs); i++)
	{
	line = rfc822headers_index(phdrs, i);
	if (line)
	    {
	    if (casncmp(s_to_c(line), "content-type:", 13) == 0)
		{
		(void) fprintf (outfp, "Original-");
		s_write(line, outfp);
		}

	    else if ((casncmp(s_to_c(line), "content-transfer-encoding:", 26) != 0) &&
		     (casncmp(s_to_c(line), "mime-version:", 13) != 0))
		s_write(line, outfp);
	    }
	}

    (void) fprintf (outfp, "Content-Length: %ld\n", fpsize(tmpfp));
    (void) fprintf (outfp, "Content-Type: %s\n", scantype(tmpfp));

    putc('\n', outfp);
    rewind(tmpfp);
    copystream(tmpfp, outfp);
    fclose(tmpfp);
    s_free(cte_word);
}

static void convert_multipart_body(phdrs, content_type, line, infp, outfp)
rfc822headers *phdrs;
string *content_type;
string *line;
FILE *infp;
FILE *outfp;
{
    FILE *holdpart;
    string *boundary1 = 0;
    string *boundary2 = 0;
    string *endboundary = 0;
    string *anotherline = s_new();
    const char *boundarypart = 0;
    char *p = s_to_c(content_type) + 13;
    int i;

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

    boundary1 =   s_xappend((string*)0, "--", boundarypart, "\n", (char*)0);
    boundary2 =   s_xappend((string*)0, "--", boundarypart, "\r", (char*)0);
    endboundary = s_xappend((string*)0, "--", boundarypart, "--", (char*)0);

    /*
	Copy headers to outfp
	    leave out Content-Transfer-Encoding, Mime-Version
	    change Content-Type to Original-Content-Type
	    Add Content-Length
	Copy newline
    */
    for (i = 0; i < rfc822headers_current_count(phdrs); i++)
	{
	line = rfc822headers_index(phdrs, i);
	if (line)
	    {
	    if (casncmp(s_to_c(line), "content-type:", 13) == 0)
		{
		(void) fprintf (outfp, "Original-");
		s_write(line, outfp);
		}

	    else if ((casncmp(s_to_c(line), "content-transfer-encoding:", 26) != 0) &&
		     (casncmp(s_to_c(line), "mime-version:", 13) != 0))
		s_write(line, outfp);
	    }
	}

    (void) fprintf (outfp, "Content-Length: %ld\n", fpsize(infp));
    (void) fprintf (outfp, "Content-Type: multipart\n");
    (void) putc('\n', outfp);

    /* a multipart consists of
	stuff before a boundary
	--boundary
	message_header_and_body
	--boundary
	message_header_and_body
	--boundary--
	trailer
    */

    /* skip stuff before boundary */
    for (;;)
	{
	s_read_new_line(infp, line, EOF_Disallowed);
	if ((strncmp(s_to_c(line), s_to_c(boundary1), s_curlen(boundary1)) == 0) ||
	    (strncmp(s_to_c(line), s_to_c(boundary2), s_curlen(boundary2)) == 0))
	    /* found boundary marker */
	    break;
	/* else */
	    /* nothing */ ;
	}

    /* Loop until we hit the end boundary. For each part found, create a */
    /* temp file with it and recursively treat the part as a message. */
    for (;;)
	{
	int havenewline;

	/* skip the boundary marker */
	/* nothing */

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
	    s_read_new_line(infp, line, EOF_Disallowed);
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
	get_message_headers_and_body(anotherline, 0, holdpart, outfp, EOF_EndsPart);
	anotherline = s_new();
	fclose(holdpart);

	/* skip the newline before the marker */
	/* if (havenewline) */
	/*    putc('\n', outfp); */
	}

    /* ignore trailer */
    /* copystream(infp, outfp); */

    s_free(anotherline);
    s_free(boundary1);
    s_free(boundary2);
    s_free(endboundary);
}

static void convert_message_body(phdrs, infp, outfp)
rfc822headers *phdrs;
FILE *infp;
FILE *outfp;
{
    string *line = s_new();
    int i;

    for (i = 0; i < rfc822headers_current_count(phdrs); i++)
	{
	line = rfc822headers_index(phdrs, i);
	if (line)
	    {
	    if (casncmp(s_to_c(line), "content-type:", 13) == 0)
		{
		(void) fprintf (outfp, "Original-");
		s_write(line, outfp);
		}

	    else if ((casncmp(s_to_c(line), "content-transfer-encoding:", 26) != 0) &&
		     (casncmp(s_to_c(line), "mime-version:", 13) != 0))
		s_write(line, outfp);
	    }
	}

    (void) fprintf (outfp, "Content-Length: %ld\n", fpsize(infp));
    (void) fprintf (outfp, "Content-Type: multipart\n");
    (void) putc('\n', outfp);

    s_read_new_line(infp, line, EOF_EndsPart);
    get_message_headers_and_body(line, 0, infp, outfp, EOF_EndsPart);
}

/* convert a stream written in quoted-printable into normal ascii */
static void decode_quoted_printable_to_file(infp, outfp)
FILE *infp;
FILE *outfp;
{
    int c;
    while ((c = getc(infp)) != EOF)
	{
	if (c != '=')
	    putc(c, outfp);

	else
	    {
	    int c1 = getc(infp);
	    int c2;

	    if (c1 == EOF)
		return;

	    c2 = getc(infp);
	    if (c2 == EOF)
		return;

	    /* check for soft CRLF */
	    if (c1 == '\r')
		{
		c2 = getc(infp);
		if (c2 == EOF)
		    return;
		if (c2 != '\n')		/* not a CRLF */
		    ungetc(c2, infp);	/* put back the char after the =<CR> */
		continue;
		}

	    /* check for soft newline */
	    if (c1 == '\n')
		{
		ungetc(c2, infp);	/* put back the char after the newline */
		continue;
		}

	    /* check for == -> = */
	    if (c1 == '=')
		{
		putc(c1, outfp);
		ungetc(c2, infp);	/* put back the char after the == */
		continue;
		}

	    /* make sure it's =XX */
	    if (!Isxdigit(c1) || !Isxdigit(c2))
		continue;

	    /* we have two hex digits, so decode them */
	    if (isdigit(c1))
		c1 -= '0';
	    else if (islower(c1))
		{
		c1 -= 'a';
		c1 += 10;
		}
	    else
		{
		c1 -= 'A';
		c1 += 10;
		}
	    if (isdigit(c2))
		c2 -= '0';
	    else if (islower(c2))
		{
		c2 -= 'a';
		c2 += 10;
		}
	    else
		{
		c2 -= 'A';
		c2 += 10;
		}
	    putc(((c1 << 4) | c2), outfp);
	    }
	}
}

#define UN 255

static unsigned char b64_map[256] =
    {
    UN,UN,UN,UN, UN,UN,UN,UN, UN,UN,UN,UN, UN,UN,UN,UN,	/* 0x00 - 0x0f, NUL - SI  */
    UN,UN,UN,UN, UN,UN,UN,UN, UN,UN,UN,UN, UN,UN,UN,UN,	/* 0x10 - 0x1f, DLE - US  */
    UN,UN,UN,UN, UN,UN,UN,UN, UN,UN,UN,62, UN,UN,UN,63,	/* 0x20 - 0x2f, SP  - /   */
    52,53,54,55, 56,57,58,59, 60,61,UN,UN, UN,UN,UN,UN,	/* 0x30 - 0x3f, 0   - ?   */
    UN, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,	/* 0x40 - 0x4f, @   - O   */
    15,16,17,18, 19,20,21,22, 23,24,25,UN, UN,UN,UN,UN,	/* 0x50 - 0x5f, P   - _   */
    UN,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,	/* 0x60 - 0x6f, `   - o   */
    41,42,43,44, 45,46,47,48, 49,50,51,UN, UN,UN,UN,UN,	/* 0x70 - 0x7f, p   - DEL */

    UN,UN,UN,UN, UN,UN,UN,UN, UN,UN,UN,UN, UN,UN,UN,UN,	/* 0x80 - 0x8f */
    UN,UN,UN,UN, UN,UN,UN,UN, UN,UN,UN,UN, UN,UN,UN,UN,	/* 0x90 - 0x9f */
    UN,UN,UN,UN, UN,UN,UN,UN, UN,UN,UN,UN, UN,UN,UN,UN,	/* 0xA0 - 0xAf */
    UN,UN,UN,UN, UN,UN,UN,UN, UN,UN,UN,UN, UN,UN,UN,UN,	/* 0xB0 - 0xBf */
    UN,UN,UN,UN, UN,UN,UN,UN, UN,UN,UN,UN, UN,UN,UN,UN,	/* 0xC0 - 0xCf */
    UN,UN,UN,UN, UN,UN,UN,UN, UN,UN,UN,UN, UN,UN,UN,UN,	/* 0xD0 - 0xDf */
    UN,UN,UN,UN, UN,UN,UN,UN, UN,UN,UN,UN, UN,UN,UN,UN,	/* 0xE0 - 0xEf */
    UN,UN,UN,UN, UN,UN,UN,UN, UN,UN,UN,UN, UN,UN,UN,UN,	/* 0xF0 - 0xFf */
};

static int nextgetc(infp)
FILE *infp;
{
    int c;
    while ((c = getc(infp)) != EOF)
	if (b64_map[c] != UN)
	    break;
    return c;
}

static void decode_base64_to_file(infp, outfp)
FILE *infp;
FILE *outfp;
{
    int c1, c2, c3, c4;

    for (;;)
	{
	c1 = nextgetc(infp);
	c2 = nextgetc(infp);
	c3 = nextgetc(infp);
	c4 = nextgetc(infp);
	if ((c1 == '=') || (c2 == '=') || (c1 == EOF) || (c2 == EOF))
	    break;
	putc((((unsigned int)b64_map[c1]<<2) | (((unsigned int)b64_map[c2]&0x30)>>4)), outfp);
	if ((c3 == '=') || (c3 == EOF))
	    break;
	putc(((((unsigned int)b64_map[c2]&0XF) << 4) | (((unsigned int)b64_map[c3]&0x3C) >> 2)), outfp);
	if ((c4 == '=') || (c4 == EOF))
	    break;
	putc(((((unsigned int)b64_map[c3]&0x03) <<6) | (unsigned int)b64_map[c4]), outfp);
	}
}
