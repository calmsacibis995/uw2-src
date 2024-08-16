/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/wctypefcns.c	1.1"

/*
 * wctypefcns.c
 *
 * underlying isw..() functions
 *
 */

#include <wcharm.h>

#undef iswalpha
#undef iswupper
#undef iswlower
#undef iswdigit
#undef iswxdigit
#undef iswalnum
#undef iswspace
#undef iswpunct
#undef iswprint
#undef iswgraph
#undef iswcntrl
#undef iswascii
#undef towupper
#undef towlower


int 
#ifdef __STDC__
iswalpha(wint_t wc) 
#else
iswalpha(wc) wint_t wc;
#endif
{
	return	((wc) > 255 ? __iswctype(wc,_U|_L) : isalpha(wc));
}

int 
#ifdef __STDC__
iswupper(wint_t wc) 
#else
iswupper(wc) wint_t wc;
#endif
{
	return	((wc) > 255 ? __iswctype(wc,_U) : isupper(wc));
}

int 
#ifdef __STDC__
iswlower(wint_t wc) 
#else
iswlower(wc) wint_t wc;
#endif
{
	return	((wc) > 255 ? __iswctype(wc,_L) : islower(wc));
}

int 
#ifdef __STDC__
iswdigit(wint_t wc) 
#else
iswdigit(wc) wint_t wc;
#endif
{
	return	((wc) > 255 ? __iswctype(wc,_N) : isdigit(wc));
}

int 
#ifdef __STDC__
iswxdigit(wint_t wc)
#else
iswxdigit(wc) wint_t wc;
#endif
{
	return	((wc) > 255 ? __iswctype(wc,_X): isxdigit(wc));
}

int 
#ifdef __STDC__
iswalnum(wint_t wc) 
#else
iswalnum(wc) wint_t wc;
#endif
{
	return	((wc) > 255 ? __iswctype(wc,_U|_L|_N) : isalnum(wc));
}

int 
#ifdef __STDC__
iswspace(wint_t wc) 
#else
iswspace(wc) wint_t wc;
#endif
{
	return ((wc) > 255 ? __iswctype(wc,_S): isspace(wc));
}

int	
#ifdef __STDC__
iswpunct(wint_t wc)
#else
iswpunct(wc) wint_t wc;
#endif
{
	return	((wc) > 255 ? __iswctype(wc,_P): ispunct(wc));
}

int 
#ifdef __STDC__
iswprint(wint_t wc)
#else
iswprint(wc) wint_t wc;
#endif
{
	return	((wc) > 255 ? __iswctype(wc,_P|_U|_L|_N|_B|_E1|_E2|_E5|_E6)
	: isprint(wc));
}

int 
#ifdef __STDC__
iswgraph(wint_t wc) 
#else
iswgraph(wc) wint_t wc;
#endif
{
	return	((wc) > 255 ? __iswctype(wc,_P|_U|_L|_N|_E1|_E2|_E5|_E6)
					    : isgraph(wc));
}

int 
#ifdef __STDC__
iswcntrl(wint_t wc)
#else
iswcntrl(wc) wint_t wc;
#endif
{
	return	((wc) > 255 ? __iswctype(wc,_C) : iscntrl(wc));
}


wint_t	
#ifdef __STDC__
towupper(wint_t wc)
#else
towupper(wc) wint_t wc;
#endif
{
	return	((wc) > 255 ? __trwctype(wc,_L) : toupper(wc));
}

wint_t	
#ifdef __STDC__
towlower(wint_t wc)
#else
towlower(wc) wint_t wc;
#endif
{
	return	((wc) > 255 ? __trwctype(wc,_U): tolower(wc));
}

/* iswascii - for mnls compatibility */

int 
#ifdef __STDC__
iswascii(wint_t wc)
#else
iswascii(wc) wint_t wc;
#endif
{
	return(isascii(wc));
}
