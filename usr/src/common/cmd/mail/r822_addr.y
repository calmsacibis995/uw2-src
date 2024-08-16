/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

%{
#ident	"@(#)mail:common/cmd/mail/r822_addr.y	1.1.1.3"
#ident	"@(#)mail:common/cmd/mail/r822_addr.y	1.1.1.2"
#ident "@(#)r822_addr.y	1.4 'attmail mail(1) command'"

#include "mail.h"
#include "r822.h"

#define ERROR	-2

/*
** The anonymous string is used as a first hop on mail that can't be
** replied to anyhow.
*/
static const char *anonymous = "Unreplyable";


/* a regrettable collection of globals forced by yacc doings */

/* flag bits */
static int flags;
static int comma_hack;
static int loose_qpairs;
static int loose_lwsp;
static int loose_domlit;
static int strict_colon;
static int strict_7bit;

/* bookkeeping */
static string *input_string;
static int    is_dex;
static int    is_length;
static r822_address *adrlist;
static string *errstr;
static r822_subthing *comment_list;
static int prev_ttype = ERROR;   /* previous token type */

static void prepr822_domain ARGS((r822_domain**, r822_domain*));
static void appr822_domain ARGS((r822_domain**, r822_domain*));
static void appr822_subdom ARGS((r822_domain**, r822_subthing*));
static void appr822_address ARGS((r822_address **head, r822_address *addr));
static void domain_literal_bookkeeping ARGS((r822_subthing *sp));

static int gather_comments ARGS((void));
static int gather_quoted_string ARGS((void));
static int gather_domain_literal ARGS((void));
static int gather_atom ARGS((void));

/*
** linear-white-space is formally defined to be just space and horizontal
** tab.  Many implmenentations go with anything that passes isspace();
** that's that the "loose LWSP" flag is all about, as embodied in the
** following definition.
*/
static  int
IS_LWSP_CHAR(a)
unsigned char a;
{
	return ((loose_lwsp && Isspace(a)) || ((a)==' ') || ((a)=='\t'));
}
static  unsigned char
THIS_CHAR(dex)
int dex;
{
	unsigned char this_char = s_to_c(input_string)[dex];
	if (strict_7bit)
	{
		this_char &= 0x7F;
	}
	return (this_char);
}
static  unsigned char
LAST_CHAR_OF(str)
string *str;
{
	return (s_curlen(str) == 0) ? 0 : s_to_c(str)[s_curlen(str) - 1];
}

/* turn a long into a string. warning: this returns a pointer to a static buffer. */
static const char *int_to_str(d)
int d;
{
    static char buf[20];
    (void) sprintf (buf, "%d", d);
    return buf;
}

void init_r822_subthing(this)
r822_subthing *this;
{
    this->element = s_new();
    this->next = 0;
    this->flags = 0; 
}
void fini_r822_subthing(this)
r822_subthing *this;
{
    s_free(this->element);
    delete_r822_subthing(this->next); 
}
r822_subthing *new_r822_subthing()
{
    r822_subthing *this = New(r822_subthing);
    if (this) init_r822_subthing(this);
    return this;
}
void delete_r822_subthing(this)
r822_subthing *this;
{
    if (this)
        {
	fini_r822_subthing(this);
	free((char*)this);
	}
}

void init_r822_domain(this)
r822_domain *this;
{
    this->dotted = s_new();
    this->subdom = 0;
    this->next = 0;
}
void fini_r822_domain(this)
r822_domain *this;
{
    s_free(this->dotted);
    delete_r822_subthing(this->subdom);
    delete_r822_domain(this->next);
}
r822_domain *new_r822_domain()
{
    r822_domain *this = New(r822_domain);
    if (this) init_r822_domain(this);
    return this;
}
void delete_r822_domain(this)
r822_domain *this;
{
    if (this)
	{
	fini_r822_domain(this);
	free((char*)this);
	}
}

void init_r822_address(this)
r822_address *this;
{
    this->route = 0;
    this->domain_part = 0;
    this->local_part = s_new();
    this->options = 0;
    this->name_phrase = s_new();
    this->group_phrase = s_new();
    this->comment = 0;
    this->error = s_new();
    this->flags = 0;
    this->next = 0;
}
void fini_r822_address(this)
r822_address *this;
{
    delete_r822_domain(this->route);
    delete_r822_domain(this->domain_part);
    s_free(this->local_part);
    delete_r822_subthing(this->options);
    s_free(this->name_phrase);
    s_free(this->group_phrase);
    delete_r822_subthing(this->comment);
    s_free(this->error);
    delete_r822_address(this->next);
}
r822_address *new_r822_address()
{
    r822_address *this = New(r822_address);
    if (this) init_r822_address(this);
    return this;
}
void delete_r822_address(this)
r822_address *this;
{
    if (this)
	{
	fini_r822_address(this);
	free((char*)this);
	}
}

/* Try to reduce yacc namespace problems */
#define yylex yylex_r822
#define yyparse yyparse_r822
#define yyerror yyerror_r822
#define yychar yychar_r822
#define yyerrflag yyerrflag_r822
#define yylval yylval_r822
#define yyval yyval_r822
#define yy_yys yy_yys_r822
#define yys yys_r822
#define yyv yyv_r822
#define yy_yyv yy_yyv_r822
#define yyexca yyexca_r822
#define yyact yyact_r822
#define yypact yypact_r822
#define yypgo yypgo_r822
#define yyr1 yyr1_r822
#define yyr2 yyr2_r822
#define yychk yychk_r822
#define yydef yydef_r822
#define yytoks yytoks_r822
#define yyreds yyreds_r822
#define yydebug yydebug_r822
#define yypv yypv_r822
#define yyps yyps_r822
#define yystate yystate_r822
#define yytmp yytmp_r822
#define yynerrs yynerrs_r822
/* end yacc yuks */
#define YYDEBUG 1  /* doesn't cost much */
extern int yydebug;

extern int yyparse ARGS((void));

%}

%union
{
	unsigned char  yChar;
	string        *yString;  /* notice it's a pointer */
	r822_domain   *yDom;
	r822_address  *yAddr;
}

%token	EOL ATOM LIT_DOMAIN QUOTED_STRING PHANTOM_COMMA

%type	<yString>	word domain_ref sub_domain local_part phrase
%type	<yDom>		domain route_list route
%type	<yAddr>		addr_spec route_addr mailbox mbox_list group address

%start	addr_list

%%
addr_list:
			  addr_lel
			| addr_list addr_lel
			;

addr_lel:
			  address EOL
			{
				$1->comment = comment_list;
				comment_list = 0;
				s_reset(errstr);
				appr822_address(&adrlist, $1);
			}
			| address comma_space
			{
				$1->comment = comment_list;
				comment_list = 0;
				s_reset(errstr);
				appr822_address(&adrlist, $1);
				/*
				** The next two if() statements are nonsense, but I'm
				** trying to shut off warnings about unused labels
				** (labels that yacc generates but I don't happen to use).
				** Notice that we've just set comment_list to zero, so the
				** if() can never be true.
				*/
				if (comment_list != 0) goto yynewstate;
				if (comment_list != 0) goto yyerrlab;
			}
			| error
			{
				r822_address *ap;
				ap = new_r822_address();
				ap->flags |= flags;
				ap->flags |= r822_IS_ERROR;
				ap->comment = comment_list;
				s_reset(ap->error);
				ap->error = s_append(ap->error, s_to_c(errstr));
				appr822_address(&adrlist, ap);
				s_reset(errstr);
				comment_list = 0;
				prev_ttype = ERROR;
				switch (yychar)
				{
				case LIT_DOMAIN:
				case QUOTED_STRING:
				case ATOM:
					/* s_free($<yString>1); */
					break;
				}
			}
			;

comma_space:
			  ',' | PHANTOM_COMMA ;

address:
			  mailbox
				{ $$ = $1; }
			| group
				{ $$ = $1; }
			;

group:
			  phrase ':' mbox_list ';'
			{
				r822_address *a;
				for (a = $3; a; a = a->next)
				{
					s_reset(a->group_phrase);
					a->group_phrase = s_append(a->group_phrase, s_to_c($1));
				}
				s_free($1);
				$$ = $3;
			}
			| phrase ':' ';'
			{
				r822_address *ap = new_r822_address();
				ap->flags |= flags;
				s_reset(ap->group_phrase);
				ap->group_phrase = s_append(ap->group_phrase, s_to_c($1));
				s_free($1);
				$$ = ap;
			}
			;

mbox_list:
			  mailbox
				{ $$ = $1; }
			| mbox_list ',' mailbox
			{
				$3->comment = comment_list;
				comment_list = 0;
				s_reset(errstr);
				appr822_address(&($1), $3);
				prev_ttype = ERROR;
				$$ = $1;
			}
			;

mailbox:
			  addr_spec
				{ $$ = $1; }
			| route_addr
				{ $$ = $1; }
			| phrase route_addr
			{
				s_reset($2->name_phrase);
				$2->name_phrase = s_append($2->name_phrase, s_to_c($1));
				s_free($1);
				$$ = $2;
			}
			;

phrase:
			  word
				{ $$ = $1; }
			| phrase word
			{
				string *ns = s_xappend((string*)0, s_to_c($1), " ", s_to_c($2), (char*)0);
				$$ = ns;
				s_free($1);
				s_free($2);
			}
			;

route_addr:
			  '<' addr_spec '>'
				{ $$ = $2; }
			| '<' route addr_spec '>'
			{
				prepr822_domain(&($3->route), $2);
				$$ = $3;
			}
			| '<' '>'
			{
				r822_address *ap;
				ap = new_r822_address();
				ap->flags |= flags;
				ap->flags |= r822_IS_ANONYMOUS;
				$$ = ap;
			}
			;

route:
			route_list ':'
				{ $$ = $1; }
			;

route_list:
			  '@' domain
				{ $$ = $2; }
			| route_list ',' '@' domain
			{
				appr822_domain(&($1), $4);
				$$ = $1;
			}
			;

addr_spec:
			  local_part '@' domain
			{
				r822_address *ap;
				ap = new_r822_address();
				ap->flags |= flags;
				s_reset(ap->local_part);
				ap->local_part = s_append(ap->local_part, s_to_c($1));
				s_free($1);
				ap->domain_part = $3;
				$$ = ap;
			}
			| local_part
			{
				r822_address *ap;
				ap = new_r822_address();
				ap->flags |= flags;
				s_reset(ap->local_part);
				ap->local_part = s_append(ap->local_part, s_to_c($1));
				s_free($1);
				$$ = ap;
			}
			;

local_part:
			  word
				{ $$ = $1; }
			| local_part '.' word
			{
				string *ns = s_xappend((string*)0, s_to_c($1), ".", s_to_c($3), (char*)0);
				s_free($1);
				s_free($3);
				$$ = ns;
			}

domain:
			  sub_domain
			{
				r822_domain *dp;
				dp = new_r822_domain();
				dp->subdom = new_r822_subthing();
				s_reset(dp->subdom->element);
				dp->subdom->element = s_append(dp->subdom->element, s_to_c($1));
				s_free($1);
				domain_literal_bookkeeping(dp->subdom);
				if (dp->subdom->flags & r822_IS_DOMLIT)
				{
					s_reset(dp->dotted);
					dp->dotted = s_xappend(dp->dotted, "[", s_to_c(dp->subdom->element), "]", (char*)0);
				}
				else
				{
					s_reset(dp->dotted);
					dp->dotted = s_append(dp->dotted, s_to_c(dp->subdom->element));
				}
				$$ = dp;
			}
			| domain '.' sub_domain
			{
				r822_subthing *sp;
				sp = new_r822_subthing();
				sp->element = s_append(sp->element, s_to_c($3));
				s_free($3);
				domain_literal_bookkeeping(sp);
				if (sp->flags & r822_IS_DOMLIT)
				{
					($1)->dotted = s_xappend(($1)->dotted, ".[", s_to_c(sp->element), "]", (char*)0);
				}
				else
				{
					($1)->dotted = s_xappend(($1)->dotted, ".", s_to_c(sp->element), (char*)0);
				}
				appr822_subdom(&($1), sp);
				$$ = $1;
			}
			;

sub_domain:
			  domain_ref
			{
				s_putc($1, ' ');
				s_terminate($1);
				$$ = $1;
			}
			| LIT_DOMAIN
			{
				yylval.yString = s_append(yylval.yString, "[");
				$$ = yylval.yString;
			}
			;

domain_ref:
			  ATOM
				{ $$ = yylval.yString; }
			;

word:
			  ATOM
				{ $$ = yylval.yString; }
			| QUOTED_STRING
				{ $$ = yylval.yString; }
			;
%%

static void
yyerror_r822(s)
char *s;
{
	char utb[20];
#define STACK (yylval.yString)

	errstr = s_append(errstr, s);
	switch(yychar) {
	default:
		sprintf(utb, (Isprint(yylval.yChar) ? "%c" : "0x%.2x"), yylval.yChar);
		errstr = s_xappend(errstr, ": unexpected CHARACTER \'",  utb, "\'", (char*)0);
		break;
	case LIT_DOMAIN:
		if (s_curlen(yylval.yString) > 1)
		{
			errstr = s_xappend(errstr, ": unexpected DOMAIN LITERAL [", s_to_c(STACK)+1, "]", (char*)0);
		}
		else
		{
			errstr = s_append(errstr, ": unexpected DOMAIN LITERAL []");
		}
		break;
	case QUOTED_STRING:
		errstr = s_xappend(errstr, ": unexpected QUOTED STRING \"", s_to_c(STACK), "\"", (char*)0);
		break;
	case ATOM:
		errstr = s_xappend(errstr, ": unexpected ATOM '", s_to_c(STACK), "'", (char*)0);
		break;
	case EOL: case 0: /* EOF */
		errstr = s_append(errstr, ": unexpected end-of-input");
		break;
	}
	errstr = s_xappend(errstr, "; detected near byte #", int_to_str(is_dex), (char*)0);
}

int
r822_addr_parse(line, parse_flags, alist)
string *line;
int parse_flags;
r822_address **alist;
{
	int rc;
	input_string = line;
	is_length = s_curlen(input_string);  /* doesn't change while parsing */
	is_dex = 0;  /* next character from input string is byte #0 */
	if (flags & r822_SKIP_NAME)
	{
		for ( ; is_dex < is_length; is_dex++)
			if (s_to_c(input_string)[is_dex] == ':')
			{
				is_dex++;
				break;
			}
	}
	adrlist = 0;
	errstr = s_new();
	comment_list = 0;
	prev_ttype = ERROR;
	flags        = parse_flags;
	comma_hack   = ~(flags & r822_STRICT_COMMA);
	loose_qpairs =  (flags & r822_LOOSE_QPAIRS);
	loose_lwsp   =  (flags & r822_LOOSE_LWSP);
	loose_domlit =  (flags & r822_LOOSE_DOMLIT);
	strict_colon =  (flags & r822_STRICT_COLON);
	strict_7bit  =  (flags & r822_STRICT_7BIT);
	rc = yyparse();
	s_delete(errstr);
	*alist = adrlist;
	return (rc);
}

static int
gather_comments()
{
	unsigned char this_char;
	int paren_count;
	string *growing_comment = s_new();

	/* may have already seen an opening '(', so back up one */
	for (--is_dex; ;)
	{
		if (is_dex >= is_length)
		{
			s_free(growing_comment);
			return (EOL);
		}
		this_char = THIS_CHAR(is_dex++);
		if (IS_LWSP_CHAR(this_char))
		{
			/* skipping over LWSP */
			continue;
		}
		else if (this_char == '(')
		{
			/* gather up a balanced comment */
			for (paren_count = 1; paren_count; )
			{
				if (is_dex >= is_length)
				{
					s_free(growing_comment);
					return (ERROR);
				}
				this_char = THIS_CHAR(is_dex++);
				switch (this_char)
				{
				case '(':
					if (LAST_CHAR_OF(growing_comment) != '\\') ++paren_count;
					break;
				case ')':
					if (LAST_CHAR_OF(growing_comment) != '\\') --paren_count;
					break;
				default:
					break;
				} /* switch */
				if (paren_count > 0)
				{
					s_putc(growing_comment,this_char);
					s_terminate(growing_comment);
				}
			} /* for */
			/*
			** Got to the end of a single comment.  Accumulate it with
			** other comments for this address.
			*/
			if (!comment_list)
			{
				comment_list = new_r822_subthing();
				s_reset(comment_list->element);
				comment_list->element = s_append(comment_list->element, s_to_c(growing_comment));
			}
			else
			{
				/*
				** Gather (multi) (comments strings) into
				** ((multi) (comment strings)).
				*/
				r822_subthing *cp;
				for (cp = comment_list; cp->next; cp = cp->next)
				{
					;  /* get to the dangling pointer on the list */
				}
				cp->next = new_r822_subthing();
				s_reset(cp->next->element);
				cp->next->element = s_append(cp->next->element, s_to_c(growing_comment));
			}
			s_reset(growing_comment);
		}
		else
		{
			--is_dex;  /* don't lose the character following the LWSP */
			break;  /* out of outter for(); saw something outside a comment */
		}
	} /* outer for() */
	s_free(growing_comment);
	return (0);
}

static int
gather_quoted_string()
{
	unsigned char this_char;
	string *growing_quoted_string = s_new();
	for (;;)
	{
		if (is_dex >= is_length)
		{
			s_free(growing_quoted_string);
			return (ERROR);
		}
		this_char = THIS_CHAR(is_dex++);
		if (this_char == '"' &&  LAST_CHAR_OF(growing_quoted_string) != '\\') break;
		s_putc(growing_quoted_string, this_char);
	}
	s_terminate(growing_quoted_string);
	yylval.yString = growing_quoted_string;
	return (QUOTED_STRING);
}

static int
gather_domain_literal()
{
	unsigned char this_char;
	string *growing_domain_literal = s_new();
	/* already saw the opening '[' */
	for (;;)
	{
		if (is_dex >= is_length)
		{
			s_free(growing_domain_literal);
			return (ERROR);
		}
		this_char = THIS_CHAR(is_dex++);
		/*
		** RFC-822 says '[' must be quoted inside a domain literal, but
		** there's no real reason to enforce that, so I'm not bothering
		** to check.  Pedants could insist that they should get a syntax
		** error if they submit one, but, hey, "be liberal in what you
		** accept".
		*/
		if (this_char == ']' && LAST_CHAR_OF(growing_domain_literal) != '\\')
		{
			break;
		}
		s_putc(growing_domain_literal,this_char);
	}
	s_terminate(growing_domain_literal);
	yylval.yString = growing_domain_literal;
	return (LIT_DOMAIN);
}

static int
gather_atom()
{
	unsigned char this_char;
	static int atom_hadda_bang = 0;
	string *growing_atom = s_new();

	--is_dex; /* don't lose the ATOM's first character */
	if (comma_hack)
	{
		if (prev_ttype == ATOM  &&  atom_hadda_bang)
		{
			s_free(growing_atom);
			return (PHANTOM_COMMA);
		}
	}
	atom_hadda_bang = 0;

#define RETURN_ATOM \
			--is_dex;  /* don't lose the first char of ATOM's follower */\
			s_terminate(growing_atom);\
			yylval.yString = growing_atom;\
			return (ATOM)

	for (;;)
	{
		if (is_dex >= is_length)
		{
			++is_dex;
			RETURN_ATOM;
		}
		this_char = THIS_CHAR(is_dex++);
		switch (this_char)
		{
		case ',':  case ';':  case '.':  case '@':
		case '<':  case '>':  case '(':  case ')':
		case '[':  case '"':
			RETURN_ATOM;

		case '\\':
			if (!loose_qpairs)
			{
				RETURN_ATOM;
			}
			s_putc(growing_atom,this_char);
			break;
			
		case ':':
			if (strict_colon || !atom_hadda_bang)
			{
				RETURN_ATOM;
			}
			s_putc(growing_atom,this_char);
			break;

		case '!':
			++atom_hadda_bang;
			s_putc(growing_atom,this_char);
			break;

		default:
			if (IS_LWSP_CHAR(this_char))
			{
				RETURN_ATOM;
			}
			s_putc(growing_atom,this_char);
			break;
		} /* switch */
	} /* for */
}


static int
yylex_r822()
{
	/*
	** General lexing strategy:
	**
	** 		If looking at LWSP or an open paren, get past the space
	** 		and/or comment (comments are accumulated in a global
	** 		variable (ick)).
	**
	** 		If looking at a "special" character not otherwise
	** 		accomodated below, return it literally.
	**
	** 		If looking at a doulbe quote, this is a a quoted-string,
	** 		so gather that up and return it.
	**
	** 		If looking at an open square bracket, this is a domain
	** 		literal, so gather that up and return it.
	**
	** 		Otherwise, gather up an ATOM and return it.
	**
	** Lexer leaves "is_dex" referencing the next character after the
	** current token.
	*/
	int rc;
	unsigned char this_char;

	if (prev_ttype == EOL)
	{
		prev_ttype = ERROR;
		return (0);  /* tells yacc we're done */
	}

	if (is_dex >= is_length)
	{
		return (prev_ttype = EOL);
	}
	this_char = THIS_CHAR(is_dex++);
	

	if (IS_LWSP_CHAR(this_char)  ||  this_char == '(')
	{
		rc = gather_comments();
		if (rc != 0)
		{
			return (prev_ttype = rc);
		}
		if (is_dex >= is_length)
		{
			return (prev_ttype = EOL);
		}
		this_char = THIS_CHAR(is_dex++);
	}

	switch (this_char)
	{
	case '"':
		rc = gather_quoted_string();
		break;

	case '[':
		rc = gather_domain_literal();
		break;

	case '(':   case ')':   /* parens shoudn't happen; stripped off above */
	case '<':   case '>':   case ']':   case '@':
	case ',':   case ';':   case '.':   case ':':
		yylval.yChar = this_char;
		rc = this_char;
		break;
		
	case '\\':
		if (loose_qpairs)
		{
			rc = gather_atom();
		}
		else
		{
			yylval.yChar = this_char;
			rc = this_char;
		}
		break;

	default:
		rc = gather_atom();
		break;
	}
	return (prev_ttype = rc);
}

static void
check_domlit_rules(addr)
r822_address *addr;
{
	/*
	** Strict checking for domain literal rules.  This applies the
	** RFC-821 restrictions.  RFC-822 allows just about anything in a
	** domain literal.  Per RFC-821, a domain literal must be
	**
	** 		[snum.snum.snum.snum]
	**
	** where each snum is a 1, 2, or 3 digit decimal integer in the range
	** [0,255].  Further, RFC-821 requires that the domain literal be the
	** whole domain part (well, the BNF doesn't actually require that,
	** but the explanatory text sort of implies it; no other sensible
	** interpretation available).  (In the subdomain strings we store,
	** the surrounding brackets aren't included, but the subdomain is
	** marked as a domain literal or otherwise.)
	**
	** NB:  I wanted to implement this with the Regex class, but that
	** wasn't available in the lib++ I had available at the time.
	*/
	const char *dlerror = "DOMAIN LITERAL must be IP dotted quartet [m.n.p.q]";
	r822_domain *dp;
	r822_subthing *sp;
	int subdoms, numdigs, numdots, number, dldex;
	for (dp = addr->domain_part; dp; dp = dp->next)
	{
		subdoms = 0;
		for (sp = dp->subdom; sp; sp = sp->next)
		{
			++subdoms;
			if (!(sp->flags & r822_IS_DOMLIT)) continue;
			if (subdoms>1  ||  sp->next)
			{
				/* more than one subdomain */
				addr->error = s_append(addr->error, dlerror);
				addr->flags |= r822_IS_ERROR;
				return;
			}
			number  = 0;
			numdigs = 0;
			numdots = 0;
			for (dldex = 0; dldex<s_curlen(sp->element); ++dldex)
			{
				unsigned char this_char;
				this_char = s_to_c(sp->element)[dldex];
				switch (this_char)
				{
				default:
					addr->error = s_append(addr->error, dlerror);
					addr->flags |= r822_IS_ERROR;
					return;
				case '.':
					++numdots;
					if (numdots > 3  ||  numdigs == 0)
					{
						addr->error = s_append(addr->error, dlerror);
						addr->flags |= r822_IS_ERROR;
						return;
					}
					numdigs = 0;
					number  = 0;
					break;
				case '0':   case '1':   case '2':   case '3':   case '4':
				case '5':   case '6':   case '7':   case '8':   case '9':
					++numdigs;
					number = (10 * number) + (this_char - '0');
					if (numdigs > 3  ||  number > 255)
					{
						addr->error = s_append(addr->error, dlerror);
						addr->flags |= r822_IS_ERROR;
						return;
					}
					break;
				}
			}
			if (numdots != 3  ||  numdigs>3  ||  number > 255)
			{
				addr->error = s_append(addr->error, dlerror);
				addr->flags |= r822_IS_ERROR;
				return;
			}
		}
	}
}


/*
**  Append addresslist "addr" to addresslist "head".
*/
static void
appr822_address(head, addr)
r822_address **head;
r822_address *addr;
{
	r822_address *ap;
	if (!loose_domlit)
	{
		check_domlit_rules(addr);
	}
	if (*head)
	{
		for (ap = *head; ap->next; ap = ap->next)
		{
			; /* empty */
		}
		ap->next = addr;
	}
	else
	{
		*head = addr;
	}
}

static void
appr822_subdom(head, subdom)
r822_domain **head;
r822_subthing *subdom;
{
	r822_subthing *sp;
    if ((*head)->subdom)
	{
		for (sp = (*head)->subdom; sp->next; sp = sp->next)
		{
			; /* empty */
		}
		sp->next = subdom;
    }
    else
	{
		(*head)->subdom = subdom;
	}
}

/*
**  Append domainlist "dom" to domainlist "head".
*/
static void
appr822_domain(head, dom)
r822_domain **head;
r822_domain *dom;
{
	r822_domain *dp;

    if (*head)
	{
		for (dp = *head; dp->next; dp = dp->next)
		{
			; /* empty */
		}
		dp->next = dom;
    }
    else
	{
		*head = dom;
	}
}


/*
**  Prepend domainlist "dom" before domainlist "head".
*/
static void
prepr822_domain(head, dom)
r822_domain **head;
r822_domain *dom;
{
	r822_domain *dp;

    for (dp = dom; dp->next; dp = dp->next)
	{
		; /* empty */
	}
    dp->next = *head;
    *head = dom;
}

static void
domain_literal_bookkeeping(sp)
r822_subthing *sp;
{
	if (s_curlen(sp->element) != 0)
	{
		char last;
		s_skipback(sp->element);
		last = s_ptr_to_c(sp->element)[0];		/* tear off last byte */
		s_terminate(sp->element);
		if (last == '[')
		{
			/* Mark this as a domain literal */
			sp->flags |= r822_IS_DOMLIT;
		}
	}
}
