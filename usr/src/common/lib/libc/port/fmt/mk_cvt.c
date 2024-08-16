/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:fmt/mk_cvt.c	1.2"
/*
* mk_cvt.c - A program that writes _cvt.c, a target-specific source file
*		that contains the floating to string conversion functions.
*
* Usage: ./mk_cvt [-c <config>] [-o <cvt.c>] [-s <script>] [-t <template>]
*/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#ifdef __STDC__
#include <stdlib.h>
#else
extern char *malloc();
extern void free();
extern int getopt();
extern int unlink();
#define remove(a) unlink(a)
#endif

#ifndef SEEK_SET
#define SEEK_SET 0
#endif

#define LOG2TO10(x)	(((x) * 30103L + 50000000L) / 100000L - 500)

#define BIT(n)	((unsigned long)1 << (n))
#define LOW(n)	(~(~(unsigned long)0 << (n)))
#define HIGH(n)	(~(~(unsigned long)0 >> (n)))

enum { FN_CONFIG, FN_OUTPUT, FN_TEMPLATE, FN_end };

static struct	/* files mk_cvt reads and writes */
{
	char	*mode;
	char	*name;
	FILE	*fp;
} finfo[FN_end] =
{
	{"r",	"cvt-config"},
	{"w",	"_cvt.c"},
	{"r",	"cvt-template"},
};

static char	*tfile_fname;
static char	*script_fname = "cvt-script";
static FILE	*tables_fp;

#define DENORM_INPUT	0x1	/* w<n> holds extra information */
#define DENORM_OUTPUT	0x2	/* w<n> should be left holding extra */
#define WIDER_INPUT	0x4	/* w<n+1> holds information */

enum { TN_LOW, TN_MED, TN_HIGH, TN_end };

static struct	/* precomputed table information */
{
	char	*name;
	int	norm;
	char	*tfmt;	/* if (<tfmt>) {...} */
	char	*pfmt;	/* &<name>[<pfmt>][0] */
	long	base;
	long	maxpos;
} tinfo[TN_end] =
{
	{"pow10low",	DENORM_OUTPUT,
		"/*%ld*/(%s & LOW(5)) != 0",
			"%ld > %ld ? (%ld - %s) & LOW(5) : %ld - %s"
		},
	{"pow10med",	0,
		"(%ld - %s) >> 5 != 0",
			"%ld>%ld?((%ld-%s)>>5)&LOW(5):(%ld-%s)>>5",
		},
	{"pow10high",	0,
		"(%ld - %s) >> 10 != 0",
			"/*%ld,%ld*/(%ld - %s) >> 10",
		},
};

static char	usage_fmtstr[] =
	"Usage: %s [-c <config>] [-o <cvt.c>] [-s <script>] [-t <template>]\n";

static int	debug;	/* enables debugging */

#define WSIZEMIN	14
#define LONGSIZEMIN	32
#define PRETYPESIZEMIN	16
#define EXPSIZEMAX	15

static long	wsize = WSIZEMIN;		/* bits/packet */
static long	longsize = LONGSIZEMIN;		/* bits/long */
static long	pretypesize = PRETYPESIZEMIN;	/* bits/precomptype */
static char	*precomptype = "short";		/* precomputed packet type */

static long	dbl_exp;	/* binary exponent size for double */
static long	dbl_bias;	/* double exponent bias */
static long	dbl_signif;	/* full number of bits (implicit, too) */
static long	ldbl_exp;	/* binary exponent size for long double */
static long	ldbl_bias;	/* long double exponent bias */
static long	ldbl_signif;	/* full number of bits (implicit, too) */

static long	dbl_inputlen;	/* number of longs initially set for double */
static long	dbl_pow10hi;	/* most positive power for double scaling */
static long	dbl_pow10lo;	/* most negative power for double scaling */
static long	ldbl_inputlen;	/* no. of longs initially set for long double */
static long	ldbl_pow10hi;	/* most positive power for long double scaling */
static long	ldbl_pow10lo;	/* most negative power for long double scaling */
static long	precomplen;	/* no. of <precomptype>s for table significands */
static long	maxpow10hi;	/* max(dbl_pow10hi, ldbl_pow10hi) */
static long	fract_index;	/* index into TN_LOW for fraction scaling */
static long	fract_nonzero;	/* last nonzero packet in TN_LOW[fract_index] */
static long	fract_digits;	/* number of decimal digits after scaling */
static long	fract_exp2;	/* binary exponent after scaling */

#ifdef __STDC__
static int	gen_decl(char *);
static int	gen_tables(char *);
static int	gen_mult(char *);
static int	gen_more(char *);
static int	gen_zero(char *);
#else
static int	gen_decl();
static int	gen_tables();
static int	gen_mult();
static int	gen_more();
static int	gen_zero();
#endif

struct cval
{
	char	*name;
	long	loc[2];		/* start/end (or 0/value for numeric) */
	int	reqflag;	/* 0:not needed; 1:needed; 2:long double needs */
	long	*nval;		/* nonnull for numeric values */
	char	**sval;		/* nonnull for string values */
	int	(*gen)();	/* nonnull for self-generating strings */
};

static struct cval config[] =
{
	/*
	* Items that are format-independent.
	*/
	{"CVT-WSIZE",		-1, -1, 0, &wsize},
	{"CVT-LONGSIZE",	-1, -1, 0, &longsize},
	{"CVT-PRETYPESIZE",	-1, -1, 0, &pretypesize},
	{"CVT-PRECOMPTYPE",	-1, -1, 0, 0, &precomptype},
	{"CVT-IDENT",		-1, -1, 1},
	{"CVT-FILEDECL",	-1, -1, 0},
	/*
	* Format-specific items that are not generatable.
	*/
	{"CVTD-EXPSIZE",	-1, -1, 1, &dbl_exp},
	{"CVTD-SIGNIFSIZE",	-1, -1, 1, &dbl_signif},
	{"CVTD-EXPBIAS",	-1, -1, 1, &dbl_bias},
	{"CVTD-DECL",		-1, -1, 0},
	{"CVTD-EXPLODE",	-1, -1, 1},
	{"CVTL-EXPSIZE",	-1, -1, 2, &ldbl_exp},
	{"CVTL-SIGNIFSIZE",	-1, -1, 2, &ldbl_signif},
	{"CVTL-EXPBIAS",	-1, -1, 2, &ldbl_bias},
	{"CVTL-DECL",		-1, -1, 0},
	{"CVTL-EXPLODE",	-1, -1, 2},
	/*
	* Generatable items.  Can be specified by configuration file.
	*/
	{"CVT-GEN-TABLES",	-1, -1, 0, 0, 0, gen_tables},
	{"CVTD-GEN-DECL",	-1, -1, 0, 0, 0, gen_decl},
	{"CVTD-GEN-MULT",	-1, -1, 0, 0, 0, gen_mult},
	{"CVTD-GEN-MORE",	-1, -1, 0, 0, 0, gen_more},
	{"CVTD-GEN-ZERO",	-1, -1, 0, 0, 0, gen_zero},
	{"CVTL-GEN-DECL",	-1, -1, 0, 0, 0, gen_decl},
	{"CVTL-GEN-MULT",	-1, -1, 0, 0, 0, gen_mult},
	{"CVTL-GEN-MORE",	-1, -1, 0, 0, 0, gen_more},
	{"CVTL-GEN-ZERO",	-1, -1, 0, 0, 0, gen_zero},
	0
};

static int
#ifdef __STDC__
getconfig(void)	/* process config file specifications */
#else
getconfig()
#endif
{
	char line[BUFSIZ], *p;
	int cont, lineno;
	long was_at, now_at, *lp;
	register struct cval *cp;

	if (debug)
		fprintf(stderr, "getconfig()\n");
	cont = EOF;
	lineno = 0;
	now_at = 0;
	while (fgets(line, BUFSIZ, finfo[FN_CONFIG].fp) != 0)
	{
		lineno++;
		was_at = now_at;
		now_at += strlen(line);
		if (debug > 2)
		{
			fprintf(stderr, "READ(line %d, byte %ld, len %d):%s",
				lineno, was_at, now_at - was_at, line);
		}
		if (cont != EOF)
		{
			if (line[0] == cont)
				continue;
			cont = EOF;
			*lp = was_at;
			if (debug)
			{
				fprintf(stderr, " ... to %ld (multiline)\n",
					now_at);
			}
		}
		switch (line[0])
		{
		default:
			fprintf(stderr,
				"%s:%d:invalid config file line\n",
				finfo[FN_CONFIG].name, lineno);
			return -1;
		case '#':
		case ' ':
		case '\t':
		case '\n':
			continue;
		case 'C':	/* all parameters are "CVT*" */
			break;
		}
		(void)strtok(line, " \t\n");
		for (cp = &config[0];; cp++)
		{
			if (cp->name == 0)
			{
				fprintf(stderr,
					"%s:%d:unknown specification: %s\n",
					finfo[FN_CONFIG].name, lineno, line);
				return -1;
			}
			if (strcmp(line, cp->name) == 0)
				break;
		}
		if (cp->loc[0] >= 0)
		{
			fprintf(stderr,
				"%s:%d:too many %s specifications\n",
				finfo[FN_CONFIG].name, lineno, line);
			return -1;
		}
		p = strtok((char *)0, "\n");
		if (cp->nval != 0)	/* numeric value */
		{
			if (p == 0 || (cp->loc[1] = atol(p)) <= 0)
			{
				fprintf(stderr,
					"%s:%d:bad numeric specification: %s\n",
					finfo[FN_CONFIG].name, lineno, line);
				return -1;
			}
			cp->loc[0] = 0;
			*cp->nval = cp->loc[1];
			if (debug)
				fprintf(stderr, "SET %s %ld\n", line, cp->loc[1]);
		}
		else if (cp->sval != 0)	/* string value */
		{
			if (p == 0)
			{
				fprintf(stderr,
					"%s:%d:missing string specification: %s\n",
					finfo[FN_CONFIG].name, lineno, line);
				return -1;
			}
			cp->loc[0] = was_at + (p - line - 1);
			cp->loc[1] = now_at;
			if ((*cp->sval = malloc(strlen(p) + 1)) == 0)
			{
				perror("cannot hold string value");
				return -1;
			}
			strcpy(*cp->sval, p);
			if (debug)
				fprintf(stderr, "SET %s %s\n", line, p);
		}
		else if (p != 0)
		{
			cp->loc[0] = was_at + (p - line - 1);
			cp->loc[1] = now_at;
			if (debug)
			{
				fprintf(stderr,
					"SET %s from %ld to %ld (1 line)\n",
					line, cp->loc[0], cp->loc[1]);
			}
		}
		else
		{
			cp->loc[0] = now_at;
			cp->loc[1] = now_at;
			if ((cont = getc(finfo[FN_CONFIG].fp)) == EOF)
			{
				fprintf(stderr,
					"%s:%d:incomplete specification: %s\n",
					finfo[FN_CONFIG].name, lineno, line);
				return -1;
			}
			ungetc(cont, finfo[FN_CONFIG].fp);
			lp = &cp->loc[1];
			if (debug)
			{
				fprintf(stderr, "SET %s from %ld (cont %c)%s",
					line, cp->loc[0], cont,
					debug > 2 ? " ...\n" : "");
			}
		}
	}
	if (cont != EOF)
	{
		*lp = now_at - 1;
		if (debug)
			fprintf(stderr, " ... to %ld (multiline)\n", now_at);
	}
	/*
	* Check for required items.
	*/
	cont = 0;
	now_at = 0;
	was_at = 0;
	for (cp = &config[0]; cp->name != 0; cp++)
	{
		switch (cp->reqflag)
		{
		default:
			fprintf(stderr, "unknown reqflag value: %d\n",
				cp->reqflag);
			return -1;
		case 0:
			continue;
		case 1:
			if (cp->loc[0] < 0)
			{
				fprintf(stderr,
					"missing required config field: %s\n",
					cp->name);
				cont = 1;
			}
			continue;
		case 2:
			now_at++;
			if (cp->loc[0] < 0)
				was_at++;
			continue;
		}
	}
	if (was_at != 0 && now_at != was_at)
	{
		fputs("all long double config fields not present\n", stderr);
		cont = 1;
	}
	if (cont != 0)
		return -1;
	return 0;
}

static int
#ifdef __STDC__
check(void)	/* check sanity of numeric values; derive other parameters */
#else
check()
#endif
{
	if (debug)
		fprintf(stderr, "check()\n");
	/*
	* Check sanity of numeric items.
	* Derive other useful values from those specified.
	*/
	if (longsize < LONGSIZEMIN)
	{
		fprintf(stderr, "too few bits/long: %ld\n", longsize);
		return -1;
	}
	if (pretypesize < PRETYPESIZEMIN)
	{
		fprintf(stderr, "too few bits/precomptype (%s): %ld\n",
			precomptype, pretypesize);
		return -1;
	}
	if (wsize < WSIZEMIN || 2 * wsize + 1 >= longsize
		|| 3 * wsize <= longsize || wsize + 1 >= pretypesize)
	{
		fprintf(stderr, "invalid bits/packet: %ld\n", wsize);
		return -1;
	}
	if (dbl_exp > EXPSIZEMAX || dbl_bias * 2 >= BIT(dbl_exp))
	{
		fprintf(stderr, "invalid double exponent/bias pair: %ld/%ld\n",
			dbl_exp, dbl_bias);
		return -1;
	}
	dbl_inputlen = (dbl_signif + wsize - 1) / wsize;
	precomplen = dbl_inputlen + 1;
	dbl_pow10hi = LOG2TO10(dbl_bias + dbl_signif + longsize);
	dbl_pow10lo = LOG2TO10(LOW(dbl_exp) - dbl_bias);
	maxpow10hi = dbl_pow10hi;
	if (ldbl_exp != 0 || ldbl_bias != 0 || ldbl_signif != 0)
	{
		if (ldbl_exp < dbl_exp || ldbl_signif < dbl_signif)
		{
			fputs("long double smaller than double\n", stderr);
			return -1;
		}
		if (ldbl_exp > EXPSIZEMAX || ldbl_bias * 2 >= BIT(ldbl_exp))
		{
			fprintf(stderr,
				"invalid long double exponent/bias pair: %ld/%ld\n",
				ldbl_exp, ldbl_bias);
			return -1;
		}
		ldbl_inputlen = (ldbl_signif + wsize - 1) / wsize;
		precomplen = ldbl_inputlen + 1;
		ldbl_pow10hi = LOG2TO10(ldbl_bias + ldbl_signif + longsize);
		ldbl_pow10lo = LOG2TO10(LOW(ldbl_exp) - ldbl_bias);
		maxpow10hi = ldbl_pow10hi;
	}
	if (debug)
	{
		fprintf(stderr, "SCALE: max 1e%ld, min 1e-%ld\n", maxpow10hi,
			ldbl_pow10lo > dbl_pow10lo ? ldbl_pow10lo : dbl_pow10lo);
	}
	return 0;
}

static int
#ifdef __STDC__
mktables(void)	/* start bc with computed parameters */
#else
mktables()
#endif
{
	FILE *fp;
	char *cmd;
	long i, j, n, tot;

	if (debug)
		fprintf(stderr, "mktables()\n");
	if ((tfile_fname = tempnam((char *)0, "cvt-")) == 0)
	{
		perror("cannot build temporary filename");
		return -1;
	}
	if ((fp = fopen(tfile_fname, "w+")) == 0)
	{
		fputs("cannot open temporary file ", stderr);
		perror(tfile_fname);
		return -1;
	}
	fprintf(fp, "scale = %ld\ni = 10\n", LOG2TO10(precomplen * wsize + 6));
	tot = 0;
	n = 1;
	j = 0;
	for (;;)
	{
		if (j >= TN_end)
		{
			fputs("too many power-of-10 tables to generate\n",
				stderr);
			return -1;
		}
		fprintf(fp, "\"$%s\n\"\n", tinfo[j].name);
		tinfo[j].base = tot;
		if ((i = (maxpow10hi - tot) / n) >= 16)
		{
			fprintf(fp, "i = n(%ld, i, 16)\ni = i * i\n", n);
			n <<= 4;
			tinfo[j].maxpos = n;
			tot += n;
			n <<= 1;
			if (debug)
			{
				fprintf(stderr,
					"TAB %s[32]: base 1e%ld, max 1e%ld\n",
					tinfo[j].name, tinfo[j].base,
					tinfo[j].maxpos);
			}
			j++;
			continue;
		}
		if (n * i + tot != maxpow10hi)
			i++;
		fprintf(fp, "i = n(%ld, i, %ld)\n", n, i);
		tinfo[j].maxpos = n * i;
		if (debug)
		{
			fprintf(stderr, "TAB %s[%d]: base 1e%ld, max 1e%ld\n",
				tinfo[j].name, i * 2 + 1, tinfo[j].base,
				tinfo[j].maxpos);
		}
		break;
	}
	fputs("\"$$\n\"\nquit\n", fp);
	if (debug > 1)
	{
		fputs("BC INPUT (to --DONE--):\n", stderr);
		rewind(fp);
		while ((i = getc(fp)) != EOF)
			putc(i, stderr);
		fputs("--DONE--\n", stderr);
	}
	fclose(fp);
	if ((cmd = malloc(3 + strlen(script_fname) +
		1 + strlen(tfile_fname) + 1)) == 0)
	{
		perror("cannot build bc command string");
		return -1;
	}
	sprintf(cmd, "bc %s %s", script_fname, tfile_fname);
	if ((tables_fp = popen(cmd, "r")) == 0)
	{
		perror("cannot invoke bc command");
		return -1;
	}
	free(cmd);
	fract_exp2 = -1;
	fract_digits = LOG2TO10(longsize - 1);
	if ((fract_index = tinfo[TN_LOW].maxpos - fract_digits) < 0)
	{
		fract_index = 0;
		fract_digits = tinfo[TN_LOW].maxpos;
	}
	return 0;
}

static char *
#ifdef __STDC__
tohexlist(register char *bp, register char *ep)	/* make hex list from bc bits */
#else
tohexlist(bp, ep)register char *bp, *ep;
#endif
{
	static char hex[] = "0123456789abcdef";
	int i;
	long n;

	if (bp[0] != '1' || bp[1] != '.')
	{
		fprintf(stderr, "unnormalized bc output value: %.20s...\n", bp);
		return 0;
	}
	bp += 2;
	if ((n = precomplen * wsize) + 2 > ep - bp)
	{
		fprintf(stderr, "fraction too short; at least %ld bits needed\n",
			n + 2);
		return 0;
	}
	bp += n;
	if (bp[0] == '1')	/* check for rounding */
	{
		register char *p = bp;

		if (p[-1] == '1')	/* round up if otherwise odd */
		{
		roundup:;
			while (*--p == '1')
				*p = '0';
			*p = '1';
		}
		else	/* see if it's more than 0.5 ulp */
		{
			for (i = ep - p; --i != 0;)
			{
				if (p[i] == '1')
					goto roundup;
			}
		}
	}
	i = wsize;
	for (n = (wsize + 3) / 4 * precomplen; --n >= 0;)
	{
		register int nibble;

		switch (i)
		{
		default:
			nibble = *--bp - '0';
			nibble |= (*--bp - '0') << 1;
			nibble |= (*--bp - '0') << 2;
			nibble |= (*--bp - '0') << 3;
			break;
		case 3:
			nibble = *--bp - '0';
			nibble |= (*--bp - '0') << 1;
			nibble |= (*--bp - '0') << 2;
			break;
		case 2:
			nibble = *--bp - '0';
			nibble |= (*--bp - '0') << 1;
			break;
		case 1:
			nibble = *--bp - '0';
			break;
		}
		*--ep = hex[nibble];
		if ((i -= 4) <= 0)
		{
			i = wsize;
			*--ep = 'x';
			*--ep = '0';
			*--ep = ',';
		}
	}
	/*
	* Add in extra high-order bit.
	*/
	if ((i = wsize % 4) == 0)
	{
		*--ep = ',';
		ep[1] = '0';
		ep[2] = 'x';
		ep[3] = '1';
	}
	else
		ep[3] += BIT(i);
	return ep;
}

static int
#ifdef __STDC__
gen_tables(char *unused)	/* emit all tables to be used */
#else
gen_tables(unused)char *unused;
#endif
{
	char line[BUFSIZ], num[20], exp[20];
	char *p, *q;
	int index;

	if (debug)
		fprintf(stderr, "gen_tables()\n");
	index = -1;
	for (;;)
	{
		if ((p = fgets(line, BUFSIZ, tables_fp)) == 0)
		{
			fputs("EOF before tables end in bc output\n", stderr);
			return -1;
		}
		while ((p = strchr(p, '\\')) != 0) /* merge continuations */
		{
			if (fgets(p, &line[BUFSIZ] - p, tables_fp) == 0)
			{
				fputs("premature EOF in bc output\n", stderr);
				return -1;
			}
		}
		if ((q = strchr(line, '\n')) == 0)
		{
			fprintf(stderr, "bc output line too long: %.20s...\n",
				line);
			return -1;
		}
		if (debug > 3)
			fprintf(stderr, "READ(bc):%s", line);
		*q = '\0';
		if (line[0] == '$')	/* table begin/end mark */
		{
			if (index >= 0)
				fputs("};\n", finfo[FN_OUTPUT].fp);
			if (line[1] == '$')
			{
				pclose(tables_fp);
				tables_fp = 0;
				return 0;
			}
			fprintf(finfo[FN_OUTPUT].fp,
				"static const %s %s[][1 + %ld] =\n{\n",
				precomptype, &line[1], precomplen);
			index = 0;
			continue;
		}
		if ((p = strchr(line, '=')) == 0)
		{
		huh:;
			fprintf(stderr, "unexpected bc output: %s\n", line);
			return -1;
		}
		switch (line[0])
		{
		default:
			goto huh;
		case 'N':
			if (q - p >= sizeof(num))
			{
				fprintf(stderr,
					"decimal value field too long: %.20s...\n",
					line);
				return -1;
			}
			strcpy(num, p + 1);
			continue;
		case 'E':
			if (q - p >= sizeof(exp))
			{
				fprintf(stderr,
					"exponent field too long: %.20s...\n",
					line);
				return -1;
			}
			strcpy(exp, p + 1);
			continue;
		case 'S':
			break;
		}
		if (index >= 32)
			continue;
		fprintf(finfo[FN_OUTPUT].fp, " /*[%2d]*/ {%s", index, exp);
		if ((p = tohexlist(p + 1, q)) == 0)
			return -1;
		fprintf(finfo[FN_OUTPUT].fp, "%s}, /*%s*/\n", p, num);
		if (index == fract_index && fract_exp2 < 0)
		{
			fract_exp2 = atol(exp);
			p += strlen(p);
			fract_nonzero = precomplen;
			do
			{
				if (*--p == ',')
					fract_nonzero--;
			} while (*p == '0' || !isxdigit(*p));
		}
		index++;
	}
}

static int
#ifdef __STDC__
gen_decl(char *p)	/* emit all w<n> declarations */
#else
gen_decl(p)char *p;
#endif
{
	int i, len;

	if (debug)
		fprintf(stderr, "gen_decl()\n");
	if (p[3] == 'D')
		len = dbl_inputlen;
	else if ((len = ldbl_inputlen) == 0)
		return 0;
	if (len > 2)
	{
		fputs("\tregister long w1, w2, w3; long w4", finfo[FN_OUTPUT].fp);
		i = 4;
	}
	else
	{
		fputs("\tregister long w1", finfo[FN_OUTPUT].fp);
		i = 1;
	}
	while (i <= len)
		fprintf(finfo[FN_OUTPUT].fp, ", w%d", ++i);
	fputs(";\n", finfo[FN_OUTPUT].fp);
	return 0;
}

static int
#ifdef __STDC__
tab_mult(int tno, int len, int max, int norm, char *pow) /* emit single multiply */
#else
tab_mult(tno, len, max, norm, pow)int tno, len, max, norm; char *pow;
#endif
{
	long sum;
	int last, i, j;

	if (norm == 0)
		fprintf(finfo[FN_OUTPUT].fp, "\tw%d = 0;\n", len + 1);
	fputs("\tif (", finfo[FN_OUTPUT].fp);
	fprintf(finfo[FN_OUTPUT].fp, tinfo[tno].tfmt, tinfo[tno].base, pow);
	fprintf(finfo[FN_OUTPUT].fp, ")\n\t{\n\t\tregister const %s *p;\n\n",
		precomptype);
	last = len;
	if (norm & DENORM_INPUT)
	{
		last++;
		for (i = len; i != 0; i--)
		{
			fprintf(finfo[FN_OUTPUT].fp, "\t\tw%d = w%d & WMASK;\n",
				i + 1, i);
		}
		fputs("\t\tw1 >>= WSIZE;\n", finfo[FN_OUTPUT].fp);
	}
	else if (norm & WIDER_INPUT)
		last++;
	fprintf(finfo[FN_OUTPUT].fp, "\t\tp = &%s[", tinfo[tno].name);
	sum = tinfo[tno].base + tinfo[tno].maxpos;
	fprintf(finfo[FN_OUTPUT].fp, tinfo[tno].pfmt,
		max, sum, sum, pow, sum, pow);
	fprintf(finfo[FN_OUTPUT].fp,
		"][0];\n\t\texp2 += p[0];\n\t\tw%d = w%d*p[1] +", len, len);
	for (i = 1; i < len; i++)
		fprintf(finfo[FN_OUTPUT].fp, " w%d*p[%d] +", len - i, i + 1);
	fprintf(finfo[FN_OUTPUT].fp, "\n\t\t   ((w%d*p[%d]", last, len - last + 2);
	for (i = 1; i < last; i++)
	{
		fprintf(finfo[FN_OUTPUT].fp, " + w%d*p[%d]",
			last - i, i + len - last + 2);
	}
#if 0
	fputs(") >> WSIZE)", finfo[FN_OUTPUT].fp);
	if (precomplen <= last + 1)
		fputs(";\n", finfo[FN_OUTPUT].fp);
	else
	{
		fprintf(finfo[FN_OUTPUT].fp, " +\n\t\t   ((w%d*p[%d]",
			last, len - last + 3);
		for (i = 1; i < last; i++)
		{
			fprintf(finfo[FN_OUTPUT].fp, " + w%d*p[%d]",
				last - i, i + len - last + 3);
		}
		fputs(") >> (2*WSIZE));\n", finfo[FN_OUTPUT].fp);
	}
#else
	fputs(") >> WSIZE);\n", finfo[FN_OUTPUT].fp);
#endif
	for (i = len; --i != 0;)
	{
		fprintf(finfo[FN_OUTPUT].fp, "\t\tw%d *= p[1];\n\t\tw%d += ",
			i, i);
		for (j = i; --j != 0;)
			fprintf(finfo[FN_OUTPUT].fp, "w%d*p[%d] + ", j, 1 + i - j);
		fprintf(finfo[FN_OUTPUT].fp, "(w%d >> WSIZE);\n", i + 1);
	}
	if ((norm & (DENORM_OUTPUT | DENORM_INPUT))
		== (DENORM_OUTPUT | DENORM_INPUT))
	{
		fputs("\t}\n", finfo[FN_OUTPUT].fp);
	}
	else if (norm & DENORM_OUTPUT)
	{
		fputs("\t}\n\telse\n\t{\n\t\tw1 <<= WSIZE;\n\t\tw1 |= w2;\n",
			finfo[FN_OUTPUT].fp);
		for (i = 2; i < last; i++)
			fprintf(finfo[FN_OUTPUT].fp, "\t\tw%d = w%d;\n", i, i + 1);
		fputs("\t}\n", finfo[FN_OUTPUT].fp);
		norm |= DENORM_INPUT;
	}
	else if (norm == 0)
	{
		for (i = len; i != 0; i--)
		{
			fprintf(finfo[FN_OUTPUT].fp, "\t\tw%d = w%d & WMASK;\n",
				i + 1, i);
		}
		fputs("\t\tw1 >>= WSIZE;\n\t}\n", finfo[FN_OUTPUT].fp);
		norm |= DENORM_OUTPUT;
	}
	return norm | WIDER_INPUT;
}

static int
#ifdef __STDC__
gen_mult(char *p)	/* emit entire multiplication step */
#else
gen_mult(p)char *p;
#endif
{
	int tno, i, len, high;
	char *pow;

	if (debug)
		fprintf(stderr, "gen_mult()\n");
	if (p[3] == 'D')
	{
		len = dbl_inputlen;
		high = dbl_pow10hi;
	}
	else if ((len = ldbl_inputlen) == 0)
		return 0;
	else
		high = ldbl_pow10hi;
	if ((pow = strchr(p, ':')) == 0)
	{
		/*
		* Multiply a fractional value by 10^fract_digits.
		* Leave denormalized, but with w<2>... clean.
		* Since approximately (at least) 2*wsize bits of
		* significance have been removed, do one less word.
		*/
		len--;
		fprintf(finfo[FN_OUTPUT].fp,
			"\t    {\n\t\tregister const %s *p = %s[%d];\n\n",
			precomptype, tinfo[TN_LOW].name, fract_index);
		fprintf(finfo[FN_OUTPUT].fp, "\t\tw%d = w%d*p[1] +", len, len);
		for (i = 1; i < len; i++)
		{
			if (i >= fract_nonzero)
				break;
			fprintf(finfo[FN_OUTPUT].fp, " w%d*p[%d] +",
				len - i, i + 1);
		}
		if (fract_nonzero < 2)
			fputs(" 0;\n", finfo[FN_OUTPUT].fp);
		else
		{
			fprintf(finfo[FN_OUTPUT].fp, " ((w%d*p[2]", len);
			for (i = 1; i < len; i++)
			{
				if (i >= fract_nonzero - 1)
					break;
				fprintf(finfo[FN_OUTPUT].fp, " + w%d*p[%d]",
					len - i, i + 2);
			}
			fputs(") >> WSIZE);\n", finfo[FN_OUTPUT].fp);
		}
		for (i = len; --i != 0;)
		{
			fprintf(finfo[FN_OUTPUT].fp,
				"\t\tw%d *= p[1];\n\t\tw%d += ", i, i);
			for (tno = i; --tno != 0;)
			{
				if (1 + i - tno > fract_nonzero)
					break;
				fprintf(finfo[FN_OUTPUT].fp, "w%d*p[%d] + ",
					tno, 1 + i - tno);
			}
			fprintf(finfo[FN_OUTPUT].fp, "(w%d >> WSIZE);\n", i + 1);
		}
		fputs("\t    }\n", finfo[FN_OUTPUT].fp);
		i = 1;
		while (i < len)
			fprintf(finfo[FN_OUTPUT].fp, "\t\tw%d &= WMASK;\n", ++i);
		return 0;
	}
	i = strlen(p);
	if (p[i - 3] != '*')
	{
		fprintf(stderr, "bad power-of-10 expression for %s", p);
		return -1;
	}
	pow++;
	p[i - 3] = '\0';
	tno = 0;
	while (tinfo[tno].base + tinfo[tno].maxpos < high)
	{
		if (++tno == TN_end)
		{
			fputs("too few power-of-10 tables\n", stderr);
			return -1;
		}
	}
	i = tinfo[tno].norm;
	do
	{
		i = tab_mult(tno, len, high, i, pow);
	} while (--tno >= 0);
	i = 1;
	while (i < len)
		fprintf(finfo[FN_OUTPUT].fp, "\tw%d &= WMASK;\n", ++i);
	fprintf(finfo[FN_OUTPUT].fp, "\tw%d = 0;\n", len + 1);
	return 0;
}

static int
#ifdef __STDC__
gen_more(char *p)	/* get w<n> ready for fraction's multiply */
#else
gen_more(p)char *p;
#endif
{
	int i, len;
	char *scale;

	if (debug)
		fprintf(stderr, "gen_more()\n");
	if (p[3] == 'D')
		len = dbl_inputlen;
	else if ((len = ldbl_inputlen) == 0)
		return 0;
	if ((scale = strchr(p, ':')) == 0)
	{
		int shift;

		/*
		* Normalize after fractional scaling.  Only len-2 packets are
		* to be kept from now on.
		*/
		len--;
		if ((shift = fract_exp2 - 2 * wsize) < 0)
		{
			shift += wsize;
			fprintf(finfo[FN_OUTPUT].fp,
				"\t\tw1 = (w%d << %d) & WMASK;\n\t\tw%d = 0;\n",
				len + 2, shift, len + 2);
			for (i = 2; i < len; i++)
			{
				fprintf(finfo[FN_OUTPUT].fp,
					"\t\tw%d <<= %d;\n\t\tw%d |= w%d ",
					i, shift, i - 1, i);
				fprintf(finfo[FN_OUTPUT].fp,
					">> WSIZE;\n\t\tw%d &= WMASK;\n", i);
			}
			fprintf(finfo[FN_OUTPUT].fp,
				"\t\tw%d |= w%d >> %d;\n",
				len - 1, len, wsize - shift);
		}
		else if (shift > 0)
		{
			for (i = 1; i < len; i++)
			{
				fprintf(finfo[FN_OUTPUT].fp,
					"\t\tw%d = w%d << %d;\n",
					i, i + 1, shift);
			}
			for (i = 2; i < len; i++)
			{
				fprintf(finfo[FN_OUTPUT].fp,
					"\t\tw%d |= w%d >> WSIZE;\n", i - 1, i);
				if (i == 2)
					continue;
				fprintf(finfo[FN_OUTPUT].fp,
					"\t\tw%d &= WMASK;\n", i - 1);
			}
			fprintf(finfo[FN_OUTPUT].fp, "\t\tw%d &= WMASK;\n",
				len - 1);
		}
		else
		{
			for (i = 2; i <= len; i++)
			{
				fprintf(finfo[FN_OUTPUT].fp, "\t\tw%d = w%d;\n",
					i - 1, i);
			}
		}
		fprintf(finfo[FN_OUTPUT].fp, "\t\tw%d = 0;\n", len);
		return 0;
	}
	i = strlen(p);
	if (p[i - 3] != '*')
	{
		fprintf(stderr, "bad scaling lvalue for %s", p);
		return -1;
	}
	p[i - 3] = '\0';
	scale++;
	fprintf(finfo[FN_OUTPUT].fp, "\tif (%s < 0)\n\t{\n", scale);
	fprintf(finfo[FN_OUTPUT].fp,
		"\t\t%s += WSIZE;\n\t\tw1 = (w%d << %s) & WMASK;\n\t\tw%d = 0;\n",
		scale, len + 1, scale, len + 1);
	for (i = 2; i < len; i++)
	{
		fprintf(finfo[FN_OUTPUT].fp, "\t\tw%d <<= %s;\n\t\t", i, scale);
		fprintf(finfo[FN_OUTPUT].fp,
			"w%d |= w%d >> WSIZE;\n\t\tw%d &= WMASK;\n",
			i - 1, i, i);
	}
	fprintf(finfo[FN_OUTPUT].fp, "\t\tw%d |= w%d >> (WSIZE - %s);\n",
		len - 1, len, scale);
	fprintf(finfo[FN_OUTPUT].fp, "\t}\n\telse if (%s > 0)\n\t{\n", scale);
	for (i = 1; i < len; i++)
	{
		fprintf(finfo[FN_OUTPUT].fp, "\t\tw%d = w%d << %s;\n",
			i, i + 1, scale);
	}
	for (i = 2; i < len; i++)
	{
		fprintf(finfo[FN_OUTPUT].fp, "\t\tw%d |= w%d >> WSIZE;\n",
			i - 1, i);
		if (i == 2)
			continue;
		fprintf(finfo[FN_OUTPUT].fp, "\t\tw%d &= WMASK;\n", i - 1);
	}
	fprintf(finfo[FN_OUTPUT].fp, "\t\tw%d &= WMASK;\n\t}\n\telse\n\t{\n",
		len - 1);
	for (i = 2; i <= len; i++)
		fprintf(finfo[FN_OUTPUT].fp, "\t\tw%d = w%d;\n", i - 1, i);
	fprintf(finfo[FN_OUTPUT].fp, "\t}\n\tw%d = 0;\n", len);
	return 0;
}

static int
#ifdef __STDC__
gen_zero(char *p)	/* emit nonzero test of w<n> for rounding */
#else
gen_zero(p)char *p;
#endif
{
	int i, len;

	if (debug)
		fprintf(stderr, "gen_zero()\n");
	if (p[3] == 'D')
		len = dbl_inputlen;
	else if ((len = ldbl_inputlen) == 0)
		return 0;
	fputs("\t\tif (w2 != 0", finfo[FN_OUTPUT].fp);
	for (i = 2; i <= len; i++)
		fprintf(finfo[FN_OUTPUT].fp, " || w%d != 0", i + 1);
	fputs(")\n\t\t\tgoto round;\n", finfo[FN_OUTPUT].fp);
	return 0;
}

static int
#ifdef __STDC__
generate(void)	/* process template file */
#else
generate()
#endif
{
	char line[BUFSIZ];

	if (debug)
		fprintf(stderr, "generate()\n");
	while (fgets(line, BUFSIZ, finfo[FN_TEMPLATE].fp) != 0)
	{
		register struct cval *cp;
		int len;
		long now_at;

		if (strncmp(line, "/*CVT", 5) != 0)
		{
			register char *p, *q = line;

			while ((p = strchr(q, '$')) != 0)
			{
				*p++ = '\0';
				fputs(q, finfo[FN_OUTPUT].fp);
				switch (*p++)
				{
				case '$':
					p--;
					break;
				case 'B':
					fprintf(finfo[FN_OUTPUT].fp, "%ld",
						fract_exp2);
					break;
				case 'E':
					fprintf(finfo[FN_OUTPUT].fp, "%ld",
						dbl_inputlen + 1);
					break;
				case 'F':
					fprintf(finfo[FN_OUTPUT].fp, "%ld",
						fract_digits);
					break;
				case 'I':
					fputs("#ident\t\"@(#)libc-port:fmt/mk_cvt.c	1.2\"",
						finfo[FN_OUTPUT].fp);
					break;
				case 'L':
					fprintf(finfo[FN_OUTPUT].fp, "%ld",
						LOG2TO10(longsize - 1));
					break;
				case 'M':
					fprintf(finfo[FN_OUTPUT].fp, "%ld",
						ldbl_inputlen + 1);
					break;
				case 'O':
					fputs(finfo[FN_OUTPUT].name,
						finfo[FN_OUTPUT].fp);
					break;
				case 'W':
					fprintf(finfo[FN_OUTPUT].fp, "%ld",
						wsize);
					break;
				default:
					fprintf(stderr,
						"unknown $-variable: $%c\n",
						p[-1]);
					return -1;
				}
				q = p;
			}
			fputs(q, finfo[FN_OUTPUT].fp);
			continue;
		}
		if (debug)
			fprintf(stderr, "REPLACE:%s", line);
		for (cp = &config[0];; cp++)
		{
			if (cp->name == 0)
			{
				fprintf(stderr, "bad template line: %s",
					line);
				return -1;
			}
			len = strlen(cp->name);
			if (strncmp(&line[2], cp->name, len) == 0 &&
				(line[2 + len] == ':' || line[2 + len] == '*'))
			{
				break;
			}
		}
		if (cp->loc[0] < 0)
		{
			if (cp->gen == 0)
				fputs(line, finfo[FN_OUTPUT].fp);
			else if ((*cp->gen)(&line[2]) != 0)
				return -1;
			continue;
		}
		else if (cp->nval != 0)
			fprintf(finfo[FN_OUTPUT].fp, "%ld%s", cp->loc[1], line);
		else if (cp->sval != 0)
			fprintf(finfo[FN_OUTPUT].fp, "%s%s", *cp->sval, line);
		else if (fseek(finfo[FN_CONFIG].fp, cp->loc[0], SEEK_SET) != 0)
		{
			perror("seek failed in config file");
			return -1;
		}
		else
		{
			for (now_at = cp->loc[0]; now_at < cp->loc[1];
				now_at += strlen(line))
			{
				if (fgets(line, BUFSIZ, finfo[FN_CONFIG].fp) == 0)
				{
					perror("unexpected EOF in config file");
					return -1;
				}
				fputs(&line[1], finfo[FN_OUTPUT].fp);
			}
		}
	}
	return 0;
}

int
#ifdef __STDC__
main(int argc, char **argv)
#else
main(argc, argv)int argc; char **argv;
#endif
{
	extern char *optarg;
	extern int optind;
	int i;

	for (;;)
	{
		switch (getopt(argc, argv, "c:o:s:t:d"))
		{
		default:
		usage:;
			fprintf(stderr, usage_fmtstr, argv[0]);
			return 2;
		case 'c':
			finfo[FN_CONFIG].name = optarg;
			continue;
		case 'o':
			finfo[FN_OUTPUT].name = optarg;
			continue;
		case 's':
			script_fname = optarg;
			continue;
		case 't':
			finfo[FN_TEMPLATE].name = optarg;
			continue;
		case 'd':
			debug++;
			continue;
		case EOF:
			break;
		}
		break;
	}
	if (argc > optind)
		goto usage;
	for (i = 0; i < FN_end; i++)
	{
		if ((finfo[i].fp = fopen(finfo[i].name, finfo[i].mode)) == 0)
		{
			fputs("cannot open ", stderr);
			perror(finfo[i].name);
			return 2;
		}
	}
	i = 0;
	if (getconfig() || check() || mktables() || generate())
		i = 2;
	if (tables_fp != 0)
		pclose(tables_fp);
	if (tfile_fname != 0)
		remove(tfile_fname);
	return i;
}
