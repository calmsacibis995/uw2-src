/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)langsup:common/ls/codesets/eucJP.sjis/_iconv.c	1.1"
#include "_iconv_defs.h"

#define C1	0x80	/* Start of Control 1 set */
#define	CS1	0xa1	/* Start of EUC CODE SET1 */
#define	SS2	0x8e	/* EUC CODE SET2 */
#define	SS3	0x8f	/* EUC CODE SET3 */

/*
 * This macro returns SS2 or SS3 if the character is one of those two
 * values. If the character is in the CS1 character range, it returns
 * CS1. Otherwise, if the character is C1, return C1. Finally, if the
 * character is ASCII or C0, it returns 0.
 */
#define codeset(char)	((char == SS2 || char == SS3) ? char : \
				((char >= CS1 && char != 0xff) ? CS1 : \
					(char < C1 ? 0 : C1)))

#define SUB_CHAR_LEN	2

/*
 * Substitute character to use for codes that are legal in EUC but
 * not present in SJIS. (There are no codes that are legal in SJIS but
 * not present in EUC.)
 */
static unsigned char sub_char[SUB_CHAR_LEN] = { 0x81, 0xa0 }; /* A box */

static unsigned short int u2s(unsigned short int c);
static unsigned short int s2u(unsigned short int c);

size_t
euc_to_sjis(iconv_t cd, const char **inbuf, size_t *inbytesleft, char **outbuf,
		size_t *outbytesleft)
{
	unsigned char *from = (unsigned char *) *inbuf;
	unsigned char *to = (unsigned char *) *outbuf;
	size_t bytesleft = *inbytesleft;
	size_t spaceleft = *outbytesleft;
	unsigned char inchar, inchar2;
	unsigned short int input, output;
	size_t ret = 0;
	int i;

	if (inbuf == NULL || *inbuf == NULL) {
		/*
		 * If there were any state to update, we would update it using:
		 * _init_state(state);
		 */
		*inbytesleft = 0;	/* Should this really be updated? */
		return 0;
	}

	while (bytesleft && spaceleft) {

		inchar = *from;

		switch (codeset(inchar)) {
		case SS3:	/* Unsupported so output a substitute char */
			if (bytesleft < 3) {
				errno = EINVAL;
				goto errret;
			}

			inchar = *(from+1);
			inchar2 = *(from+2);

			if (inchar < 0x80 || inchar2 < 0x80) {
				errno = EILSEQ;
				goto errret;
			}
			if (spaceleft < SUB_CHAR_LEN) {
				errno=E2BIG;
				goto errret;
			}

			for (i=0; i < SUB_CHAR_LEN; i++) {
				*to++ = sub_char[i];
			}
			spaceleft -= SUB_CHAR_LEN;

			from += 3;
			bytesleft -= 3;
			break;
		case SS2:	/* KATAKANA */
			if (bytesleft < 2) {
				errno = EINVAL;
				goto errret;
			}
			/*
			 * Since we only output one character, we know that
			 * there is enough space in the output buffer.
			 */
			inchar = *(from+1);
			if (inchar >= CS1 && inchar <= 0xdf) {
				from += 2;
				bytesleft -= 2;

				*to++ = inchar;
				spaceleft--;
				ret += 2;
			} else {
				errno = EILSEQ;
				goto errret;
			}
			break;
		case CS1:	/* KANJI */
			if (bytesleft < 2) {
				errno = EINVAL;
				goto errret;
			}

			inchar2 = *(from+1);

			/*
			 * Note that EILSEQ is always given priority over E2BIG
			 */
			if (inchar2 < CS1 || inchar2 == 0xff) {
				errno = EILSEQ;
				goto errret;
			} else if (spaceleft < 2) {
				errno=E2BIG;
				goto errret;
			} else {
				from += 2;
				bytesleft -= 2;

				input = (inchar << 8) | inchar2;
				output = u2s(input);
				*to++ = output >> 8;
				*to++ = (unsigned char) output;
				spaceleft -= 2;
				ret += 2;
			}
			break;
		case C1:	/* Doesn't translate, so output substitute */
			if (spaceleft < SUB_CHAR_LEN) {
				errno=E2BIG;
				goto errret;
			}

			for (i=0; i < SUB_CHAR_LEN; i++) {
				*to++ = sub_char[i];
			}
			spaceleft -= SUB_CHAR_LEN;

			from++;
			bytesleft--;
			break;
		default:	/* ASCII */
			*to++ = *from++;
			bytesleft--;
			spaceleft--;
			break;
		}
	}

	if (bytesleft != 0) {		/* if everything was not converted */
		errno = E2BIG;
	}
errret:
	*inbytesleft = bytesleft;
	*outbytesleft = spaceleft;
	*inbuf = (char *) from;
	*outbuf = (char *) to;
	if (bytesleft != 0) {		/* if error, then always true */
		return (size_t) -1;
	} else {
		return ret;
	}
}

unsigned short int
u2s(unsigned short int c)
{
	unsigned short int hi, lo;

	hi = (c >> 8) & 0x7f;
	lo = c & 0x7f;

	if ( hi & 1) {
		lo += 0x1f;
	} else {
		lo += 0x7d;
	}

	hi = ( (hi - 0x21) >> 1 ) + 0x81;

	if (lo >= 0x7f) {
		lo++;
	}

	if (hi > 0x9f) {
		hi += 0x40;
	}

	return ((hi << 8) | lo);
}

size_t
sjis_to_euc(iconv_t cd, const char **inbuf, size_t *inbytesleft, char **outbuf,
		size_t *outbytesleft)
{
	unsigned char *from = (unsigned char *) *inbuf;
	unsigned char *to = (unsigned char *) *outbuf;
	size_t bytesleft = *inbytesleft;
	size_t spaceleft = *outbytesleft;
	unsigned char inchar, inchar2;
	unsigned short int input, output;
	size_t ret = 0;

	if (inbuf == NULL || *inbuf == NULL) {
		/*
		 * If there were any state to update, we would update it using:
		 * _init_state(state);
		 */
		*inbytesleft = 0;	/* Should this really be updated? */
		return 0;
	}

	while(bytesleft && spaceleft) {

		inchar = *from;

		if (inchar < 0x80) {				/* ASCII */
			from++;
			bytesleft--;

			*to++ = inchar;
			spaceleft--;

		} else if (inchar > 0xa0 && inchar < 0xe0) { /* KATAKANA */

			if (spaceleft < 2) {
				errno=E2BIG;
				goto errret2;
			}
			from++;
			bytesleft--;

			*to++ = SS2;
			*to++ = inchar;
			spaceleft -= 2;
			ret++;

		} else if (inchar == 0x80 || inchar == 0xa0 ||	/* ILLEGAL */
				inchar >= 0xf0) {
			errno=EILSEQ;
			goto errret2;

		} else {					/* 2 BYTE */

			if (bytesleft < 2) {
				errno = EINVAL;
				goto errret2;
			}

			inchar2 = *(from+1);

			if (inchar2 < 0x40 || inchar2 == 0x7f ||
				inchar2 > 0xfc) {		/* ILLEGAL */

				errno=EILSEQ;
				goto errret2;
			} else {				/* KANJI */
				if (spaceleft < 2) {
					errno=E2BIG;
					goto errret2;
				}
				from += 2;
				bytesleft -= 2;

				input = (inchar << 8) | inchar2;
				output = s2u(input);
				*to++ = (unsigned char) (output >> 8);
				*to++ = (unsigned char) output;
				spaceleft -= 2;
				ret += 2;
			}
		}
	}

	if (bytesleft != 0) {		/* if everything was not converted */
		errno = E2BIG;
	}
errret2:
	*inbytesleft = bytesleft;
	*outbytesleft = spaceleft;
	*inbuf = (char *) from;
	*outbuf = (char *) to;
	if (bytesleft != 0) {		/* if error, must always be true */
		return (size_t) -1;
	} else {
		return ret;
	}
}

unsigned short int
s2u(unsigned short int c)
{
	unsigned short int hi, lo;

	hi = ( c >> 8 ) & 0xff;
	lo = c & 0xff;
	hi -= ( hi <= 0x9f) ? 0x71 : 0xb1;
	hi = hi * 2 + 1;

	if ( lo > 0x7f) {
		lo--;
	}

	if ( lo >= 0x9e) {
		lo -= 0x7d;
		hi++;
	} else {
		lo -= 0x1f;
	}

	return (((hi << 8) | lo) | 0x8080);
}
