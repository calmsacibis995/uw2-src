/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/mimecomm.h	1.1"
#ident "@(#)mimecomm.h	1.2 'attmail mail(1) command'"
#ifndef MIMECOMM_H
#define MIMECOMM_H

/* These functions are common to the mime conversion programs. */

typedef enum Check_EOF { EOF_Disallowed, EOF_Allowed, EOF_EndsPart } Check_EOF;

void add_continuation_lines ARGS((FILE *fpin, string *line));
void checkoutput ARGS((FILE *fpout));
string *extract_word ARGS((string *header));
long fpsize ARGS((FILE *fp));
int is_start_of_a_header ARGS((string *line));
int s_read_new_line ARGS((FILE *infp, string *line, Check_EOF eofOK));
void s_write ARGS((string *line, FILE *fpout));

typedef struct rfc822headers
{
    string *hdrarray[100];
    string **headers;
    int curhdrcount;
    int maxhdrcount;
} rfc822headers;

void rfc822headers_init ARGS((rfc822headers *this));
void rfc822headers_fini ARGS((rfc822headers *this));
void rfc822headers_add ARGS((rfc822headers *this, string *line));
void rfc822headers_zap ARGS((rfc822headers *this, int i));

/* return the current count of headers in the rfc822headers structure */
#ifdef lint
int rfc822headers_current_count ARGS((rfc822headers *this));
#else
# define rfc822headers_current_count(this) ((this)->curhdrcount)
#endif

/* return the i'th element of the rfc822headers structure */
#ifdef lint
string *rfc822headers_index ARGS((rfc822headers *this, int i));
#else
# define rfc822headers_index(this, i) ((this)->headers[i])
#endif

#define s_isblankline(line) ((s_curlen(line) == 1 && s_to_c(line)[0] == '\n') || \
	  (s_curlen(line) == 1 && s_to_c(line)[0] == '\r' && s_to_c(line)[1] == '\n'))
#endif
