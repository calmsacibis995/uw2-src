/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/mimecomm.c	1.1"
#ident "@(#)mimecomm.c	1.1 'attmail mail(1) command'"
#include "libmail.h"
#include "mimecomm.h"

/* verify that the output has been written correctly */
void checkoutput(outfp)
FILE *outfp;
{
    (void) fflush(outfp);
    if (ferror(outfp))
	{
	(void) pfmt(stderr, MM_ERROR, ":171:Cannot write: %s\n", Strerror(errno));
	exit(1);
	}
}

/* remove everything after the first white spaces in a string */
static void trim_at_space(p)
char *p;
{
    for ( ; *p; p++)
	if ((*p == ' ') || (*p == '\t'))
	    {
	    *p = '\0';
	    break;
	    }
    return;
}

/* extract the first word of the value from a Name: Value header */
string *extract_word(header)
string *header;
{
    int colon_location = is_start_of_a_header(header);
    string *ret = s_copy(skipspace(s_to_c(header) + colon_location + 1));
    s_terminate(ret);
    trimnl(s_to_c(ret));
    trim_at_space(s_to_c(ret));
    return ret;
}

/* initialize an rfc822headers structure */
void rfc822headers_init(this)
rfc822headers *this;
{
    this->headers = this->hdrarray;
    this->curhdrcount = 0;
    this->maxhdrcount = 100;
}

/* deallocate everything associated with an rfc822headers structure */
void rfc822headers_fini(this)
rfc822headers *this;
{
    int i, max = this->curhdrcount;
    for (i = 0; i < max; i++)
	if (this->headers[i])
	    s_free(this->headers[i]);
    if (this->headers != this->hdrarray)
	free(this->headers);
}

/* add a header to the rfc822headers structure */
void rfc822headers_add(this, line)
rfc822headers *this;
string *line;
{
    if (this->curhdrcount == this->maxhdrcount)
	if (this->hdrarray == this->headers)
	    {
	    this->maxhdrcount += 50;
	    this->headers =
		(string**) malloc(sizeof(this->headers[0]) * this->maxhdrcount);
	    (void) memcpy((char*)this->headers, (const char*)this->hdrarray, sizeof(this->hdrarray));
	    }
	else
	    {
	    this->maxhdrcount += 50;
	    this->headers =
		(string**) realloc((char*)this->hdrarray, sizeof(this->headers[0]) * this->maxhdrcount);
	    }

    this->hdrarray[this->curhdrcount++] = line;
}

/* remove everything associated with a header at a given location */
void rfc822headers_zap(this, i)
rfc822headers *this;
int i;
{
    s_free(this->headers[i]);
    this->headers[i] = 0;
}

/* read a line, resetting the string first */
int s_read_new_line(infp, line, eofOK)
FILE *infp;
string *line;
Check_EOF eofOK;
{
    const char *ret;
    (void) s_reset(line);
    ret = s_read_line(infp, line);
    if (!ret)
	switch (eofOK)
	    {
	    case EOF_EndsPart:
		return 0;
	    case EOF_Allowed:
		checkoutput(stdout);
		exit(0);
		/* NOTREACHED */
	    case EOF_Disallowed:
		(void) pfmt(stderr, MM_ERROR, ":33:\t(Unexpected end-of-file).\n");
		exit(1);
	    }
    return 1;
}

/*
    Return an indication of if the line is the beginning of a header block.
    RFC 822 says a header is a series of non-control and
    non-space characters followed by a :. Return 0 or the index
    of the ':'.
*/
int is_start_of_a_header(line)
string *line;
{
    register char *p, *endp;

    if (!isgraph(s_to_c(line)[0]))
	return 0;

    endp = s_to_c(line) + s_curlen(line);
    for (p = s_to_c(line); p < endp; p++)
	{
	if (Iscntrl(*p) || Isspace(*p))
	    return 0;

	if (*p == ':')
	    return p - s_to_c(line);
	}

    return 0;
}

/* return the size of the file open with the FILE* */
long fpsize(fp)
FILE *fp;
{
    struct stat statbuf;
    if (fstat(fileno(fp), &statbuf) == -1)
	return -1;
    return statbuf.st_size;
}

/* append any continuation lines of the header */
void add_continuation_lines(infp, line)
FILE *infp;
string *line;
{
    int c;

    /* RFC822 says a continuation line starts with a blank or tab */
    for (c = peekc(infp); (c == ' ') || (c == '\t'); c = peekc(infp))
	if (!s_read_line(infp, line))
	    break;
}
