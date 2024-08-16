/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libmail:libmail/s_string.c	1.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)s_string.c	1.16 'attmail mail(1) command'"
#include "stdc.h"
#include "libmail.h"

/* global to this file */
#define STRLEN 128
#define STRALLOC 128
#define MAXINCR 250000

/* buffer pool for allocating string structures */
typedef struct {
	string s[STRALLOC];
	int o;
} stralloc;
static stralloc *freep=NULL;

/* pool of freed strings */
static string *freed=NULL;

static	char	MAmallocating[]  = ":156:Allocating string: %s\n";

void
s_free(sp)
	string *sp;
{
	if (sp != NULL) {
#ifdef DEBUG_USEMALLOC
		if (sp->base) free(sp->base);
		free(sp);
#else
		sp->ptr = (char *)freed;
		freed = sp;
#endif
	}
}

/* allocate a string head */
static string *
s_alloc()
{
#ifdef DEBUG_USEMALLOC
	string *ret = (string*) malloc(sizeof(string));
	if (!ret) {
		pfmt(stderr, MM_ERROR, MAmallocating, Strerror(errno));
		exit(1);
	}
	return ret;
#else
	if (freep==NULL || freep->o >= STRALLOC) {
		freep = (stralloc *)malloc(sizeof(stralloc));
		if (freep==NULL) {
			pfmt(stderr, MM_ERROR, MAmallocating, Strerror(errno));
			exit(1);
		}
		freep->o = 0;
	}
	return &(freep->s[freep->o++]);
#endif
}

/* create a new `short' string */
extern string *
s_new()
{
	string *sp;

	if (freed!=NULL) {
		sp = freed;
		freed = (string *)(freed->ptr);
		sp->ptr = sp->base;
		s_terminate(sp);
		return sp;
	}
	sp = s_alloc();
	sp->base = sp->ptr = malloc(STRLEN);
	if (sp->base == NULL) {
		pfmt(stderr, MM_ERROR, MAmallocating, Strerror(errno));
		exit(1);
	}
	sp->end = sp->base + STRLEN;
	s_terminate(sp);
	return sp;
}

/* grow a string's allocation by at least `incr' bytes */
static void
s_simplegrow(sp, incr)
	string *sp;
	int incr;
{
	char *cp;
	int size;

	/*
	 *  take a larger increment to avoid mallocing too often
	 */
	if((sp->end-sp->base) < incr && MAXINCR < incr)
		size = (sp->end-sp->base) + incr;
	else if((sp->end-sp->base) > MAXINCR)
		size = (sp->end-sp->base)+MAXINCR;
	else
		size = 2*(sp->end-sp->base);

	cp = realloc(sp->base, (unsigned)size);
	if (cp == NULL) {
		pfmt(stderr, MM_ERROR, MAmallocating, Strerror(errno));
		exit(1);
	}
	sp->ptr = (sp->ptr - sp->base) + cp;
	sp->end = cp + size;
	sp->base = cp;
}

/* grow a string's allocation */
extern int
s_grow(sp, c)
	string *sp;
	int c;
{
	s_simplegrow(sp, 2);
	s_putc(sp, c);
	return c;
}

/* return a string containing a character array (this had better not grow) */
string *
s_array(cp, len)
	char *cp;
	int len;
{
	string *sp = s_alloc();

	sp->base = sp->ptr = cp;
	sp->end = sp->base + len;
	return sp;
}

/* return a string containing a copy of the passed char array */
extern string*
s_copy(cp)
	const char *cp;
{
	int len = strlen(cp) + 1;
	return s_copy_reserve(cp, len, len);
}

/* return a string containing a copy of the passed char array, but reserving */
/* enough space for alen bytes and copying slen bytes from the char array. */
/* sp->ptr is left pointing to the NUL terminator of the passed char array. */
extern string*
s_copy_reserve(cp, slen, alen)
	const char *cp;
	int slen;
	int alen;
{
	string *sp;

	sp = s_alloc();
	sp->base = malloc((unsigned)alen);
	if (sp->base == NULL) {
		pfmt(stderr, MM_ERROR, MAmallocating, Strerror(errno));
		exit(1);
	}
	sp->end = sp->base + alen;	/* point past end of allocation */
	(void) memcpy(sp->base, cp, slen);
	sp->ptr = sp->base + slen - 1;	/* point to NUL terminator */
	sp->ptr[0] = '\0';		/* plant the NUL */
	return sp;
}

/* convert string to lower case */
extern void
s_tolower(sp)
	string *sp;
{
	register char *cp;

	for (cp=sp->ptr; *cp; cp++)
		if(isupper(*cp))
			*cp = tolower(*cp);
}

/* move the ptr forward past any whitespace characters */
extern void
s_skipwhite(sp)
	string *sp;
{
	while (Isspace(*sp->ptr))
		s_skipc(sp);
}

/* append a char array to a string */
extern string *
s_append(to, from)
	register string *to;
	register const char *from;
{
	if (to == NULL)
		to = s_new();
	if (from == NULL)
		return to;
	for(; *from; from++)
		s_putc(to, *from);
	s_terminate(to);
	return to;
}

/* append a char array ( of up to n characters) to a string */
extern string *
s_nappend(to, from, n)
	register string *to;
	register const char *from;
{
	if (to == NULL)
		to = s_new();
	if (from == NULL)
		return to;
	for(; *from && n; from++, n--)
		s_putc(to, *from);
	s_terminate(to);
	return to;
}

/* Append a logical input sequence into a string.  Ignore blank and
 * comment lines.  Backslash preceding newline indicates continuation.
 * The `lineortoken' variable indicates whether the sequence to be input
 * is a whitespace delimited token or a whole line.
 *
 * Returns a pointer to the string (or NULL). Trailing newline is stripped off.
 */
extern string *
s_seq_read(fp, to, lineortoken)
	register FILE *fp;	/* stream to read from */
	string *to;		/* where to put token */
	int lineortoken;	/* how the sequence terminates */
{
	register int c;
	register int done=0;

	if(feof(fp))
		return NULL;

	/* get rid of leading goo */
	do {
		c = getc(fp);
		switch(c) {
		case EOF:
			if (to != NULL)
				s_terminate(to);
			return NULL;
		case '#':
			while((c = getc(fp)) != '\n' && c != EOF);
			break;
		case ' ':
		case '\t':
		case '\n':
		case '\r':
		case '\f':
			break;
		default:
			done = 1;
			break;
		}
	} while (!done);

	if (to == NULL)
		to = s_new();

	/* gather up a sequence */
	for (;;) {
		switch(c) {
		case '\\':
			c = getc(fp);
			if (c != '\n') {
				s_putc(to, '\\');
				s_putc(to, c);
			}
			break;
		case EOF:
		case '\r':
		case '\f':
		case '\n':
			s_terminate(to);
			return to;
		case ' ':
		case '\t':
			if (lineortoken == TOKEN) {
				s_terminate(to);
				return to;
			}
			/* fall through */
		default:
			s_putc(to, c);
			break;
		}
		c = getc(fp);
	}
}

extern string *
s_tok(from, split)
	string *from;
	char *split;
{
	string *ret = s_etok(from, split);
	if (ret) s_restart(ret);
	return ret;
}

extern string *
s_etok(from, split)
	string *from;
	char *split;
{
	char *splitend = strpbrk(from->ptr, split);

	if (splitend) {
		string *to = s_new();
		for ( ; from->ptr < splitend; )
			s_putc(to, *from->ptr++);
		s_terminate(to);
		from->ptr += strspn(from->ptr, split);
		return to;
	}

	else if (from->ptr[0]) {
		string *to = s_clone(from);
		s_skiptoend(from);
		return to;
	}

	else
		return 0;
}

extern string *
s_tokc(from, split)
	string *from;
	int split;
{
	string *ret = s_etokc(from, split);
	if (ret) s_restart(ret);
	return ret;
}

extern string *
s_etokc(from, split)
	string *from;
	int split;
{
	char *splitend = strchr(from->ptr, split);

	if (splitend) {
		string *to = s_new();
		for ( ; from->ptr < splitend; )
			s_putc(to, *from->ptr++);
		s_terminate(to);
		from->ptr++;
		return to;
	}

	else if (from->ptr[0]) {
		string *to = s_clone(from);
		s_skiptoend(from);
		return to;
	}

	else
		return 0;
}

void
s_skiptoend(from)
string *from;
{
	while (*from->ptr)
		from->ptr++;
}

/* Append an input line to a string.
 *
 * Returns a pointer to the string (or NULL).
 * Trailing newline is left on.
 */
extern char *
s_read_line(fp, to)
	register FILE *fp;
	register string *to;
{
	register int c;
	register int len=0;

	s_terminate(to);

	/* end of input */
	if (feof(fp) || (c=getc(fp)) == EOF)
		return NULL;

	/* gather up a line */
	for(;;) {
		len++;
		switch(c) {
		case EOF:
			s_terminate(to);
			return to->ptr-len;
		case '\n':
			s_putc(to, '\n');
			s_terminate(to);
			return to->ptr-len;
		default:
			s_putc(to, c);
			break;
		}
		c=getc(fp);
	}
}

/*
 *  Read till eof or some limit is passed.  If the limit is passed,
 *  return a negative count.
 */
extern int
s_read_to_lim(fp, to, lim)
	register FILE *fp;
	register string *to;
	int lim;
{
	register int got;
	register int have;

	s_terminate(to);

	for(;;){
		if(lim && to->ptr - to->base > lim){
			s_terminate(to);
			return -(to->ptr - to->base);
		}
		if(feof(fp))
			break;
		/* allocate room for a full buffer */
		have = to->end - to->ptr;
		if(have<4096)
			s_simplegrow(to, 4096);

		/* get a buffers worth */
		have = to->end - to->ptr;
		got = fread(to->ptr, 1, have, fp);
		if(got<=0)
			break;
		to->ptr += got;
	}

	/* null terminate the line */
	s_terminate(to);
	return to->ptr - to->base;
}

/*
 *  read to eof
 */
extern int
s_read_to_eof(fp, to)
	register FILE *fp;
	register string *to;
{
	return s_read_to_lim(fp, to, 0);
}

/* Get the next field from a string.  The field is delimited by white space,
 * single or double quotes.
 */
extern string *
s_parse(from, to)
	string *from;	/* string to parse */
	string *to;	/* where to put parsed token */
{
	while (Isspace(*from->ptr))
		from->ptr++;
	if (*from->ptr == '\0')
		return NULL;
	if (to == NULL)
		to = s_new();
	if (*from->ptr == '\'') {
		from->ptr++;
		for (;*from->ptr != '\'' && *from->ptr != '\0'; from->ptr++)
			s_putc(to, *from->ptr);
		if (*from->ptr == '\'')
			from->ptr++;
	} else if (*from->ptr == '"') {
		from->ptr++;
		for (;*from->ptr != '"' && *from->ptr != '\0'; from->ptr++)
			s_putc(to, *from->ptr);
		if (*from->ptr == '"')
			from->ptr++;
	} else {
		for (;!Isspace(*from->ptr) && *from->ptr != '\0'; from->ptr++)
			s_putc(to, *from->ptr);
	}
	s_terminate(to);

	return to;
}

/* VARARGS3 */
string *
#ifdef __STDC__
s_xappend(string *to, const char *from1, const char *from2, ...)
#else
# ifdef lint
s_xappend(Xto, Xfrom1, Xfrom2, va_alist)
string *Xto;
char *Xfrom1;
char *Xfrom2;
va_dcl
# else
s_xappend(va_alist)
va_dcl
# endif
#endif
{
#ifndef __STDC__
	string *to;
	char *from1;
	char *from2;
#endif
	va_list args;

#ifndef __STDC__
# ifdef lint
	to = Xto;
	from1 = Xfrom1;
	from2 = Xfrom2;
# endif
#endif

#ifdef __STDC__
	va_start(args, from2);
#else
	va_start(args);
	to = va_arg(args, string*);
	from1 = va_arg(args, char*);
	from2 = va_arg(args, char*);
#endif
	to = s_append(to, from1);
	for ( ; from2; from2 = va_arg(args, const char*))
		to = s_append(to, from2);
	return to;
}

/* write out a string to the given FILE* */
void s_write(line, outfp)
string *line;
FILE *outfp;
{
    if (s_curlen(line) > 0)
	(void) fwrite(s_to_c(line), sizeof(char), s_curlen(line), outfp);
}
