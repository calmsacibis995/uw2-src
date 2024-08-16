%{
/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
%}
%{
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
%}
%{
/*	  All Rights Reserved  	*/
%}

%{
/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
%}
%{
/*	The copyright notice above does not evidence any   	*/
%}
%{
/*	actual or intended publication of such source code.	*/
%}

#ident	"@(#)mail:common/cmd/mail/mailproc/cmd.l	1.2"
%{
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
%}
%{
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
%}
%{
/*	  All Rights Reserved  	*/
%}

%{
/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
%}
%{
/*	The copyright notice above does not evidence any   	*/
%}
%{
/*	actual or intended publication of such source code.	*/
%}

%{
#ident "@(#)cmd.l	1.1 'attmail mail(1) command'"
#undef	input
#undef	unput

#include "../mail/libmail.h"
#include "mailproc.h"
#include "lparse.h"

%}

%%

\%m>>		{
			return TXA;
		} /* NOTREACHED */

\%m>		{
			return TXR;
		} /* NOTREACHED */

\%M>>		{
			return MGA;
		} /* NOTREACHED */

\%M>		{
			return MGR;
		} /* NOTREACHED */

\%m		{
			(void) setfile(yytext, M_TXT);
			return ALP;
		} /* NOTREACHED */

\%M		{
			(void) setfile(yytext, M_MSG);
			return ALP;
		} /* NOTREACHED */

\%H		{
			(void) setfile(yytext, M_HDR);
			return ALP;
		} /* NOTREACHED */

\%F		{
			return LFR;
		} /* NOTREACHED */

\%f		{
			return FRM;
		} /* NOTREACHED */

\%!F		{
			Prtdomain = 0;
			return LFR;
		} /* NOTREACHED */

\%!f		{
			Prtdomain = 0;
			return FRM;
		} /* NOTREACHED */

\%@F		{
			Prtdomain = 1;
			return LFR;
		} /* NOTREACHED */

\%@f		{
			Prtdomain = 1;
			return FRM;
		} /* NOTREACHED */

\%s		{
			return SUB;
		} /* NOTREACHED */

\%d		{
			return DAT;
		} /* NOTREACHED */

\%\%		{
			return ALP;
		} /* NOTREACHED */

\%\{[^}]+\}	{
			return HDR;
		} /* NOTREACHED */

\%:		{
			yytext[0] = ':';
			yytext[1] = '\0';
			return ALP;
		} /* NOTREACHED */

\\\%		{	/* monitor escapes but don't escape % */
			yyless(yyleng-1); /* put % back in input stream */
			return ALP;
		} /* NOTREACHED */

\%		;

\\.		{
			/*
			 * character is quoted -- need since we are
			 * monitoring strings.  Just pass it on.
			 */
			return ALP;
		} /* NOTREACHED */

\"		{
			/*
			 * This quote is to be passed to shell but
			 * we need to know if we are in a string and
			 * if so, what kind of string.
			 */
			return DQT;
		} /* NOTREACHED */

'		{
			/*
			 * This quote is to be passed to shell but
			 * we need to know if we are in a string and
			 * if so, what kind of string.
			 */
			return SQT;
		} /* NOTREACHED */

[>]+		{
			return ALP;
		} /* NOTREACHED */

:		{
			return COL;
		} /* NOTREACHED */

[^\\'":\n>%]+	{
			return ALP;
		} /* NOTREACHED */

\n		{
			return EOL;
		} /* NOTREACHED */

%%
