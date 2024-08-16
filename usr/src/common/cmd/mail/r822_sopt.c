/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/r822_sopt.c	1.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)r822_sopt.c	1.2 93/06/16 13:01:44"

#include "mail.h"
#include "r822.h"
#include "r822t.h"

/*
** Slice off the slash options.  The parsing is grossly simple-minded,
** but it conforms to current GMS practice.  To whit:
**
** 		Any slash ("/") in an address starts an option.
**
** 		The option continues up to the next slash or the end of the address.
**
** 		Within an option, the leftmost equal ("=") signals the
** 		beginning of the option value.  Not all options have values.
**
** In the above rules, no other punctuation interferes with the parsing,
** so:
**
** 		a!b/c=d!e/f!g
**
** is addresses to "a!b" with two options:
**
** 		option "c" with value "d!e"
** 		option "f!g" with no value
**
** In the r822_address structure, we make a list of r822_subthings of the
** options.  If the subthing has flag r822_HAS_VALUE, then the subthing
** following it is the value; otherwise, it's the next option.  Options
** without option names or completely empty options are skipped.  I also
** trim the leading and trailing space from options and values (but not
** embedded spaces in either).
*/

/* trim leading and trailing spaces from the passed in string */
static void
TRIM(str)
string *str;
{
	for (; s_curlen(str) != 0 ;)
	{
		char ch = s_to_c(str)[s_curlen(str)-1];
		if (Isspace(ch))
		{
			s_skipback(str);
			s_terminate(str);
		}
		else
		{
			break;
		}
	}
	if (Isspace(s_to_c(str)[0]))
	{
		string *ns;
		s_terminate(str);
		s_restart(str);
		s_skipwhite(str);
		ns = s_copy(s_ptr_to_c(str));
		s_reset(str);
		s_append(str, s_to_c(ns));
		s_free(ns);
	}
}

/* strip off all options from the string str. Return the options as an r822_subthing list. */
r822_subthing *
r822_slash_options(str)
string *str;
{
	r822_subthing *sp = 0, *sp_start = 0;
	string *up_to_slash = s_etok(s_restart(str), "/");
	if (s_ptr_to_c(str)[0])		/* is anything after the '/'? */
	{
		string *slashes;
		for (slashes = s_tok(str, "/"); slashes; slashes = s_tok(str, "/"))
		{
			/* set this_option and this_value to the values */
			/* before and after any '=' in the string. */
			string *this_option = this_option = s_etok(slashes, "=");
			string *this_value = 0;
			if (s_ptr_to_c(str)[0])	/* is anything after the '='? */
			    this_value = s_clone(slashes);
			TRIM(this_option);
			if (this_value)
			    TRIM(this_value);

			if (s_curlen(this_option) != 0)
			{
				if (sp)
				{
					sp->next = new_r822_subthing();
					sp = sp->next;
				}
				else
				{
					sp = new_r822_subthing();
					sp_start = sp;
				}
				s_reset(sp->element);
				sp->element = s_append(sp->element, s_to_c(this_option));
				if (this_value)
				{
					sp->flags |= r822_HAS_VALUE;
					sp->next = new_r822_subthing();
					sp = sp->next;
					s_reset(sp->element);
					sp->element = s_append(sp->element, s_to_c(this_value));
				}
			}

			/* clean up */
			if (this_option) s_free(this_option);
			if (this_value) s_free(this_value);
			s_free(slashes);
		}

		/* reset str to string up to the 1st slash */
		s_reset(str);
		s_append(str, s_to_c(up_to_slash));
	}
	s_free(up_to_slash);
	return (sp_start);
}

/*
** Iterate through an list of addresses, slicing off options as we go.
*/
int
r822_slash_options_all(ap)
r822_address *ap;
{
	for (; ap; ap=ap->next)
	{
		ap->options = r822_slash_options(ap->local_part);
	}
	return (0);
}
