/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:re/regerror.c	1.4"

#include "synonyms.h"
#include "re.h"
#include "_locale.h"

#ifdef __STDC__
	#pragma weak regerror = _regerror
#endif

size_t
#ifdef __STDC__
regerror(int err, const regex_t *ep, char *str, size_t max)
#else
regerror(err, ep, str, max)int err; const regex_t *ep; char *str; size_t max;
#endif
{
	const struct
	{
		int		index;
		const char	*str;
	} unk = 
	{
			88,  "unknown regular expression error"
	}, msgs[] =
	{
	/*ENOSYS*/	89,  "feature not implemented",
	/*0*/		0,   "",
	/*NOMATCH*/	90,  "regular expression failed to match",
	/*BADPAT*/	91,  "invalid regular expression",
	/*ECOLLATE*/	92,  "invalid collating element construct",
	/*ECTYPE*/	93,  "invalid character class construct",
	/*EEQUIV*/	94,  "invalid equivalence class construct",
	/*EBKTCHAR*/	95,  "invalid character in '[ ]' construct",
	/*EESCAPE*/	96,  "trailing \\ in pattern",
	/*ESUBREG*/	97,  "'\\digit' out of range",
	/*EBRACK*/	98,  "'[ ]' imbalance",
	/*EMPTYSUBBKT*/	99,  "empty nested '[ ]' construct",
	/*EMPTYPAREN*/	100, "empty '\\( \\)' or '( )'",
	/*NOPAT*/	101, "empty pattern",
	/*EPAREN*/	102, "'\\( \\)' or '( )' imbalance",
	/*EBRACE*/	103, "'\\{ \\} or '{ }' imbalance",
	/*BADBR*/	104, "invalid '\\{ \\}' or '{ }'",
	/*ERANGE*/	105, "invalid endpoint in range",
	/*ESPACE*/	106, "out of regular expression memory",
	/*BADRPT*/	107, "invalid *, +, ?, \\{\\} or {} operator",
	/*BADESC*/	108, "invalid escape sequence (e.g. \\0)",
	};
	const char *p;
	size_t len;
	int i;

	if (err < REG_ENOSYS || REG_BADESC < err)
	{
		i = unk.index;
		p = unk.str;
	}
	else
	{
		i = msgs[err - REG_ENOSYS].index;
		p = msgs[err - REG_ENOSYS].str;
	}
	p = __gtxt(_str_uxlibc, i, p);
	len = strlen(p) + 1;
	if (max != 0)
	{
		if (max > len)
			max = len;
		else if (max < len)
			str[--max] = '\0';
		memcpy((void *)str, (const void *)p, max);
	}
	return len;
}