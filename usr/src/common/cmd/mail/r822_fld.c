/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/r822_fld.c	1.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)r822_fld.c	1.2 93/06/16 13:01:24"

#include "mail.h"
#include "r822.h"
#include "r822t.h"

#define GET_this_char \
		this_int = getter(blob);\
		if (this_int < 0) {return (this_int);}\
		this_char = this_int & ((strict_7bit) ? 0x7F : 0xFF)

#define GET_this_char_EOF \
		this_int  = getter(blob);\
		this_char = this_int & ((strict_7bit) ? 0x7F : 0xFF)

#define IS_LWSP_CHAR(a) \
		((loose_lwsp && isspace(a)) || ((a)==' ') || ((a)=='\t'))

#define UNGET_STRING(s) \
		for (sdex=s_curlen(s); sdex; --sdex) {ungetter(s_to_c(s)[sdex-1], blob);}\
		s_reset(s)

#define IS_FNAME_CHAR(ch) \
		(ch != ' ' && ch != ':' && !iscntrl(ch) && \
		 ((loose_fname) \
		  ? 1 \
		  : (isalnum(ch) || ch == '-')))

int
r822_find_field_name(getter, ungetter, blob, flags, fn)
int (*getter) ARGS((void *));
int (*ungetter) ARGS((int, void*));
void *blob;
int flags;
string *fn;
{
	/*
	** Return >0 if found a header, 0 if at header/body boundary, and <0
	** if that's what getter() tells us.
	*/
	int this_int;
	char this_char;
	int sdex;
	int strict_7bit =  (flags & r822_STRICT_7BIT);
	int allow_from_ =  (flags & r822_ALLOW_FROM_);
	int loose_lwsp  =  (flags & r822_LOOSE_LWSP);
	int loose_fname =  (flags & r822_LOOSE_FNAME);
	s_reset(fn);
	for (;;)
	{
		GET_this_char;
		if (IS_FNAME_CHAR(this_char))
		{
			s_putc(fn,this_char);
			continue;
		}
		break;
	}
	
	if (allow_from_  &&  this_char == ' '  &&  ((strcmp(s_to_c(fn), "From") == 0) || (strcmp(s_to_c(fn), ">From") == 0)))
	{
		s_putc(fn, ' ');
		return (1);
	}

	
	/*
	** Get rid of optional trailing whitespace after the field name and
	** before the colon.  The whitespace isn't really legal, but you see
	** it all the time.
	*/
	ungetter(this_char, blob);
	for (;;)
	{
		GET_this_char;
		if (this_char == ':')
		{
			return (1);
		}
		if (this_char == '\n'  ||  this_char == '\r')
		{
			/*
			** No trailing colon, so this must be the first line of the
			** body of the message.
			*/
			UNGET_STRING(fn);
			ungetter(this_char, blob);
			return (0);
		}
		
		if (IS_LWSP_CHAR(this_char))
		{
			continue;
		}

		UNGET_STRING(fn);
		ungetter(this_char, blob);
		return (0);
	}
}

/* See state info at bottom file */

#define bpc 3  /* bits per code */
#define cBS			1	/* backslash         */
#define cCR			2	/* carriage return   */
#define cEOF			3	/* end-of-input      */
#define cLF			4	/* line feed         */
#define cLWSP			5	/* linear whitespace */
#define cXXX			6	/* anything else     */

#define cBS_BS			(((cBS)<<bpc)|cBS)
#define cBS_CR			(((cBS)<<bpc)|cCR)
#define cBS_CR_BS		(((((cBS)<<bpc)|cCR)<<bpc)|cBS)
#define cBS_CR_CR		(((((cBS)<<bpc)|cCR)<<bpc)|cCR)
#define cBS_CR_EOF		(((((cBS)<<bpc)|cCR)<<bpc)|cEOF)
#define cBS_CR_LF		(((((cBS)<<bpc)|cCR)<<bpc)|cLF)
#define cBS_CR_LF_BS		(((((((cBS)<<bpc)|cCR)<<bpc)|cLF)<<bpc)|cBS)
#define cBS_CR_LF_CR		(((((((cBS)<<bpc)|cCR)<<bpc)|cLF)<<bpc)|cCR)
#define cBS_CR_LF_EOF		(((((((cBS)<<bpc)|cCR)<<bpc)|cLF)<<bpc)|cEOF)
#define cBS_CR_LF_LF		(((((((cBS)<<bpc)|cCR)<<bpc)|cLF)<<bpc)|cLF)
#define cBS_CR_LF_LWSP		(((((((cBS)<<bpc)|cCR)<<bpc)|cLF)<<bpc)|cLWSP)
#define cBS_CR_LF_XXX		(((((((cBS)<<bpc)|cCR)<<bpc)|cLF)<<bpc)|cXXX)
#define cBS_CR_LWSP		(((((cBS)<<bpc)|cCR)<<bpc)|cLWSP)
#define cBS_CR_XXX		(((((cBS)<<bpc)|cCR)<<bpc)|cXXX)
#define cBS_EOF			(((cBS)<<bpc)|cEOF)
#define cBS_LF			(((cBS)<<bpc)|cLF)
#define cBS_LF_BS		(((((cBS)<<bpc)|cLF)<<bpc)|cBS)
#define cBS_LF_CR		(((((cBS)<<bpc)|cLF)<<bpc)|cCR)
#define cBS_LF_EOF		(((((cBS)<<bpc)|cLF)<<bpc)|cEOF)
#define cBS_LF_LF		(((((cBS)<<bpc)|cLF)<<bpc)|cLF)
#define cBS_LF_LWSP		(((((cBS)<<bpc)|cLF)<<bpc)|cLWSP)
#define cBS_LF_XXX		(((((cBS)<<bpc)|cLF)<<bpc)|cXXX)
#define cBS_LWSP		(((cBS)<<bpc)|cLWSP)
#define cBS_XXX			(((cBS)<<bpc)|cXXX)
#define cCR_BS			(((cCR)<<bpc)|cBS)
#define cCR_CR			(((cCR)<<bpc)|cCR)
#define cCR_EOF			(((cCR)<<bpc)|cEOF)
#define cCR_LF			(((cCR)<<bpc)|cLF)
#define cCR_LF_BS		(((((cCR)<<bpc)|cLF)<<bpc)|cBS)
#define cCR_LF_CR		(((((cCR)<<bpc)|cLF)<<bpc)|cCR)
#define cCR_LF_EOF		(((((cCR)<<bpc)|cLF)<<bpc)|cEOF)
#define cCR_LF_LF		(((((cCR)<<bpc)|cLF)<<bpc)|cLF)
#define cCR_LF_LWSP		(((((cCR)<<bpc)|cLF)<<bpc)|cLWSP)
#define cCR_LF_XXX		(((((cCR)<<bpc)|cLF)<<bpc)|cXXX)
#define cCR_LWSP		(((cCR)<<bpc)|cLWSP)
#define cCR_XXX			(((cCR)<<bpc)|cXXX)
#define cLF_BS			(((cLF)<<bpc)|cBS)
#define cLF_CR			(((cLF)<<bpc)|cCR)
#define cLF_EOF			(((cLF)<<bpc)|cEOF)
#define cLF_LF			(((cLF)<<bpc)|cLF)
#define cLF_LWSP		(((cLF)<<bpc)|cLWSP)
#define cLF_XXX			(((cLF)<<bpc)|cXXX)

int
r822_find_field_body(getter, ungetter, blob, flags, fb)
int (*getter) ARGS((void *));
int (*ungetter) ARGS((int, void*));
void *blob;
int flags;
string *fb;
{
	int this_int, this_code, tail = 0;
	char this_char;
	int loose_crlf   =  (flags & r822_LOOSE_CRLF);
	int loose_unfold =  (flags & r822_LOOSE_UNFOLD);
	int strict_7bit  =  (flags & r822_STRICT_7BIT);
	int loose_lwsp   =  (flags & r822_LOOSE_LWSP);
	s_reset(fb);
	for (;;)
	{
		GET_this_char_EOF;
		if      (this_char == '\r')        this_code = cCR;
		else if (this_char == '\n')        this_code = cLF;
		else if (this_int  == EOF)         this_code = cEOF;
		else if (this_char == '\\')        this_code = cBS;
		else if (IS_LWSP_CHAR(this_char))  this_code = cLWSP;
		else                               this_code = cXXX;

		tail = (tail << bpc) | this_code;
		switch (tail)
		{
		default:
			return (EOF);

		case cEOF:
			return (EOF);
			
		case cLWSP:
		case cXXX:
			s_putc(fb,this_char);
			tail = 0;
			break;

		case cBS:   case cBS_CR:   case cBS_CR_LF:   case cBS_LF:
		case cCR:   case cCR_LF:
		case cLF:
			/* cases that are prefixes of other cases */
			break;

		case cBS_CR_LF_LWSP:
		case cBS_CR_LF_EOF:
		case cBS_CR_LF_XXX:
		case cBS_CR_LF_CR:
		case cBS_CR_LF_LF:
		case cBS_CR_LF_BS:
			if (loose_unfold)
			{
				fb = s_append(fb, "\r\n");
			}
			else
			{
				s_putc(fb,'\n');
			}
			tail = 0;
			if (this_code == cEOF)
			{
				return (EOF);
			}
			if (this_code != cLWSP)
			{
				ungetter(this_char, blob);
				return (1);
			}
			break;

		case cBS_CR_EOF:
		case cBS_CR_XXX:
		case cBS_CR_CR:
		case cBS_CR_BS:
		case cBS_CR_LWSP:
			fb = s_append(fb, "\\\r");
			if (this_code == cEOF)
			{
				return (EOF);
			}
			ungetter(this_char, blob);
			tail = 0;
			break;

		case cBS_LF_LWSP:
			if (!loose_crlf)
			{
				ungetter(this_char, blob);
			}
			if (loose_unfold)
			{
				if (loose_crlf)
				{
					fb = s_append(fb, "\n");
				}
				else
				{
					fb = s_append(fb, "\\\n");
				}
			}
			else
			{
				if (loose_crlf)
				{
					s_putc(fb, '\n');
				}
				else
				{
					fb = s_append(fb, "\\\n");
				}
			}
			tail = 0;
			break;
		case cBS_LF_EOF:
			if (loose_unfold)
			{
				if (loose_crlf)
				{
					fb = s_append(fb, "\n");
				}
				else
				{
					fb = s_append(fb, "\\\n");
				}
			}
			else
			{
				s_putc(fb, '\n');
			}
			return (EOF);
		case cBS_LF_XXX:
		case cBS_LF_CR:
		case cBS_LF_LF:
		case cBS_LF_BS:
			ungetter(this_char, blob);
			if (loose_unfold)
			{
				if (loose_crlf)
				{
					fb = s_append(fb, "\n");
				}
				else
				{
					fb = s_append(fb, "\\\n");
				}
			}
			tail = 0;
			break;
		case cBS_BS:
			fb = s_append(fb, "\\\\");
			tail = 0;
			break;
		case cBS_EOF:
		case cBS_XXX:
		case cBS_LWSP:
			fb = s_append(fb, "\\");
			if (this_code == cEOF)
			{
				return (EOF);
			}
			ungetter(this_char, blob);
			tail = 0;
			break;
		case cLF_LWSP:
		case cLF_EOF:
			if (loose_unfold  ||  !loose_crlf)
			{
				fb = s_append(fb, "\n");
			}
			if (this_code == cEOF)
			{
				return (EOF);
			}
			ungetter(this_char, blob);
			tail = 0;
			break;
		case cLF_XXX:
		case cLF_BS:
		case cLF_CR:
		case cLF_LF:
			ungetter(this_char, blob);
			tail = 0;
			if (loose_crlf)
			{
				return (1);
			}
			fb = s_append(fb, "\n");
			break;
		case cCR_LF_LWSP:
			ungetter(this_char, blob);
			tail = 0;
			if (loose_unfold)
			{
				fb = s_append(fb, "\n");
			}
			break;
		case cCR_LF_EOF:
			return (EOF);
		case cCR_LF_XXX:
		case cCR_LF_BS:
		case cCR_LF_CR:
		case cCR_LF_LF:
			ungetter(this_char, blob);
			return (1);
		case cCR_LWSP:
		case cCR_XXX:
		case cCR_BS:
		case cCR_CR:
			ungetter(this_char, blob);
			tail = 0;
			fb = s_append(fb, "\r");
			break;
		case cCR_EOF:
			fb = s_append(fb, "\r");
			return (EOF);
		}
	}
}


/*
**                                 loose unfold                      strict unfold
**                           loose crlf      strict crlf       loose crlf   strict crlf
**
** EOF                       rE              rE                rE           rE
** XXX                       tc              tc                tc           tc
**
** BS              (prefix)
** BS   BS                   bs,bs,^tc       bs,bs,^tc         bs,bs,^tc    bs,bs,^tc
**
** BS   CR         (prefix)
** BS   CR   BS              bs,cr           bs,cr             bs,cr        bs,cr
** BS   CR   CR              bs,cr           bs,cr             bs,cr        bs,cr
** BS   CR   EOF             bs,cr,rE        bs,cr,rE          bs,cr,rE     bs,cr,rE
**
** BS   CR   LF    (prefix)
** BS   CR   LF   BS         cr,lf,rS        cr,lf,rS          lf,rS        lf,rS
** BS   CR   LF   CR         cr,lf,rS        cr,lf,rS          lf,rS        lf,rS
** BS   CR   LF   EOF        cr,lf,rE        cr,lf,rE          lf,rE        lf,rE
** BS   CR   LF   LF         cr,lf,rS        cr,lf,rS          lf,rS        lf,rS   
** BS   CR   LF   LWSP       cr,lf,^tc       cr,lf,^tc         lf,^tc       lf,^tc
** BS   CR   LF   XXX        cr,lf,rS        cr,lf,rS          lf,rS        lf,rS   
**
** BS   CR   LWSP            bs,cr           bs,cr             bs,cr        bs,cr
** BS   CR   XXX             bs,cr           bs,cr             bs,cr        bs,cr
**
** BS   EOF                  bs,rE           bs,rE             bs,rE        bs,rE
**
** BS   LF         (prefix)
** BS   LF   BS              lf              bs,lf
** BS   LF   CR              lf              bs,lf
** BS   LF   EOF             lf,rE           bs,lf,rE          lf,rE        lf,rE   
** BS   LF   LF              lf              bs,lf
** BS   LF   LWSP            lf,^tc          bs,lf             lf,^tc       bs,lf
** BS   LF   XXX             lf              bs,lf
**
** BS   LWSP                 bs              bs                bs           bs
** BS   XXX                  bs              bs                bs           bs
**
** CR              (prefix)
** CR   BS                   cr              cr                cr           cr
** CR   CR                   cr              cr                cr           cr
** CR   EOF                  cr,rE           cr,rE             cr,rE        cr,rE
**
** CR   LF         (prefix)
** CR   LF   BS              rS              rS                rS           rS
** CR   LF   CR              rS              rS                rS           rS
** CR   LF   EOF             rE              rE                rE           rE
** CR   LF   LF              rS              rS                rS           rS
** CR   LF   LWSP            lf              lf
** CR   LF   XXX             rS              rS                rS           rS
**
** CR   LWSP                 cr              cr                cr           cr
**
** CR   XXX                  cr              cr                cr           cr
**
** LF              (prefix)
** LF   BS                   rS              lf                rS           lf
** LF   CR                   rS              lf                rS           lf
** LF   EOF                  lf,rE           lf,rE             rE           lf,rE
** LF   LF                   rS              lf                rS           lf
** LF   LWSP                 lf              lf                             lf           
** LF   XXX                  rS              lf                rS           lf
**
** "tc" means accumulate the current character
** "^tc" means discard the current character
** if no "tc" or "^tc", then unget the current character
** "rS" means return (success)
** "rE" means return (EOF)
*/
