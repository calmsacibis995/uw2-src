/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:fmt/_idoscan.c	1.12"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <values.h>
#ifdef __STDC__
#   include <stdlib.h>
#   include <stdarg.h>
#   include <float.h>
#   include <limits.h>
#else /* use some approximations instead */
    extern char *malloc();
    extern void free();
#   include <varargs.h>
#   define CHAR_BIT	BITSPERBYTE
#   define UCHAR_MAX	((1 << CHAR_BIT) - 1)
#   define SHRT_MAX	0x7fff			/* have to guess: 16 bits? */
#   define SHRT_MIN	(~0x7fff)		/* assuming 2's complement */
#   define LONG_MIN	((long)~(~(Ulong)0 >> 1)) /* ibid */
#   define DBL_MANT_DIG	DSIGNIF
#   define LDBL_MANT_DIG LDSIGNIF
#   define DBL_DIG	(DSIGNIF / 3)		/* 3+ binary digits/decimal */
#   define LDBL_DIG	(LDSIGNIF / 3)		/* 3+ binary digits/decimal */
#endif
#include <locale.h>
#include "_locale.h"
#include "stdiom.h"
#include "format.h"
#include "qstr.h"

	/*
	* This code, unlike most of the portable I/O library, takes
	* advantage of the ISONEBYTE macro which determines whether
	* a byte is a complete multibyte character.  It also assumes
	* that all multibyte characters that can comprise numbers
	* (+-0-9a-fA-FpP) must be such single bytes.
	*/
#ifdef WIDE
#   include "wcharm.h"
#   ifdef ALLWIDE
#	define FCNNAME	_iwsdoscan
#	define ISSPACE(c) iswspace(c)
#	define INT	wint_t
#	define UCHAR	wuchar_t
#   else
#	define FCNNAME	_iwdoscan
#	define ISSPACE(c) isspace(c)
#	define INT	int
#	define UCHAR	Uchar
#   endif
#   define CHAR		wchar_t
#   define ISDIGIT(c)	('0' <= (c) && (c) <= '9')
#   define ISXDIGIT(c)	((c) <= UCHAR_MAX && isxdigit(c))
#   define PARGLIST	_wparglist
#else
#   define FCNNAME	_idoscan
#   define ISSPACE(c)	isspace(c)
#   define INT		int
#   define UCHAR	Uchar
#   define CHAR		char
#   define ISDIGIT(c)	isdigit(c)
#   define ISXDIGIT(c)	isxdigit(c)
#   define PARGLIST	_parglist
#endif

#define F_EOF	0x0001	/* GET() returned EOF (or sometimes bad multibyte) */
#define F_HALF	0x0002	/* 'h' */
#define F_LONG	0x0004	/* 'l' */
#define F_LDBL	0x0008	/* 'L' or 'll' */
#define F_STAR	0x0010	/* '*' */
#define F_UINT	0x0020	/* unsigned integer destination */
#define F_VPTR	0x0040	/* void * "integer" destination */
#define F_NEG	0x0080	/* negate resulting value; complemented scan set */
#define F_ALLOC	0x0100	/* result is allocated string */
#define F_SIGN	0x0200	/* seen leading [+-] for numeric value */
#define F_ZERO	0x0400	/* seen leading 0[bBxX] for integer value */

typedef struct
{
	size_t		width;	/* max number of bytes/characters to scan */
	size_t		total;	/* number of bytes/characters processed */
	Uint		flags;	/* the above bits */
	Uint		uival;	/* base for integer; array length for float */
	union
	{
		Ulong	ul;
		long	l;
#ifndef NO_MSE
		wchar_t	*list;
#endif
		UCHAR	*str;
	} ret;			/* "return value" shapes */
#ifndef NO_LONG_LONG_EMULATE
	Ulong		high;
#endif
} Scan;

#ifdef ALLWIDE

static wint_t
#ifdef __STDC__
dogetw(FILE *fp, Scan *sp)	/* wide string version of getwc() */
#else
dogetw(fp, sp)FILE *fp; Scan *sp;
#endif
{
	BFILE *bp = (BFILE *)fp->_base;
	wint_t wi;
	size_t sz;

	sp->flags &= ~F_EOF;		/* assume that there are more */
	if (fp->_ptr < bp->endptr)	/* still are wide characters */
	{
		wi = *(wchar_t *)fp->_ptr;
		fp->_ptr += sizeof(wchar_t);
		if ((sz = (bp->endptr - fp->_ptr) / sizeof(wchar_t)) > INT_MAX)
			sz = INT_MAX;
		fp->_cnt = sz;
		return wi;
	}
	sp->flags |= F_EOF;
	return EOF;
}

#define GET(fp, sp)	(--(fp)->_cnt < 0 ? dogetw(fp, sp) : \
				((fp)->_ptr += sizeof(wchar_t), \
				*(wchar_t *)&(fp)->_ptr[-sizeof(wchar_t)]))
#define UNGET(fp, sp)	((void)(((sp)->flags & F_EOF) || \
				((fp)->_ptr -= sizeof(wchar_t), ++(fp)->_cnt)))

#else /*!ALLWIDE*/

static int
#ifdef __STDC__
dogetb(FILE *fp, Scan *sp)	/* altered version of getc() */
#else
dogetb(fp, sp)FILE *fp; Scan *sp;
#endif
{
	BFILE *bp = (BFILE *)fp->_base;
	int ch;

	sp->flags &= ~F_EOF;	/* assume that there are more */
	if (bp->fd < 0)		/* string-based _idoscan */
	{
		if (fp->_ptr < bp->endptr)	/* still are characters */
		{
			size_t sz;

			ch = *fp->_ptr++;
			if ((sz = bp->endptr - fp->_ptr) > INT_MAX)
				sz = INT_MAX;
			fp->_cnt = sz;
			return ch;
		}
		sp->flags |= F_EOF;
		return EOF;
	}
	if ((ch = _ifilbuf(fp)) == EOF)
		sp->flags |= F_EOF;
	return ch;
}

#define GET(fp, sp)	(--(fp)->_cnt < 0 ? dogetb(fp, sp) : *(fp)->_ptr++)
#define UNGET(fp, sp)	((void)(((sp)->flags & F_EOF) || \
				(--(fp)->_ptr, ++(fp)->_cnt)))

#if defined(WIDE) || !defined(NO_MSE)

static int
#ifdef __STDC__
doread(FILE *fp)	/* front end to dogetb() for _inwc() */
#else
doread(fp)FILE *fp;
#endif
{
	Scan field;	/* value ignored */

	return dogetb(fp, &field);
}

#endif /*defined(WIDE) || !defined(NO_MSE)*/

#endif /*ALLWIDE*/

static int
#ifdef __STDC__
scanint(register FILE *fp, register Scan *sp)	/* process integer field */
#else
scanint(fp, sp)register FILE *fp; register Scan *sp;
#endif
{
	register Ulong ul;
	register INT ch;

#ifndef NO_LONG_LONG_EMULATE
	sp->high = 0;
#endif
	/*
	* Gather any sign and base-specifying prefix
	* until the first digit of the integer has been scanned.
	*/
	for (;;)
	{
		switch (ch = GET(fp, sp))
		{
		errback:;
			UNGET(fp, sp);
			return 0;
		retback:;
			UNGET(fp, sp);
			sp->ret.ul = 0;
			return 1;
		default:
			if (sp->flags & (F_SIGN | F_ZERO))
				goto errback;
#if defined(WIDE) && !defined(ALLWIDE)
			if (!ISONEBYTE(ch))
			{
				wint_t wi = ch;

				if ((ch = _inwc(&wi, fp, &doread)) <= 0)
				{
					sp->flags |= F_EOF;
					return 0;
				}
				if (!iswspace(wi))
				{
					_unwc(fp, wi, ch);
					return 0;
				}
			}
			else
#endif /*defined(WIDE) && !defined(ALLWIDE)*/
			if (!ISSPACE(ch))
				goto errback;
			sp->total++;
			continue;
		case '-':
			sp->flags |= F_NEG;
			/*FALLTHROUGH*/
		case '+':
			if (sp->flags & (F_SIGN | F_ZERO))
				goto errback;
			if (--sp->width == 0)
				return 0;
			sp->total++;
			sp->flags |= F_SIGN;
			continue;
		case '0':
			if (sp->uival == 10 || sp->uival == 8
				|| (sp->flags & F_ZERO))
			{
				ul = 0;
				break;
			}
			sp->total++;
			if (--sp->width == 0)
			{
				sp->ret.ul = 0;
				return 1;
			}
			switch (ch = GET(fp, sp))
			{
			default:
				if (sp->uival == 0)
					sp->uival = 8;
				ul = 0;
				goto gotdig;
			case 'b':
			case 'B':
				if (sp->uival == 16)
				{
					ul = 0;
					goto gotdig;
				}
				if (sp->width == 1 && sp->uival == 0)
					goto retback;
				sp->uival = 2;
				break;
			case 'x':
			case 'X':
				if (sp->uival == 2
					|| sp->width == 1 && sp->uival == 0)
				{
					goto retback;
				}
				sp->uival = 16;
				break;
			}
			if (--sp->width == 0)
				return 0;
			sp->total++;
			sp->flags |= F_ZERO;
			continue;
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			ul = ch - '0';
			if (sp->uival == 0)
				sp->uival = 10;
			else if (ul >= sp->uival)
				goto errback;
			break;
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
			ul = ch - 'a' + 10;
			if (sp->uival != 16)
				goto errback;
			break;
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
			ul = ch - 'A' + 10;
			if (sp->uival != 16)
				goto errback;
			break;
		}
		break;
	}
	/*
	* Any leading sign, base-specific prefix, and the first digit
	* of the value have been scanned and handled.
	* Accumulate the value in a Ulong (or Ulong pair when
	* emulating "long long"), ignoring any overflows.
	*/
	while (++sp->total, --sp->width != 0)
	{
		ch = GET(fp, sp);
	gotdig:;
#ifdef ALLWIDE
		if (ch > UCHAR_MAX)
			ch = sp->uival;	/* bad value below */
		else
#endif
		if (isdigit(ch))
			ch -= '0';
		else if (!isxdigit(ch))
			ch = sp->uival;	/* bad value below */
		else if (isupper(ch))
			ch -= 'A' - 10;
		else
			ch -= 'a' - 10;
		if (ch >= sp->uival)
		{
			UNGET(fp, sp);
			break;
		}
#ifndef NO_LONG_LONG_EMULATE
		{
			Ulong carry = ch;

			ul = _muladd(&carry, 0L, (Ulong)sp->uival, ul);
			sp->high = _muladd(&carry, 0L, (Ulong)sp->uival,
				sp->high);
		}
#else
		ul *= sp->uival;
		ul += ch;
#endif
	}
	sp->ret.ul = ul;	/* this covers signed values, too */
	return 1;
}

#ifndef NO_NCEG_FPE
#   define NANSIZE	(3 + 1 + NDIG_HEX + 1 + 1)
#else
#   define NANSIZE	0
#endif

#define LP	'('
#define RP	')'
		
static int
#ifdef __STDC__
scanflt(register FILE *fp, register Scan *sp)	/* process floating field */
#else
scanflt(fp, sp)register FILE *fp; register Scan *sp;
#endif
{
	register UCHAR *p;
	register INT ch;
	UCHAR *beg, *end;
	enum	/* states during a floating value recognition */
	{
		FS_IntDigits,		/* stable: decimal integer digits */
		FS_DecPoint,
		FS_FracDigits,		/* stable: decimal fraction digits */
#ifndef NO_NCEG_FPE
		FS_ZeroDigit = FS_FracDigits + 2, /* stable: just 0 */
		FS_HexStart,
		FS_HexIntDigits,	/* stable: hex integer digits */
		FS_HexPoint,
		FS_HexFracDigits,	/* stable: hex fraction digits */
#endif
		FS_ExpStart,
		FS_ExpDigits,		/* stable: exponent digits */
		FS_ExpSign
	};
	int fs_state;	/* must end on an even state (...Digit[s]) */

	/*
	* Handle special cases as determined by the first character.
	* Only loop for +, -, and white space characters.
	*/
	for (;;)
	{
		switch (ch = GET(fp, sp))
		{
		errback:;
			UNGET(fp, sp);
			return 0;
#ifndef NO_NCEG_FPE
		case 'i':
		case 'I':
			/*
			* Check for "inf" or "infinity", case insensitive.
			*/
			if (--sp->width == 0)
				return 0;
			if ((ch = GET(fp, sp)) != 'n' && ch != 'N')
				goto errback;
			if (--sp->width == 0)
				return 0;
			if ((ch = GET(fp, sp)) != 'f' && ch != 'F')
				goto errback;
			if (--sp->width == 0)
			{
				sp->total += 3;
				goto gotinf;
			}
			if ((ch = GET(fp, sp)) != 'i' && ch != 'I')
			{
				UNGET(fp, sp);
				sp->total += 3;
				goto gotinf;
			}
			{
				register const char *s = &_str__inity[2];

				do
				{
					if (--sp->width == 0)
						return 0;
					if ((ch = GET(fp, sp)) != s[0]
						&& ch != s[1])
					{
						goto errback;
					}
					s += 2;
				} while (*s != '\0');
			}
			sp->total += 8;
		gotinf:;
#ifdef ALLWIDE
			sp->ret.str = (UCHAR *)_wcs_lc_inf;
#else
			sp->ret.str = (UCHAR *)_str_lc_inf;
#endif
			return 1;
		case 'n':
		case 'N':
			/*
			* Check for "nan", case insensitive,
			* possibly followed by "(hex-digits-opt)".
			* If the parens are present, the string is
			* copied into _idoscan's local array, which
			* must have length of at least NANSIZE.
			*/
			if (--sp->width == 0)
				return 0;
			if ((ch = GET(fp, sp)) != 'a' && ch != 'A')
				goto errback;
			if (--sp->width == 0)
				return 0;
			if ((ch = GET(fp, sp)) != 'n' && ch != 'N')
				goto errback;
			if (--sp->width == 0)
			{
			gotnan:;
				sp->total += 3;
#ifdef ALLWIDE
				sp->ret.str = (UCHAR *)_wcs_lc_nan;
#else
				sp->ret.str = (UCHAR *)_str_lc_nan;
#endif
				return 1;
			}
			if (GET(fp, sp) != LP)
			{
				UNGET(fp, sp);
				goto gotnan;
			}
			if (--sp->width == 0)
				return 0;
			sp->total += 3 + 1;
			/*
			* Copy in prefix and leading paren.
			*/
#ifdef ALLWIDE
			beg = (UCHAR *)_wcs_lc_nan;
#else
			beg = (UCHAR *)_str_lc_nan;
#endif
			p = sp->ret.str;
			while ((*p++ = *beg++) != '\0')
				;
			*--p = LP;
			while ((ch = GET(fp, sp)) != RP)
			{
#ifdef WIDE
				if (!ISONEBYTE(ch))
					goto errback;
#endif
				if (!isalnum(ch) && ch != '_')
					goto errback;
				if (--sp->width == 0)
					return 0;
				sp->total++;
				if (!isxdigit(ch))
					continue;
				if (p - sp->ret.str >= NANSIZE - 3)
					continue;
				*++p = ch;
			}
			*++p = RP;
			*++p = '\0';
			return 1;
		case '0':
			fs_state = FS_ZeroDigit;
			break;
#else /*!NO_NCEG_FPE*/
		case '0':
#endif /*NO_NCEG_FPE*/
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			fs_state = FS_IntDigits;
			break;
		case '-':
			sp->flags |= F_NEG;
			/*FALLTHROUGH*/
		case '+':
			if (sp->flags & F_SIGN)
				goto errback;
			if (--sp->width == 0)
				return 0;
			sp->total++;
			sp->flags |= F_SIGN;
			continue;
		default:
			if (ch == _numeric[0])
			{
				fs_state = FS_DecPoint;
				break;
			}
			if (sp->flags & F_SIGN)
				goto errback;
#if defined(WIDE) && !defined(ALLWIDE)
			if (!ISONEBYTE(ch))
			{
				wint_t wi = ch;

				if ((ch = _inwc(&wi, fp, &doread)) <= 0)
				{
					sp->flags |= F_EOF;
					return 0;
				}
				if (!iswspace(wi))
				{
					_unwc(fp, wi, ch);
					return 0;
				}
			}
			else
#endif /*defined(WIDE) && !defined(ALLWIDE)*/
			if (!ISSPACE(ch))
				goto errback;
			sp->total++;
			continue;
		}
		break;
	}
	/*
	* First character of a regular floating value has been seen.
	* Most of the time, share the result string with the input
	* buffer.  Copy only if conversion can happen (!F_STAR) and
	*  1. The string spans input buffers.  Or
	*  2. The field width limit is reached.
	* If copied, first use buffer provided by _i[w[s]]doscan.
	* Use malloc'd buffers only when it is too short.
	*/
	p = (UCHAR *)fp->_ptr;
	beg = p - 1;
	end = p + fp->_cnt;
	while (++sp->total, --sp->width != 0)
	{
		/*
		* Get the next input unit into ch.
		*/
		if (p == (UCHAR *)fp->_ptr) /* still sharing the source buffer */
		{
			/*
			* Our own GET:
			* Make sure we control when doget[bw]() is called.
			*/
			if (--fp->_cnt >= 0) /* at least one more available */
			{
#ifdef ALLWIDE
				fp->_ptr += sizeof(wchar_t);
#else
				fp->_ptr++;
#endif
				ch = *p++;
			}
			else if (sp->flags & F_STAR) /* don't really need it */
			{
#ifdef ALLWIDE
				ch = dogetw(fp, sp);	/* refill the buffer */
#else
				ch = dogetb(fp, sp);	/* refill the buffer */
#endif
				p = (UCHAR *)fp->_ptr;
				beg = p - 1;
				end = p + fp->_cnt;
			}
			else	/* copy the string so far */
			{
				goto copy;
			}
		}
		else	/* keeping a copy of the string */
		{
			if (p == end)	/* no more room (and must need string) */
			{
				size_t n, sz;

			copy:;
				/*
				* Really want number of bytes for n.
				*/
				n = (Uchar *)p - (Uchar *)beg;
				if (n < sp->uival)	/* fits in local */
				{
					sz = sp->uival;
					p = sp->ret.str;
					memcpy((void *)p, (void *)beg, n);
				}
				else /* allocate buffer rounded to mult. of 32 */
				{
					sz = ((n >> 5) + 1) << 5;
#ifdef ALLWIDE
					/*CONSTANTCONDITION*/
					if ((1 << 5) % sizeof(wchar_t) != 0
						|| (1 << 5) < sizeof(wchar_t))
					{
						sz /= sizeof(wchar_t);
						sz *= sizeof(wchar_t);
					}
#endif
					if ((p = (UCHAR *)malloc(sz)) == 0)
					{
						if (sp->flags & F_ALLOC)
							free((void *)beg);
						return 0; /* errno = ERANGE?? */
					}
					memcpy((void *)p, (void *)beg, n);
					if (sp->flags & F_ALLOC)
						free((void *)beg);
					sp->flags |= F_ALLOC;
				}
				/*
				* Reset beg, end, and p.  Note that n and sz
				* always count bytes, not number of input units.
				*/
				beg = p;
				end = (UCHAR *)(sz + (Uchar *)p);
				p = (UCHAR *)(n + (Uchar *)p);
				if (sp->width == 0)	/* from below */
					break;
			}
			*p++ = ch = GET(fp, sp);
		}
		/*
		* Run character through the state machine
		* to determine whether it still matches.
		*/
		switch (fs_state)
		{
#ifndef NO_NCEG_FPE
		case FS_ZeroDigit:
			if (sp->width != 1 && (ch == 'x' || ch == 'X'))
			{
				fs_state = FS_HexStart;
				continue;
			}
			fs_state = FS_IntDigits;
			/*FALLTHROUGH*/
#endif
		case FS_IntDigits:
			if (ch == _numeric[0])
			{
				fs_state = FS_FracDigits;
				continue;
			}
			/*FALLTHROUGH*/
		case FS_FracDigits:
			if (ch == 'e' || ch == 'E')
			{
				fs_state = FS_ExpStart;
				continue;
			}
			/*FALLTHROUGH*/
		case FS_ExpDigits:
			if (ISDIGIT(ch))
				continue;
			UNGET(fp, sp);
			sp->ret.str = beg;
			return 1;
#ifndef NO_NCEG_FPE
		case FS_HexIntDigits:
			if (ch == _numeric[0])
			{
				fs_state = FS_HexFracDigits;
				continue;
			}
			/*FALLTHROUGH*/
		case FS_HexFracDigits:
			if (ch == 'p' || ch == 'P')
			{
				fs_state = FS_ExpStart;
				continue;
			}
			if (ISXDIGIT(ch))
				continue;
			UNGET(fp, sp);
			sp->ret.str = beg;
			return 1;
		case FS_HexPoint:
			if (ISXDIGIT(ch))
			{
				fs_state = FS_HexFracDigits;
				continue;
			}
			break;
		case FS_HexStart:
			if (ch == _numeric[0])
			{
				fs_state = FS_HexPoint;
				continue;
			}
			if (ISXDIGIT(ch))
			{
				fs_state = FS_HexIntDigits;
				continue;
			}
			break;
#endif /*NO_NCEG_FPE*/
		case FS_DecPoint:
			if (ISDIGIT(ch))
			{
				fs_state = FS_FracDigits;
				continue;
			}
			break;
		case FS_ExpStart:
#ifndef NO_CI4
			if (ch == '-' || ch == '+'
				|| ch == ' ' && _lib_version == c_issue_4)
#else
			if (ch == '-' || ch == '+')
#endif
			{
				fs_state = FS_ExpSign;
				continue;
			}
			/*FALLTHROUGH*/
		case FS_ExpSign:
			if (ISDIGIT(ch))
			{
				fs_state = FS_ExpDigits;
				continue;
			}
			break;
		}
		/*
		* Only here when mismatched character occurs at an
		* unstable state.
		*/
		UNGET(fp, sp);
		if (sp->flags & F_ALLOC)
			free((void *)beg);
		return 0;
	}
	/*
	* Only here when the length limit has been reached.
	* Make sure that there's a termination if the string will be converted.
	*/
	if (fs_state & 0x1)	/* odd: unstable state */
	{
		if (sp->flags & F_ALLOC)
			free((void *)beg);
		return 0;
	}
	if ((sp->flags & F_STAR) == 0)
	{
		if (p == (UCHAR *)fp->_ptr || p == end)
			goto copy;	/* will come back again */
		*p = '\0';
	}
	sp->ret.str = beg;
	return 1;
}

#define NBYTE	(1 << CHAR_BIT)	/* assumed size for byte scan table */

static const CHAR *
#ifdef __STDC__
makeset(register const CHAR *fmt, Scan *sp) /* fill for '[' scan */
#else
makeset(fmt, sp)register const CHAR *fmt; Scan *sp;
#endif
{
#ifdef WIDE
	/*
	* Do no real initial processing of the scan set.
	* The code in checkset will handle ranges, for example.
	*/
	sp->ret.list = (wchar_t *)++fmt;
	if (*fmt == '^') /* complement set */
	{
		sp->flags |= F_NEG;
		sp->ret.list = (wchar_t *)++fmt;
	}
	do
	{
		if (*fmt == 0) /* unexpected end of format */
			return 0;
	} while (*++fmt != ']');
	sp->uival = fmt - sp->ret.list;
#else /*!WIDE*/
	/*
	* For regular scanf's %[...], fill in a table of bytes that
	* represent the byte values that are accepted (indexed).
	* For %l[...], convert multibyte characters to wide characters
	* into the same table, allocating space if the provided table
	* is too short.
	*/
#ifndef NO_MSE
	if (sp->flags & F_LONG)
	{
		wchar_t *list, *high;
		wint_t ch;
		int n;

		list = (wchar_t *)sp->ret.str;
		high = (wchar_t *)&sp->ret.str[sp->uival];
		sp->ret.list = list;
		if ((ch = *(Uchar *)++fmt) == '^') /* complement set */
		{
			sp->flags |= F_NEG;
			ch = *(Uchar *)++fmt;
		}
		do
		{
			if (!ISONEBYTE(ch))
			{
				if ((n = _mb2wc(&ch, 1 + (Uchar *)fmt)) > 0)
					fmt += n;
			}
			else if (ch == '\0') /* unexpected end of format */
			{
			err:;
				if (sp->flags & F_ALLOC)
					free((void *)sp->ret.list);
				return 0;
			}
			if (list >= high) /* need more room */
			{
				if ((list = (wchar_t *)malloc(sp->uival
					+ 128 * sizeof(wchar_t))) == 0)
				{
					goto err;
				}
				memcpy((void *)list, (void *)sp->ret.list,
					(size_t)sp->uival);
				high = (wchar_t *)(sp->uival + (Uchar *)list);
				sp->uival += 128 * sizeof(wchar_t);
				if (sp->flags & F_ALLOC)
					free((void *)sp->ret.list);
				sp->flags |= F_ALLOC;
				sp->ret.list = list;
				list = high;
				high += 128;
			}
			*list++ = ch;
		} while ((ch = *(Uchar *)++fmt) != ']');
		sp->uival = list - sp->ret.list;
	}
	else /* classic byte table */
#endif /*NO_MSE*/
	{
		register Uchar *tab = sp->ret.str;
		register int ch, prev, fill;

		fill = 1;
		if ((ch = *(Uchar *)++fmt) == '^') /* complement set */
		{
			fill = 0;
			ch = *(Uchar *)++fmt;
		}
		memset((void *)tab, fill ^ 1, (size_t)NBYTE);
		if (ch == ']' || ch == '-') /* initial ] or - are special */
		{
			tab[ch] = fill;
			ch = *(Uchar *)++fmt;
		}
		while (ch != ']')
		{
			if (ch == '\0') /* unexpected end of format */
				return 0;
			if (ch == '-')
			{
				if ((ch = *(Uchar *)++fmt) != ']'
					&& (prev = *(Uchar *)&fmt[-2]) < ch)
				{
					do
						tab[++prev] = fill;
					while (prev < ch);
					ch = *(Uchar *)++fmt;
					continue;
				}
				tab['-'] = fill;
				continue;
			}
			tab[ch] = fill;
			ch = *(Uchar *)++fmt;
		}
		tab['\0'] = 0; /* null characters never match */
	}
#endif /*WIDE*/
	return fmt;
}

#ifndef NO_MSE

static int
#ifdef __STDC__
checkset(register wint_t wc, Scan *sp)
#else
checkset(wc, sp)register wint_t wc; Scan *sp;
#endif
{
	register const wchar_t *lp = sp->ret.list;
	const wchar_t *last;
	int ret = 1;

	if (sp->flags & F_NEG)
	{
		if (wc == 0) /* complemented set does not include null */
			return 0;
		ret = 0;
	}
	last = &lp[sp->uival - 1];
	while (lp <= last)
	{
		if (*lp == '-' && lp != sp->ret.list && lp != last)
		{
			/*
			* See if the character is between the
			* end points (lp[-1]==wc already done).
			*/
			if (lp[-1] < wc && wc <= lp[1])
			{
#ifdef EUCMASK
				if (utf8 || (EUCMASK & lp[-1]) == (EUCMASK & lp[1]))
#endif /*EUCMASK*/
					goto match;
			}
			lp += 2;
			continue;
		}
		if (*lp++ == wc)
			goto match;
	}
	ret ^= 1; /* not found */
match:;
	return ret;
}

#endif /*NO_MSE*/

#define NARGS	30	/* cache positions of this many arguments */

#ifdef WIDE
#   define BYTELEN	(128 < NANSIZE ? NANSIZE : 128)
#else
#   define BYTELEN	(NBYTE < NANSIZE ? NANSIZE : NBYTE)
#endif

#ifdef NO_MSE
#   define BUFTYPE	Uchar
#else
#   define BUFTYPE	wchar_t
#endif

#define BUFLEN	((BYTELEN + sizeof(BUFTYPE) - 1) / sizeof(BUFTYPE))

int
#ifdef __STDC__
FCNNAME(register FILE *fp, register const CHAR *fmt, va_list ap)
#else
FCNNAME(fp, fmt, ap)register FILE *fp; register const CHAR *fmt; va_list ap;
#endif
{
	const CHAR *orig_fmt = fmt;	/* saved start of format */
	BUFTYPE buf[BUFLEN];		/* floating strings & scan tables */
	RA_va_list arglist[NARGS];	/* cached arg positions */
	RA_va_list curpos;		/* current va_list position */
	int nmatch = 0;			/* number of conversions matched */
	int pos_arg_flag = 1;		/* no positional arguments so far */
	Scan field;			/* current field processing */

	field.total = 0;		/* no bytes/characters yet */
	field.flags = 0;		/* so that F_EOF is clear */
	curpos = *(RA_va_list *)&ap;
	for (;; fmt++)
	{
		/*
		* Scan format string for matching directives.
		* Eat white space given a white-space character.
		* A conversion specification starts with % and
		* is handled below.  Otherwise, the input must
		* match the format byte/character.
		*
		* _idoscan assumes that bytes with the value %
		* and the single-byte white-space characters
		* cannot be part of a multibyte character.
		*/
		for (;; fmt++) /* loop until return or reach % */
		{
			switch (*fmt)
			{
			default:
#if !defined(WIDE) || defined(ALLWIDE)
				if (GET(fp, &field) != *fmt)
				{
					UNGET(fp, &field);
					goto mismatch;
				}
#else /*defined(WIDE) && !defined(ALLWIDE)*/
			{
				wint_t wi;

				if (ISONEBYTE(wi = GET(fp, &field)))
				{
					if (wi != *fmt || wi == EOF)
					{
						UNGET(fp, &field);
						goto mismatch;
					}
				}
				else
				{
					int n;

					if ((n = _inwc(&wi, fp, &doread)) <= 0)
						goto badchar;
					if (wi != *fmt)
					{
						_unwc(fp, wi, n);
						goto mismatch;
					}
				}
			}
#endif /*!defined(WIDE) || defined(ALLWIDE)*/
				field.total++;
				continue;
			case '\0':
				return nmatch;	/* successful */
			case '%':
				break;
			case ' ':
			case '\t':
			case '\n':
			case '\r':
			case '\f':
			case '\v':
#if defined(WIDE) && !defined(ALLWIDE)
				for (;; field.total++)
				{
					register int ch = GET(fp, &field);
					wint_t wi;

					if (ISONEBYTE(ch))	/* EOF too */
					{
						if (isspace(ch))
							continue;
						UNGET(fp, &field);
						break;
					}
					wi = ch;
					if ((ch = _inwc(&wi, fp, &doread)) <= 0)
						goto badchar;
					if (!iswspace(wi))
					{
						_unwc(fp, wi, ch);
						break;
					}
				}
#else /*!defined(WIDE) || defined(ALLWIDE)*/
				{
					register INT ch;

					while (ch = GET(fp, &field), ISSPACE(ch))
						field.total++;
				}
				UNGET(fp, &field);
#endif /*defined(WIDE) && !defined(ALLWIDE)*/
				continue;
			}
			break;	/* only here for % */
		}
		/*
		* Must be at a %.  Process conversion specification.
		*/
		field.flags &= F_EOF;
		field.width = ~(size_t)0;
	again:;
		switch (*++fmt)
		{
		case '*':
			field.flags |= F_STAR;
			goto again;
		case 'h':
			field.flags |= F_HALF;
			goto again;
		case 'l':
#ifndef NO_LONG_LONG_EMULATE
			if (field.flags & F_LONG)
				field.flags |= F_LDBL;
#endif
			field.flags |= F_LONG;
			goto again;
		case 'L':
			field.flags |= F_LDBL;
			goto again;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		{
			register Uint n = *fmt - '0';

			while (++fmt, ISDIGIT(*fmt))	/* ignore any overflow */
			{
				n *= 10;
				n += *fmt - '0';
			}
			if (*fmt != '$')	/* not position spec. */
			{
				fmt--;
				if ((field.width = n) == 0)	/* no zeros */
					field.width = ~(size_t)0;
				goto again;
			}
			if (pos_arg_flag)	/* first time */
			{
				pos_arg_flag = 0;
				arglist[0] = *(RA_va_list *)&ap;
				PARGLIST(orig_fmt, arglist, NARGS, 0, 1);
			}
			if (n > NARGS)
			{
				curpos = arglist[NARGS - 1];
				PARGLIST(orig_fmt, &curpos, NARGS, n, 1);
			}
			else
			{
				if (n == 0)
					n++;
				curpos = arglist[n - 1];
			}
			goto again;
		}
		/*
		* The following cases each end a conversion specification.
		* ('[' still needs to reach its matching ']', however.)
		*/
		default:
			return nmatch;	/* invalid format character */
		case '%':
			if (GET(fp, &field) != '%')
			{
				UNGET(fp, &field);
				goto mismatch;
			}
			field.total++;
			continue;
		case 'n':
			if (field.flags & F_STAR)
				continue;
			if (field.flags & F_LONG)
				*va_arg(curpos.ap, long *) = field.total;
			else if (field.flags & F_HALF)
				*va_arg(curpos.ap, short *) = field.total;
			else
				*va_arg(curpos.ap, int *) = field.total;
			continue;
		case 'c':
		{
			register Uchar *p;
#ifdef WIDE
			wint_t ch;
#else
			int ch;
#endif

#if defined(WIDE) || !defined(NO_MSE)
			if (field.flags & F_LONG)
				goto C_fmt;
#endif
			if (field.width == ~(size_t)0)
				field.width = 1;
#if !defined(WIDE) || defined(ALLWIDE)
			if ((ch = GET(fp, &field)) == EOF)
				goto mismatch;
#else /*defined(WIDE) && !defined(ALLWIDE)*/
			if (ISONEBYTE(ch = GET(fp, &field)))
			{
				if (ch == EOF)
					goto mismatch;
			}
			else if (_inwc(&ch, fp, &doread) <= 0)
				goto badchar;
#endif /*!defined(WIDE) || defined(ALLWIDE)*/
			if (field.flags & F_STAR)
				p = 0;
			else
				p = (Uchar *)va_arg(curpos.ap, char *);
			for (;;)
			{
				if (p != 0)
				{
#ifdef WIDE
					if (!ISONEBYTE(ch))
					{
						p += wctomb((char *)p,
							(wchar_t)ch);
					}
					else
#endif
						*p++ = ch;
				}
				field.total++;
				if (--field.width == 0)
					break;
#if !defined(WIDE) || defined(ALLWIDE)
				if ((ch = GET(fp, &field)) == EOF)
					break;
#else /*defined(WIDE) && !defined(ALLWIDE)*/
				if (ISONEBYTE(ch = GET(fp, &field)))
				{
					if (ch == EOF)
						break;
				}
				else if (_inwc(&ch, fp, &doread) <= 0)
					goto badchar;
#endif /*!defined(WIDE) || defined(ALLWIDE)*/
			}
			if (p != 0)
				nmatch++;
			continue;
		case 's':
#if defined(WIDE) || !defined(NO_MSE)
			if (field.flags & F_LONG)
				goto S_fmt;
#endif
			for (;;) /* skip leading white space */
			{
#if !defined(WIDE) || defined(ALLWIDE)
				if ((ch = GET(fp, &field)) == EOF)
					goto mismatch;
				if (!ISSPACE(ch))
					break;
#else /*defined(WIDE) && !defined(ALLWIDE)*/
				if (ISONEBYTE(ch = GET(fp, &field)))
				{
					if (ch == EOF)
						goto mismatch;
				}
				else if (_inwc(&ch, fp, &doread) <= 0)
					goto badchar;
				if (!iswspace(ch))
					break;
#endif /*!defined(WIDE) || defined(ALLWIDE)*/
				field.total++;
			}
			if (field.flags & F_STAR)
				p = 0;
			else
				p = (Uchar *)va_arg(curpos.ap, char *);
			for (;;)
			{
				if (p != 0)
				{
#ifdef WIDE
					if (!ISONEBYTE(ch))
					{
						p += wctomb((char *)p,
							(wchar_t)ch);
					}
					else
#endif
						*p++ = ch;
				}
				field.total++;
				if (--field.width == 0)
					break;
#if !defined(WIDE) || defined(ALLWIDE)
				if ((ch = GET(fp, &field)) == EOF)
					break;
				if (ISSPACE(ch))
				{
					UNGET(fp, &field);
					break;
				}
#else /*defined(WIDE) && !defined(ALLWIDE)*/
				if (ISONEBYTE(ch = GET(fp, &field)))
				{
					if (ch == EOF)
						break;
					if (isspace(ch))
					{
						UNGET(fp, &field);
						break;
					}
				}
				else
				{
					int sz = _inwc(&ch, fp, &doread);

					if (sz <= 0)
						goto badchar;
					if (iswspace(ch))
					{
						_unwc(fp, ch, sz);
						break;
					}
				}
#endif /*!defined(WIDE) || defined(ALLWIDE)*/
			}
			if (p != 0)
			{
				*p = '\0';
				nmatch++;
			}
			continue;
		}
#if defined(WIDE) || !defined(NO_MSE)
		case 'C':
		{
			wchar_t *wp;
			wint_t ch;
#ifndef ALLWIDE
			int sz;
#endif

		C_fmt:;
			if (field.width == ~(size_t)0)
				field.width = 1;
#ifdef ALLWIDE
			if ((ch = GET(fp, &field)) == EOF)
				goto mismatch;
#else /*!ALLWIDE*/
			if (ISONEBYTE(ch = GET(fp, &field)))
			{
				if (ch == EOF)
					goto mismatch;
				sz = 1;
			}
			else if ((sz = _inwc(&ch, fp, &doread)) <= 0)
				goto badchar;
#endif /*ALLWIDE*/
			if (field.flags & F_STAR)
				wp = 0;
			else
				wp = va_arg(curpos.ap, wchar_t *);
			for (;;)
			{
				if (wp != 0)
					*wp++ = ch;
#ifdef WIDE
				field.total++;
#else
				field.total += sz;
#endif
				if (--field.width == 0)
					break;
#ifdef ALLWIDE
				if ((ch = GET(fp, &field)) == EOF)
					break;
#else /*!ALLWIDE*/
				if (ISONEBYTE(ch = GET(fp, &field)))
				{
					if (ch == EOF)
						break;
					sz = 1;
				}
				else if ((sz = _inwc(&ch, fp, &doread)) <= 0)
					goto badchar;
#endif /*ALLWIDE*/
			}
			if (wp != 0)
				nmatch++;
			continue;
		case 'S':
		S_fmt:;
			/*
			* Skip leading white space, either single-byte
			* or full wide character.
			*/
			for (;;)
			{
#ifdef ALLWIDE
				if ((ch = GET(fp, &field)) == EOF)
					goto mismatch;
				if (!iswspace(ch))
					break;
				field.total++;
#else /*!ALLWIDE*/
				if (ISONEBYTE(ch = GET(fp, &field)))
				{
					if (!isspace(ch))
					{
						if (ch == EOF)
							goto mismatch;
						break;
					}
					field.total++;
					continue;
				}
#ifdef WIDE
				if (_inwc(&ch, fp, &doread) <= 0)
					goto badchar;
				if (!iswspace(ch))
					break;
				field.total++;
#else /*!WIDE*/
				if ((sz = _inwc(&ch, fp, &doread)) <= 0)
					goto badchar;
				if (ch > UCHAR_MAX || !isspace(ch))
					break;
				field.total += sz;
#endif /*WIDE*/
#endif /*ALLWIDE*/
			}
			if (field.flags & F_STAR)
				wp = 0;
			else
				wp = va_arg(curpos.ap, wchar_t *);
			for (;;)
			{
				if (wp != 0)
					*wp++ = ch;
#ifdef WIDE
				field.total++;
#else
				field.total += sz;
#endif
				if (--field.width == 0)
					break;
#ifdef ALLWIDE
				if ((ch = GET(fp, &field)) == EOF)
					break;
				if (iswspace(ch))
				{
					UNGET(fp, &field);
					break;
				}
#else /*!ALLWIDE*/
				if (ISONEBYTE(ch = GET(fp, &field)))
				{
					if (ch == EOF)
						break;
					sz = 1;
				}
				else if ((sz = _inwc(&ch, fp, &doread)) <= 0)
					goto badchar;
#ifdef WIDE
				if (iswspace(ch))
				{
					_unwc(fp, ch, sz);
					break;
				}
#else /*!WIDE*/
				if (ch <= UCHAR_MAX && isspace(ch))
				{
					UNGET(fp, &field); /* sz must be 1 */
					break;
				}
#endif /*WIDE*/
#endif /*ALLWIDE*/
			}
			if (wp != 0)
			{
				*wp = 0;
				nmatch++;
			}
			continue;
		}
#endif /*defined(WIDE) || !defined(NO_MSE)*/
		case '[':
#ifndef WIDE
			field.ret.str = (Uchar *)buf;
			field.uival = sizeof(buf);
#endif
			if ((fmt = makeset(fmt, &field)) == 0)
				goto mismatch;
#ifndef NO_MSE
			if (field.flags & F_LONG)
			{
				register wchar_t *p;
				wint_t ch;
#ifdef ALLWIDE
				if ((ch = GET(fp, &field)) == EOF)
					goto setmismatch;
#else /*!ALLWIDE*/
				int sz;

				sz = 1;
				if (!ISONEBYTE(ch = GET(fp, &field)))
				{
					if ((sz = _inwc(&ch, fp, &doread)) <= 0)
						goto setbadchar;
					if (!checkset(ch, &field))
					{
						_unwc(fp, ch, sz);
						goto setmismatch;
					}
				}
				else if (ch == EOF)
					goto setmismatch;
#endif /*ALLWIDE*/
				else if (!checkset(ch, &field))
				{
					UNGET(fp, &field);
					goto setmismatch;
				}
				if (field.flags & F_STAR)
					p = 0;
				else
					p = va_arg(curpos.ap, wchar_t *);
				for (;;)
				{
					if (p != 0)
						*p++ = ch;
#ifdef WIDE
					field.total++;
#else
					field.total += sz;
#endif
					if (--field.width == 0)
						break;
#ifdef ALLWIDE
					if ((ch = GET(fp, &field)) == EOF)
						break;
#else /*!ALLWIDE*/
					sz = 1;
					if (!ISONEBYTE(ch = GET(fp, &field)))
					{
						sz = _inwc(&ch, fp, &doread);
						if (sz <= 0)
							goto setbadchar;
						if (!checkset(ch, &field))
						{
							_unwc(fp, ch, sz);
							break;
						}
					}
					else if (ch == EOF)
						break;
#endif /*ALLWIDE*/
					else if (!checkset(ch, &field))
					{
						UNGET(fp, &field);
						break;
					}
				}
				if (p != 0)
				{
					*p = 0;
					nmatch++;
				}
			}
			else
#endif /*NO_MSE*/
			{
				register Uchar *p;
#ifdef WIDE
				wint_t ch;

#ifdef ALLWIDE
				if ((ch = GET(fp, &field)) == EOF)
					goto setmismatch;
#else /*!ALLWIDE*/
				if (!ISONEBYTE(ch = GET(fp, &field)))
				{
					int sz = _inwc(&ch, fp, &doread);

					if (sz <= 0)
						goto setbadchar;
					if (!checkset(ch, &field))
					{
						_unwc(fp, ch, sz);
						goto setmismatch;
					}
				}
				else if (ch == EOF)
					goto setmismatch;
#endif /*ALLWIDE*/
				else if (!checkset(ch, &field))
				{
					UNGET(fp, &field);
					goto setmismatch;
				}
#else /*!WIDE*/
				register int ch;

				if ((ch = GET(fp, &field)) == EOF)
					goto setmismatch;
				if (*(ch + (Uchar *)buf) == 0)
				{
					UNGET(fp, &field);
					goto setmismatch;
				}
#endif /*WIDE*/
				if (field.flags & F_STAR)
					p = 0;
				else
					p = (Uchar *)va_arg(curpos.ap, char *);
				for (;;)
				{
					if (p != 0)
					{
#ifdef WIDE
						if (!ISONEBYTE(ch))
						{
							p += wctomb((char *)p,
								(wchar_t)ch);
						}
						else
#endif
							*p++ = ch;
					}
					field.total++;
					if (--field.width == 0)
						break;
#ifdef WIDE
#ifdef ALLWIDE
					if ((ch = GET(fp, &field)) == EOF)
						break;
#else /*!ALLWIDE*/
					if (!ISONEBYTE(ch = GET(fp, &field)))
					{
						int sz = _inwc(&ch, fp, &doread);

						if (sz <= 0)
							goto setbadchar;
						if (!checkset(ch, &field))
						{
							_unwc(fp, ch, sz);
							break;
						}
					}
					else if (ch == EOF)
						break;
#endif /*ALLWIDE*/
					else if (!checkset(ch, &field))
					{
						UNGET(fp, &field);
						break;
					}
#else /*!WIDE*/
					if ((ch = GET(fp, &field)) == EOF)
						break;
					if (*(ch + (Uchar *)buf) == 0)
					{
						UNGET(fp, &field);
						break;
					}
#endif /*WIDE*/
				}
				if (p != 0)
				{
					*p = '\0';
					nmatch++;
				}
			}
#if !defined(WIDE) && !defined(NO_MSE)
			if (field.flags & F_ALLOC)
				free((void *)field.ret.list);
#endif
			continue;
		setmismatch:;
#if !defined(WIDE) && !defined(NO_MSE)
			if (field.flags & F_ALLOC)
				free((void *)field.ret.list);
#endif
			goto mismatch;
#ifndef ALLWIDE
		setbadchar:;
#if !defined(WIDE) && !defined(NO_MSE)
			if (field.flags & F_ALLOC)
				free((void *)field.ret.list);
#endif
			goto badchar;
#endif /*ALLWIDE*/
#ifndef NO_CI4
		case 'I':
			if (_lib_version != c_issue_4)
				return nmatch;
			field.flags |= F_LONG;
			/*FALLTHROUGH*/
#endif
		case 'i':
			field.uival = 0;	/* base not yet known */
		integer:;
			if (!scanint(fp, &field))
				goto mismatch;
			if (field.flags & F_STAR)
				continue;
#ifndef NO_LONG_LONG_EMULATE
			if (field.flags & F_LDBL)
			{
				_ullmove(va_arg(curpos.ap, long *),
					field.flags & F_NEG,
					field.high, field.ret.ul);
			}
			else
#endif
			if (field.flags & F_UINT)	/* unsigned target */
			{
				if (field.flags & F_NEG)
					field.ret.ul = -field.ret.ul;
				if (field.flags & F_VPTR)
				{
					*va_arg(curpos.ap, void **)
						= (void *)field.ret.ul;
				}
				else if (field.flags & F_LONG)
				{
					*va_arg(curpos.ap, Ulong *)
						= field.ret.ul;
				}
				else if (field.flags & F_HALF)
				{
					*va_arg(curpos.ap, Ushort *)
						= field.ret.ul;
				}
				else
				{
					*va_arg(curpos.ap, Uint *)
						= field.ret.ul;
				}
			}
			else	/* signed target */
			{
				if (field.flags & F_NEG)
				{
#if -SHRT_MAX > SHRT_MIN /* two's complement: don't negate most negative */
					if (field.ret.l != LONG_MIN)
						field.ret.l = -field.ret.l;
#else
					field.ret.l = -field.ret.l;
#endif
				}
				if (field.flags & F_LONG)
				{
					*va_arg(curpos.ap, long *)
						= field.ret.l;
				}
				else if (field.flags & F_HALF)
				{
					*va_arg(curpos.ap, short *)
						= field.ret.l;
				}
				else
				{
					*va_arg(curpos.ap, int *)
						= field.ret.l;
				}
			}
			nmatch++;
			continue;
#ifndef NO_CI4
		case 'D':
			if (_lib_version != c_issue_4)
				return nmatch;
			field.flags |= F_LONG;
			/*FALLTHROUGH*/
#endif
		case 'd':
			field.uival = 10;
			goto integer;
#ifndef NO_CI4
		case 'B':
			if (_lib_version != c_issue_4)
				return nmatch;
			field.flags |= F_LONG;
			/*FALLTHROUGH*/
#endif
		case 'b':
			field.flags |= F_UINT;
			field.uival = 2;
			goto integer;
#ifndef NO_CI4
		case 'O':
			if (_lib_version != c_issue_4)
				return nmatch;
			field.flags |= F_LONG;
			/*FALLTHROUGH*/
#endif

		case 'o':
			field.flags |= F_UINT;
			field.uival = 8;
			goto integer;
#ifndef NO_CI4
		case 'U':
			if (_lib_version != c_issue_4)
				return nmatch;
			field.flags |= F_LONG;
			/*FALLTHROUGH*/
#endif
		case 'u':
			field.flags |= F_UINT;
			field.uival = 10;
			goto integer;
		case 'X':
#ifndef NO_CI4
			if (_lib_version == c_issue_4
				&& (field.flags & F_HALF) == 0)
			{
				field.flags |= F_LONG;
			}
			/*FALLTHROUGH*/
#endif
		case 'x':
			field.flags |= F_UINT;
			field.uival = 16;
			goto integer;
		case 'p':
			field.flags |= F_UINT | F_VPTR;
			field.uival = 16;
			goto integer;
#ifndef NO_NCEG_FPE
		case 'A':
#endif
		case 'E':
		case 'F':
		case 'G':
#ifndef NO_CI4
			if (_lib_version == c_issue_4
				&& (field.flags & F_LDBL) == 0)
			{
				field.flags |= F_LONG;
			}
			/*FALLTHROUGH*/
#endif
#ifndef NO_NCEG_FPE
		case 'a':
#endif
		case 'e':
		case 'f':
		case 'g':
			field.ret.str = (UCHAR *)buf;
			field.uival = sizeof(buf);
			if (!scanflt(fp, &field))
				goto mismatch;
			if ((field.flags & F_STAR) == 0)
			{
				if ((field.flags & (F_LONG | F_LDBL)) == 0)
				{
					float f;

#ifdef ALLWIDE
#ifndef NO_NCEG_FPE
					f = wcstof((wchar_t *)field.ret.str,
						(wchar_t **)0);
#else
					f = wcstod((wchar_t *)field.ret.str,
						(wchar_t **)0);
#endif
#else /*!ALLWIDE*/
#ifndef NO_NCEG_FPE
					f = strtof((char *)field.ret.str,
						(char **)0);
#else
					f = strtod((char *)field.ret.str,
						(char **)0);
#endif
#endif /*ALLWIDE*/
					if (field.flags & F_NEG)
						f = -f;
					*va_arg(curpos.ap, float *) = f;
				}
#ifndef NO_LONG_DOUBLE
				else if (field.flags & F_LDBL)
				{
					long double ld;

#ifdef ALLWIDE
					ld = wcstold((wchar_t *)field.ret.str,
						(wchar_t **)0);
#else
					ld = strtold((char *)field.ret.str,
						(char **)0);
#endif
					if (field.flags & F_NEG)
						ld = -ld;
					*va_arg(curpos.ap, long double *) = ld;
				}
#endif /*NO_LONG_DOUBLE*/
				else
				{
					double d;

#ifdef ALLWIDE
					d = wcstod((wchar_t *)field.ret.str,
						(wchar_t **)0);
#else
					d = strtod((char *)field.ret.str,
						(char **)0);
#endif
					if (field.flags & F_NEG)
						d = -d;
					*va_arg(curpos.ap, double *) = d;
				}
				nmatch++;
			}
			if (field.flags & F_ALLOC)
				free((void *)field.ret.str);
			continue;
		}
	}
mismatch:;
	if (nmatch != 0)
		return nmatch;
	if (field.flags & F_EOF)
		return EOF;
	return 0;
#if (defined(WIDE) || !defined(NO_MSE)) && !defined(ALLWIDE)
badchar:;
	if (nmatch != 0)
		return nmatch;
	return EOF;
#endif
}
