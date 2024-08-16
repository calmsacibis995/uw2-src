/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/r822.h	1.1.1.3"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#if 0
#ident "@(#)r822.h	1.4 'attmail mail(1) command'"
#endif
#ifndef r822_h
/*
** Any source file using this header must also include <s_string.h>.
*/
/*
** Flag values for r822_addr_parse
**
** 		r822_LOOSE_QPAIRS  use RFC-821 rules for quoted pairs inside local part
** 		r822_STRICT_COMMA  no phantom comma hack
** 		r822_LOOSE_CRLF    relax rules about treatment of EOL stuff
** 				           (makes it easier to understand but not quite correct)
**		r822_LOOSE_LWSP    take isspace() as linear whitespace
**		r822_LOOSE_DOMLIT  allows full RFC-822 for domain literals instead of RFC-821
**		r822_STRICT_COLON  no GMS folder names in bang addresses
**		r822_STRICT_7BIT   clear of hi-bit enforced (no error signalled)
**		r822_LOOSE_FNAME   allow full RFC-822 field-names, else just alnum and hyphen
**		r822_SKIP_NAME     skip past an RFC-822 header tag
**
** Other useful flags
**
**		r822_IS_ANONYMOUS  e.g., From:<>
**		r822_IS_ERROR      something's wrong with this address
**		r822_IS_DOMLIT     domain literals
*/
#define r822_LOOSE_QPAIRS     (1<< 0)
#define r822_STRICT_COMMA     (1<< 1)
#define r822_LOOSE_CRLF       (1<< 2)
#define r822_LOOSE_LWSP       (1<< 3)
#define r822_LOOSE_DOMLIT     (1<< 4)
#define r822_STRICT_COLON     (1<< 5)
#define r822_STRICT_7BIT      (1<< 6)
#define r822_LOOSE_FNAME      (1<< 8)
#define r822_IS_ANONYMOUS     (1<< 9)
#define r822_IS_ERROR         (1<<10)
#define r822_IS_DOMLIT        (1<<12)
#define r822_SKIP_NAME        (1<<22)

typedef struct r822_subthing r822_subthing;
typedef struct r822_domain r822_domain;
typedef struct r822_address r822_address;

/*
** Mail address data structures
*/
/*
** Destructors for the following classes follow the "->next" pointers and
** delete the descendents in the chains.  You only have to delete the
** first object (or let it go out of scope).
*/
struct r822_subthing
{
	string *element;
	int flags;     /* e.g., is this a domain literal? */
	r822_subthing *next;
};

extern void init_r822_subthing ARGS((r822_subthing *this));
extern void fini_r822_subthing ARGS((r822_subthing *this));
extern r822_subthing *new_r822_subthing ARGS(());
extern void delete_r822_subthing ARGS((r822_subthing *this));

/*
**  A domain, which is a collection of subdomain elements
*/
struct r822_domain
{
	string		*dotted;	/* redundant with subdom info */
	r822_subthing	*subdom;	/* subdomain element strings */
	r822_domain	*next;		/* next subdomain (more general) */
};

extern void init_r822_domain ARGS((r822_domain *this));
extern void fini_r822_domain ARGS((r822_domain *this));
extern r822_domain *new_r822_domain ARGS(());
extern void delete_r822_domain ARGS((r822_domain *this));

/*
**  An address.
**
**  phrase1 phrase2 <@route2,@route1:local_part@domain>
**  gphrase1 gphrase1: address1, address2;
**  local_part@domain (Comment1) (Comment2)
*/
struct r822_address
{
	r822_domain *route;		/* route, not including destination domain */
	r822_domain *domain_part;	/* domain part */
	string *local_part;		/* local part */
	r822_subthing *options;		/* /options... */
	string *name_phrase;		/* Comment phrase */
	string *group_phrase;		/* Group name phrase */
	r822_subthing *comment;		/* () comment phrase */
	string *error;			/* error text if not empty */
	int    flags;			/* mainly parsing behavior flags */
	r822_address *next;		/* next address in list */
};

extern void init_r822_address ARGS((r822_address *this));
extern void fini_r822_address ARGS((r822_address *this));
extern r822_address *new_r822_address ARGS(());
extern void delete_r822_address ARGS((r822_address *this));

extern int  r822_addr_parse ARGS((string *input, int flags, r822_address **output));

#endif /* r822_h */
