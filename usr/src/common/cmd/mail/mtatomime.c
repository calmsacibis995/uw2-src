/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/mtatomime.c	1.3"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)mtatomime.c	1.2 'attmail mail(1) command'"
/*
    NAME
	mtatomime - convert MTA format message to MIME format

    SYNOPSIS
	mtatomime < mta-message > mime-message

    DESCRIPTION
	mtatomime is used when you have an AT&T MTA message which must be
	converted into a MIME message.
*/
#define	TRUE 1
#include "libmail.h"
#include "mimecomm.h"

const char *progname = "mtatomime";

typedef enum Rm_mta { keep_mta, rm_mta } Rm_mta;
typedef enum Pass { first_time_through, recursive_times_through } Pass;

static void convert ARGS((string *line, FILE *infp, FILE *outfp, Pass first_time, Rm_mta rm_hdrs));
static string *getboundary ARGS((void));
static int atob ARGS((FILE *infp, FILE *ofp));
static void atob_byteout ARGS((long c, FILE *ofp));
static int atob_decode ARGS((long c, FILE *ofp));

static const char *mappingfile = "/usr/lib/mail/mtatomime.map";

#ifdef TRUE /*JALE*/
static char *CharacterSet = NULL;
#endif

/* print a usage message and exit */
static void usage()
{
    (void) pfmt(stderr, MM_ACTION, ":648:Usage: %s [-m mapping-file] [-r] < mta-message > mime-message\n", progname);
    (void) pfmt(stderr, MM_NOSTD, ":669:-m\tmappings between mta and mime content-types\n");
    (void) pfmt(stderr, MM_NOSTD, ":649:-r\tremove mta-specific headers\n");
    exit(1);
}

main(argc, argv)
int argc;
char **argv;
{
    Rm_mta rm_hdrs = keep_mta;
    int c;
    string *line = s_new();

    (void) setlocale(LC_ALL, "");
    (void) setcat("uxemail");
    (void) setlabel("UX:mtatomime");

    while ((c = getopt(argc, argv, "m:r")) != -1)
	switch (c)
	    {
	    case 'm': mappingfile = optarg; break;
	    case 'r': rm_hdrs = rm_mta; break;
	    default: usage();
	    }

    if (argc != optind)
	usage();

#ifdef TRUE /*JALE*/
    {
    char *sdum;
    CharacterSet = getenv("MM_CHARSET");
    if(!CharacterSet) CharacterSet = "us-ascii";
    for (sdum = CharacterSet; *sdum; ++sdum)
	if (isupper(*sdum)) *sdum = tolower(*sdum);
    }
#endif

    /* read From headers */
    s_read_new_line(stdin, line, EOF_Allowed);
    while ((strncmp(s_to_c(line), "From ", 5) == 0) || (strncmp(s_to_c(line), ">From ", 6) == 0))
	{
	s_write(line, stdout);
	s_read_new_line(stdin, line, EOF_Allowed);
	}

    (void) printf("Mime-Version: 1.0\n");
    convert(line, stdin, stdout, first_time_through, rm_hdrs);
    checkoutput(stdout);
    return 0;
}

/* write out a set of headers. */
static void write_headers_and_newline(headers, outfp)
rfc822headers *headers;
FILE *outfp;
{
    register int i = 0, max = rfc822headers_current_count(headers);
    for ( ; i < max; i++)
        {
	string *hdr = rfc822headers_index(headers, i);
	if (hdr)
	    s_write(hdr, outfp);
	}
    putc('\n', outfp);
}

static char *mta_skip_headers[] =
    {
    ">To:",
    "Auto-Forwarded-Count:",
    "Content-Type:",
    "Email-Version:",
    "End-of-Header:",
    "End-of-Protocol:",
    "MTS-Message-ID:",
    "Message-Service:",
    "Message-Version:",
    "UA-Content-ID:",
    "UA-Message-ID:"
    };

/* a function for use by bsearch to search for a keyword */
static int mta_hdr_cmp(v1, v2)
const VOID *v1;
const VOID *v2;
{
    return cascmp((const char*)v1, (const char*)v2);
}

/* read a message on infp and convert it to MIME format */
static void convert(line, infp, outfp, first_time, rm_hdrs)
string *line;
FILE *infp, *outfp;
Pass first_time;
Rm_mta rm_hdrs;
{
    rfc822headers hdrs;
    static char content_type_string[] = "content-type:";
    static char content_length_string[] = "content-length:";
    static char encoding_algorithm_string[] = "encoding-algorithm:";
    string *content_length = 0;
    string *encoding_algorithm = 0;
    string *content_type = 0;
    int where_is_content_length = -1;
    int where_is_content_type = -1;
    int where_is_encoding_algorithm = -1;
    int content_length_count = 0;
    string *content_type_word;
    int i;
    FILE *tmpfp;

    rfc822headers_init(&hdrs);

    /* while reading headers */
    while (is_start_of_a_header(line))
	{
	add_continuation_lines(infp, line);
	rfc822headers_add(&hdrs, line);

	/* remember any special headers we're interested in */
	/* Content-Type: */
	if (casncmp(s_to_c(line), content_type_string, sizeof(content_type_string)-1) == 0)
	    {
	    /* only keep first content-type header */
	    if (!content_type)
		{
		content_type = line;
		where_is_content_type = rfc822headers_current_count(&hdrs) - 1;
		}
	    }

	/* Content-Length: */
	else if (casncmp(s_to_c(line), content_length_string, sizeof(content_length_string)-1) == 0)
	    {
	    /* only keep first content-length header */
	    if (!content_length)
		{
		content_length = line;
		where_is_content_length = rfc822headers_current_count(&hdrs) - 1;
		}
	    }

	/* Encoding-Algorithm: */
	else if (casncmp(s_to_c(line), encoding_algorithm_string, sizeof(encoding_algorithm_string)-1) == 0)
	    {
	    /* only keep first content-encoding header */
	    if (!encoding_algorithm)
		{
		encoding_algorithm = line;
		where_is_encoding_algorithm = rfc822headers_current_count(&hdrs) - 1;
		}
	    }

	line = s_new();
	s_read_new_line(infp, line, EOF_EndsPart);
	}

    /* can we continue? we can't if we're not dealing */
    /* with MTA messages which have Content-Length headers */
    if (!content_length)
	{
	(void) pfmt(stderr, MM_ERROR, ":650:No Content-Length header. Cannot encode non-MTA messages.\n");
	exit(1);
	}

    /* strip mta hdrs */
    if (rm_hdrs == rm_mta)
        {
	int max = rfc822headers_current_count(&hdrs);
	for (i = 0; i < max; i++)
	    {
	    string *str = rfc822headers_index(&hdrs, i);
	    if (bsearch(s_to_c(str), (char*)mta_skip_headers, nelements(mta_skip_headers), sizeof(mta_skip_headers[0]), mta_hdr_cmp))
		{
		rfc822headers_zap(&hdrs, i);
		}
	    }
	}	

    /* zap content-length header */
    content_length_count = atoi(s_to_c(content_length)+15);
    rfc822headers_zap(&hdrs, where_is_content_length);
    content_length = 0;

    tmpfp = xtmpfile();
    if (!tmpfp)
	{
	(void) pfmt(stderr, MM_ERROR, ":530:Cannot create temp file for MIME message\n");
	exit(1);
	}

    /* skip blank line between headers and body */
    if (!s_isblankline(line))
	{
	s_write(line, tmpfp);
	content_length_count -= s_curlen(line);
	}

    /* copy content-length bytes to tmp file */
    copynstream(infp, tmpfp, content_length_count);
    checkoutput(tmpfp);

    rewind(tmpfp);

    /* check for an encoded message */
    if (encoding_algorithm)
	{
	string *algorithm = extract_word(encoding_algorithm);
	if (cascmp(s_to_c(algorithm), "btoa") == 0)
	    {
	    FILE *newfp = xtmpfile();
	    rewind(tmpfp);
	    if (atob(tmpfp, newfp))
		{
		checkoutput(newfp);
		fclose(tmpfp);
		tmpfp = newfp;
		rewind(newfp);
		rfc822headers_zap(&hdrs, where_is_encoding_algorithm);
		encoding_algorithm = 0;
		}

	    else
		{
		(void) pfmt(stderr, MM_ERROR, ":651:Cannot decode atob segment\n");
		fclose(newfp);
		}
	    }

	else
	    {
	    (void) pfmt(stderr, MM_ERROR, ":652:Unknown MTA encoding: %s\n", s_to_c(algorithm));
	    exit(1);
	    }

	s_free(algorithm);
	}

    /* extract the type */
    content_type_word = extract_word(content_type);

    /* multipart is made up of sub-messages */
    if (cascmp(s_to_c(content_type_word), "multipart") == 0)
	{
	string *boundary = getboundary();

	/* change the Content-Type header */
	rfc822headers_zap(&hdrs, where_is_content_type);
	content_type = 0;
	rfc822headers_add(&hdrs, s_xappend((string*)0, "Content-Type: multipart/mixed; boundary=\"", s_to_c(boundary), "\"\n", (char*)0));

	/* copy headers to outfp, skipping over zapped headers */
	write_headers_and_newline(&hdrs, outfp);

	/* let people know we're using MIME */
	if (first_time == first_time_through)
	    {
	    (void) fprintf (outfp, "> THIS IS A MESSAGE IN 'MIME' FORMAT.  Your mail reader does not support MIME.\n");
	    (void) fprintf (outfp, "> Some parts of this will be readable as plain text.\n");
	    (void) fprintf (outfp, "> To see the rest, you will need to upgrade your mail reader.\n");
	    }

	/* convert all of the parts */
	while (!feof(infp))
	    {
	    if (!s_read_new_line(tmpfp, line, EOF_EndsPart))
		break;
	    if (s_isblankline(line))
		if (!s_read_new_line(tmpfp, line, EOF_EndsPart))
		    break;
	    (void) fprintf (outfp, "\n--%s\n", s_to_c(boundary));
	    convert(line, tmpfp, outfp, recursive_times_through, rm_hdrs);
	    line = s_new();
	    }

	(void) fprintf (outfp, "\n--%s--\n", s_to_c(boundary));
	}

    /* other types are a single part */
    else if (cascmp(s_to_c(content_type_word), "text") == 0)
	{
	/* change the Content-Type header */
	content_type = 0;
	rfc822headers_zap(&hdrs, where_is_content_type);
	rfc822headers_add(&hdrs, s_copy("Content-Type: text/plain\n"));

	/* copy headers to outfp, skipping over zapped headers */
	write_headers_and_newline(&hdrs, outfp);

	copystream(tmpfp, outfp);
	}

    else if (cascmp(s_to_c(content_type_word), "generic-text") == 0)
	{
	/* change the Content-Type header */
	content_type = 0;
	rfc822headers_zap(&hdrs, where_is_content_type);
#ifndef TRUE /*JALE*/
	rfc822headers_add(&hdrs, s_copy("Content-Type: text/plain; charset=iso8859-1\n"));
#else  /* JALE */
	{  char strtmp[256]; 
 	   strcpy(strtmp, "Content-Type: text/plain; charset=");
	   strcat(strtmp, CharacterSet);
	   strcat(strtmp, "\n");
	   rfc822headers_add(&hdrs, s_copy(strtmp));
	}
#endif

	/* add a content-transfer-encoding header */
	rfc822headers_add(&hdrs, s_copy("Content-Transfer-Encoding: quoted-printable\n"));

	/* copy headers to outfp, skipping over zapped headers */
	write_headers_and_newline(&hdrs, outfp);

	/* copy the part */
	encode_file_to_quoted_printable(tmpfp, outfp);
	}

    else if (cascmp(s_to_c(content_type_word), "binary") == 0)
	{
	/* change the Content-Type header */
	content_type = 0;
	rfc822headers_zap(&hdrs, where_is_content_type);
	rfc822headers_add(&hdrs, s_copy("Content-Type: application/octet-stream\n"));

	/* add a content-transfer-encoding header */
	rfc822headers_add(&hdrs, s_copy("Content-Transfer-Encoding: base64\n"));

	/* copy headers to outfp, skipping over zapped headers */
	write_headers_and_newline(&hdrs, outfp);

	/* copy the part */
	encode_file_to_base64(tmpfp, outfp, 0);
	}

    else /* Content-Type == other */
	{
	/* change content-type to original-content-type and */
	/* add a content-transfer-encoding header */
	rfc822headers_add(&hdrs, s_copy("Content-Transfer-Encoding: base64\n"));
	rfc822headers_add(&hdrs, s_nappend(s_copy("Original-"), s_to_c(content_type), s_curlen(content_type)));

	/* change the Content-Type header */
	content_type = 0;
	rfc822headers_zap(&hdrs, where_is_content_type);
	rfc822headers_add(&hdrs, s_copy("Content-Type: application/octet-stream\n"));

	/* copy headers to outfp, skipping over zapped headers */
	write_headers_and_newline(&hdrs, outfp);

	/* copy the part */
	encode_file_to_base64(tmpfp, outfp, 0);
	}

    fclose(tmpfp);
    checkoutput(outfp);
    s_free(content_type_word);
    rfc822headers_fini(&hdrs);
}

/* generate a MIME boundary string */
static string *getboundary()
{
    static long thiscall = 0;
    const char *cluster = mailsystem(0);
    const char *nodename = mailsystem(1);
    const char *domain = maildomain();
    char msgid[40];

    sprintf(msgid, "_%lx.%lx.%lx@", (long)time((long*)0), (long)getpid(), thiscall++);

    if (strcmp(cluster, nodename) == 0)
	return s_xappend((string*)0, msgid, nodename, ".", domain, "=_", (char*)0);
    else
	return s_xappend((string*)0, msgid, nodename, ".", cluster, ".", domain, "=_", (char*)0);
}

/* variables used by atob/btoa encoding */
static unsigned long	Ceor;
static unsigned long	Csum;
static unsigned long	Crot;
static long		bcount;
static long		word;

#define DE(c)	((c) - ' ')

/* decode a btoa'd stream */
static int atob(infp, ofp)
FILE *infp;
FILE *ofp;
{
    register long c;
    string *line = s_new();
    unsigned long n1, n2, oeor, osum, orot;

    /* look for beginning marker */

    while (s_read_new_line(infp, line, EOF_EndsPart))
	{
	s_terminate(line);
	trimnl(s_to_c(line));
	if (strcmp(s_to_c(line), "xbtoa Begin") == 0)
	    break;
	}

    if (s_curlen(line) == 0)
	{
	s_free(line);
	(void) pfmt(stderr, MM_ERROR, ":653:Error decoding atob segment: No xbtoa header\n");
	return 0;
	}

    /* get the body of the encoded data */
    Ceor = Csum = Crot = bcount = word = 0L;

    while ((c = getc(infp)) != EOF)
	{
	if (c == '\n' || c == '\r')
	    continue;
	if (c == 'x')
	    break;
	if (!atob_decode(c, ofp))
	    {
	    s_free(line);
	    (void) pfmt(stderr, MM_ERROR, ":654:Error decoding atob segment: premature zero word\n");
	    return 0;
	    }
	}

    /* look for the ending marker (already found 'x') */
    if (!s_read_new_line(infp, line, EOF_EndsPart))
	{
	s_free(line);
	(void) pfmt(stderr, MM_ERROR, ":655:Error decoding atob segment: premature end of file\n");
	return 0;
	}

    s_terminate(line);

    if (sscanf(s_to_c(line), "btoa End N %ld %lx E %lx S %lx R %lx", &n1, &n2, &oeor, &osum, &orot) != 5)
	{
	s_free(line);
	(void) pfmt(stderr, MM_ERROR, ":656:Error decoding atob sgement: malformed trailer\n");
	return 0;
	}

    s_free(line);

    if ((n1 != n2) || (oeor != Ceor) || (osum != Csum) || (orot != Crot))
	{
	if (n1 != n2)
	    (void) pfmt(stderr, MM_ERROR, ":657:Error decoding atob segment: file sizes do not match!\n");
	if (oeor != Ceor)
	    (void) pfmt(stderr, MM_ERROR, ":658:Error decoding atob segment: Exclusive-Or checksums do not match!\n");
	if (osum != Csum)
	    (void) pfmt(stderr, MM_ERROR, ":659:Error decoding atob segment: Summation checksums do not match!\n");
	if (orot != Crot)
	    (void) pfmt(stderr, MM_ERROR, ":660:Error decoding atob segment: Rotation checksums do not match!\n");
	return 0;
	}

    return 1;
}

/* decode an atob character */
static int atob_decode(c, ofp)
register long c;
FILE *ofp;
{
    if (c == (unsigned long) 'z')
	{
	if (bcount != 0)
	    return 0;
	atob_byteout(0L, ofp);
	atob_byteout(0L, ofp);
	atob_byteout(0L, ofp);
	atob_byteout(0L, ofp);
	}
    else if ((c >= (unsigned long) ' ') && (c < (unsigned long) (' ' + 85)))
	{
	if (bcount == 0)
	    {
	    word = DE(c);
	    ++bcount;
	    }
	else if (bcount < 4)
	    {
	    word *= 85L;
	    word += DE(c);
	    ++bcount;
	    }
	else
	    {
	    word = ((unsigned long) word * (unsigned long) 85) + DE(c);
	    atob_byteout((word >> 24) & 255L, ofp);
	    atob_byteout((word >> 16) & 255L, ofp);
	    atob_byteout((word >> 8) & 255L, ofp);
	    atob_byteout(word & 255L, ofp);
	    word = 0;
	    bcount = 0;
	    }
	}
    else
	return 0;

    return 1;
}

/* output a decoded atob character */
static void atob_byteout(c, ofp)
register long c;
FILE *ofp;
{
    Ceor ^= c;
    Csum += c;
    Csum += 1;
    if (Crot & 0x80000000L)
	{
	Crot <<= 1;
	Crot += 1;
	}
    else
	{
	Crot <<= 1;
	}
    Crot += c;
    putc((int)c, ofp);
}
