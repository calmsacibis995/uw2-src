/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)acpp:common/xpr.c	1.51.1.3"
/* xpr.c - parse and evaluate constant expressions */

#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include "cpp.h"
#include "syms.h"
#include "file.h"

#define BITMASK(n) (((n)==SZLONG)?-1L:((1L<<(n))-1))
#ifdef	DEBUG
#ifdef __STDC__
#	define DBGCALL(num,funcname,tp)\
		if ( DEBUG('x') > (num) )\
		{\
			(void)fprintf(stderr, #funcname "() called with: ");\
			tk_pr(tp, '\n');\
		}
#	define DBGCALLL(num,funcname,tp)\
		if ( DEBUG('x') > (num) )\
		{\
			(void)fprintf(stderr, #funcname "() called with: ");\
			tk_prl(tp);\
		}
#	define DBGRET(num,funcname,retval)\
                if ( DEBUG('x') > (num) )\
		{\
			(void)fprintf(stderr, #funcname "() returns:");\
			prifval(retval);\
		}
#	define DBGRETN(num,funcname,retnum)\
		if ( DEBUG('x') > (num) )\
			(void)fprintf(stderr, #funcname "() return value = %ld\n",(retnum) );
#else
#	define DBGCALL(num,funcname,tp)\
		if ( DEBUG('x') > (num) )\
		{\
			(void)fprintf(stderr, "funcname() called with: ");\
			tk_pr(tp, '\n');\
		}
#	define DBGCALLL(num,funcname,tp)\
		if ( DEBUG('x') > (num) )\
		{\
			(void)fprintf(stderr, "funcname() called with: ");\
			tk_prl(tp);\
		}
#	define DBGRET(num,funcname,retval)\
                if ( DEBUG('x') > (num) )\
		{\
			(void)fprintf(stderr, "funcname() returns:");\
			prifval(retval);\
		}
#	define DBGRETN(num,funcname,retnum)\
		if ( DEBUG('x') > (num) )\
			(void)fprintf(stderr, "funcname() return value = %ld\n",(retnum) );
#endif
#else
#	define DBGCALL(num,funcname,tp)
#	define DBGCALLL(num,funcname,tp)
#	define DBGRET(num,funcname,retval)		
#	define DBGRETN(num,funcname,retnum)		
#endif
#define SZCHAR 		8
#define SZLONG		SZINT
#define SZINT 		32

#ifdef __STDC__

typedef enum _Action {
	A_reduce,
	A_shift,
	A_accept,
	A_error
} Action;
typedef enum _Boolean {
	B_false,
	B_true
} Boolean;
typedef enum _Expect {
	E_number,
	E_operator
} Expect;
typedef enum _Type {
	T_signed,
	T_unsigned
} Type;

#else	/* 2.1 disallows assignment of enum member to an int */

extern unsigned long strtoul();	/* ANSI C library routine */
typedef long wchar_t;

typedef int	 Action;
typedef int	 Boolean;
typedef int	 Expect;
typedef int	 Type;
#	define	A_reduce	0
#	define	A_shift		1
#	define	A_accept	2
#	define	A_error		3
#	define	B_false		0
#	define	B_true		1
#	define	E_number	0
#	define	E_operator	1
#	define	T_signed	0
#	define	T_unsigned	1
#endif

/* Parses and evaluates the constant expression in condition inclusion
** directives. Operator precedence parsing is used.
** Evaluation is done by a push-down automaton that has
** two stacks, one for operators and the other for operands.
*/

/* WARNING: struct tree is very closely associated with the Token structure.  
** If the Token structure is changed, most likely a change will be
** necessitated in struct tree.  A runtime check is made in xp_value()
** to ensure the fields line up as is required.
*/

typedef struct tree {
	struct tree *next;	/* same offset as Token.next */
	struct tree *left;	/* left subtree, same offset as Token.ptr */
	struct tree *right;	/* pointer to right subtree, overwrites rlen
				 * and use fields, which are no longer needed
				 * by the time we are doing #if processing.
				 */

			/* The following three fields are used exactly
			** as they are used in Token.  They must be the
			** same size and the same type.
			*/
	unsigned short code;	/* same offset as Token.code */
	Aux aux;		/* same offset as Token.aux */
	long number;		/* same offset as Token.number */
} Tree;

static Token *	curtoken;	/* current token */
static Tree *	curtree;	/* current tree	*/
static Expect	expect;		/* what next token is expected to be */
static int	input_op;	/* code corresponding to curtoken */
static Tree *	nstp;		/* top of number stack	*/
static Tree * 	ostp;		/* top of operator stack*/

static	Action 	action();
static	void	freetree();
static	void	lxchar();
static  int	precedence();
#ifdef DEBUG
static  void  	prifval();
#endif
static	void 	primary();
static	void  	reduce();
static	Action	reducechk();
static	void  	shift();
static	Boolean unsgnd();

static const int 	prec[]= { /* precedence table */
	10,	/* C_RParen	 )	*/
	11,	/* C_Comma	 ,	*/ /* not allowed in ANSI C */
	12,	/* C_Question	 ?	*/
	12,	/* C_Colon	 :	*/
	13,	/* C_LogicalOR	 ||	*/
	14,	/* C_LogicalAND	 &&	*/
	15,	/* C_InclusiveOR |	*/
	16,	/* C_ExclusiveOR ^	*/
	17,	/* C_BitwiseAND	 &	*/
	18,	/* C_Equal	 ==	*/
	18,	/* C_NotEqual	 !=	*/
	19,	/* C_GreaterThan >	*/
	19,	/* C_GreaterEqual>=	*/
	19,	/* C_LessThan	 <	*/
	19,	/* C_LessEqual	 <=	*/
	20,	/* C_LeftShift	 <<	*/
	20,	/* C_RightShift	 >>	*/
	21,	/* C_Plus	 +	*/
	21,	/* C_Minus	 -	*/
	22,	/* C_Mult	 *	*/
	22,	/* C_Div	 /	*/
	22,	/* C_Mod	 %	*/
	23,	/* C_UnaryPlus	 +	*/
	23,	/* C_UnaryMinus	 -	*/
	23,	/* C_Complement	 ~	*/
	23,	/* C_Not	 !	*/
	24,	/* C_LParen 	 (	*/
	100	/* C_Operand	any integral constant	*/
};

static Action
action()
/* This routine examines the current input Token and judges what
** action the automaton should take.
** (i.e shift, reduce, accept or give an error).
*/
{
#ifdef DEBUG
	if ( DEBUG('x') > 4 )
	{
		(void)fprintf( stderr, "action() called on: ");
		if ( curtoken )
			tk_pr( curtoken, '\n' );
		else
			fputc( '\n', stderr );
	}
#endif
	if (curtoken == 0)
	{
		if (ostp == 0)
			return A_accept;
		else if (expect == E_number)
		{
			UERROR(gettxt(":526","number expected"));
			return A_error;
		}
		else
			return reducechk();
	}
	if (TK_CONDEXPR(curtoken))
		if (TK_ISCONSTRAINED(curtoken))
			input_op = C_Operand;
		else
			input_op = curtoken->code;
	else if (curtoken->code == C_Sharp)
	{
		COMMENT(fl_numerrors() >= 1);
		return A_error;
	}
	else
	{
		TKERROR(gettxt(":527","token not allowed in directive"), curtoken);
		return A_error;
	}
	switch (input_op)
	{
	case C_Operand:
	case C_Not:
	case C_Complement:
		if ( expect == E_operator )
		{
			UERROR(gettxt(":528","missing operator" ));
			return A_error;
		}
		break;

	case C_RParen:
		if ( expect == E_number )
		{
			UERROR(gettxt(":529", "unexpected \")\"" ));
			return A_error;
		}
		break;

	case C_Plus:
		if ( expect == E_number )	input_op = C_UnaryPlus;
		break;

	case C_Minus:
		if ( expect == E_number )	input_op = C_UnaryMinus;
		break;

	case C_LParen:
		if ( expect == E_operator )
		{
			UERROR(gettxt(":530", "unexpected \"(\"" ));
			return A_error;
		}
		break;

	default:if(input_op >= C_Question && input_op <= C_Mod && expect==E_number)
		{	/* for now - is there an easier way to say this ?- binary ops ? */
			UERROR(gettxt(":531","missing operand"));
			return A_error;
		}
	}
	if  ((ostp == 0) || (precedence((int)ostp->code) < precedence(input_op)))
		return A_shift;
	else if (precedence((int)ostp->code) > precedence(input_op))
		return reducechk();
	else 
		switch (input_op)
		{
		case C_Colon:
			if (ostp->code == C_Colon)	return reducechk();
			/*FALLTHRU*/
		case C_RParen:
		case C_UnaryPlus:
		case C_UnaryMinus:
		case C_Complement:
		case C_Not:
			return A_shift;

		case C_Question:
			if (	(ostp->code == C_Question) ||
			     	(ostp->code == C_Colon)	
			   )	
				return A_shift;
		default:return reducechk();
		}
}

static  void
eval(tree)
register Tree *tree;
/* Recursively evaluates the expression tree rooted at tree.  Put the numeric
** result in tree->number.  Free left and right subtrees.
*/
{
	register Tree *left = tree->left;
	register Tree *right = tree->right;
#ifdef DEBUG
	if ( DEBUG('x') > 1 )
		(void)fprintf(stderr, "eval() called on: %13s\n",
			tk_saycode(tree->code));
#endif
	switch (TK_ENUMNO(tree->code))
	{
	CASE(C_InclusiveOR)
	CASE(C_ExclusiveOR)
	CASE(C_BitwiseAND)
	CASE(C_Equal)
	CASE(C_NotEqual)
	CASE(C_GreaterThan)
	CASE(C_GreaterEqual)
	CASE(C_LessThan)
	CASE(C_LessEqual)
	CASE(C_LeftShift)
	CASE(C_RightShift)
	CASE(C_Plus)
	CASE(C_Minus)
	CASE(C_Mult)
	CASE(C_Div)
	CASE(C_Mod)
		eval(right);
		/*FALLTHROUGH*/
	CASE(C_Question)
	CASE(C_LogicalOR)
	CASE(C_LogicalAND)
	CASE(C_UnaryMinus)
	CASE(C_Complement)
	CASE(C_Not)
		eval(left);
		break;
	}
	switch (TK_ENUMNO(tree->code))
	{
	CASE(C_Question) 
		if (left->number) {
			eval(right->left);
			tree->number = right->left->number;
		}
		else {
			eval(right->right);
			tree->number = right->right->number;
		}
		break;

	CASE(C_LogicalOR) 
		if (left->number)
			tree->number = 1;
		else {
			eval(right);
			tree->number = (right->number != 0);
		}
		break;

	CASE(C_LogicalAND) 
		if (left->number == 0)
			tree->number = 0;
		else {
			eval(right);
			tree->number = right->number != 0;
		}
		break;

	CASE(C_InclusiveOR) 
    		if ( unsgnd(tree) )
		{
			tree->number = 
				(unsigned long) left->number |
				(unsigned long) right->number;
    		}
    		else
		{
			tree->number = 
				left->number |
				right->number;
		}
		break;

	CASE(C_ExclusiveOR) 
    		if ( unsgnd(tree) )
		{
			tree->number = 
				(unsigned long) left->number ^
				(unsigned long) right->number;
    		}
    		else
		{
			tree->number = 
				left->number ^
				right->number;
		}
		break;

	CASE(C_BitwiseAND) 
    		if ( unsgnd(tree) )
		{
			tree->number = 
				(unsigned long) left->number &
				(unsigned long) right->number;
    		}
    		else
		{
			tree->number = 
				left->number &
				right->number;
		}
		break;

	CASE(C_Equal) 
		tree->number = 
			left->number == right->number;
		break;

	CASE(C_NotEqual) 
		tree->number = 
			left->number != right->number;
		break;

	CASE(C_GreaterThan) 
    		if ( unsgnd(tree) )
			tree->number =
			  (unsigned long) left->number > 
			  (unsigned long) right->number;
    		else
			tree->number =
			  left->number > 
			  right->number;
		break;

	CASE(C_GreaterEqual) 
    		if ( unsgnd(tree) )
			tree->number =
			  (unsigned long) left->number >=
			  (unsigned long) right->number;
    		else
			tree->number =
			  left->number >= 
			  right->number;
		break;

	CASE(C_LessThan) 
    		if ( unsgnd(tree) )
			tree->number =
			  (unsigned long) left->number <
			  (unsigned long) right->number;
    		else
			tree->number =
			  left->number <
			  right->number;
		break;

	CASE(C_LessEqual) 
    		if ( unsgnd(tree) )
			tree->number =
			  (unsigned long) left->number <=
			  (unsigned long) right->number;
    		else
			tree->number =
			  left->number <=
			  right->number;
		break;

	CASE(C_LeftShift) 
    		if ( unsgnd(tree) )
		{
			tree->number =
			  (unsigned long) left->number <<
			  (unsigned long) right->number;
		}
    		else
		{
			tree->number =
			  left->number <<
			  right->number;
		}
		break;

	CASE(C_RightShift) 
    		if ( unsgnd(tree) )
		{
			tree->number =
			  (unsigned long) left->number >>
			  (unsigned long) right->number;
		}
    		else
		{
			tree->number =
			  left->number >>
			  right->number;
#ifdef			C_SIGNED_RS
			/* 
			** simulate signed right shift.
			*/
			if (left->number < 0)
				tree->number |= ~((~0) >> right->number);
#endif
		}
		break;

	CASE(C_Plus) 
    		if ( unsgnd(tree) )
		{
			tree->number =
			  (unsigned long) left->number +
			  (unsigned long) right->number;
		}
    		else
		{
			tree->number =
			  left->number +
			  right->number;
		}
		break;

	CASE(C_Minus) 
    		if ( unsgnd(tree) )
		{
			tree->number =
			  (unsigned long) left->number -
			  (unsigned long) right->number;
		}
    		else
		{
			tree->number =
			  left->number -
			  right->number;
		}
		break;

	CASE(C_Mult) 
    		if ( unsgnd(tree) )
		{
			tree->number =
			  (unsigned long) left->number *
			  (unsigned long) right->number;
		}
    		else
		{
			tree->number =
			  left->number *
			  right->number;
		}
		break;

	CASE(C_Div) 
		if ( !right->number )
		{
			WARN(gettxt(":532","division by zero"));
			tree->number = 0;
		}
    		else if ( unsgnd(tree) )
		{
			/* Workaround an Amdahl compiler bug: force it
			** to generate an unsigned divide.
			*/
			unsigned long l = left->number;
			unsigned long r = right->number;
			tree->number = l / r;
		}
    		else
		{
			tree->number =
			  left->number /
			  right->number;
		}
		break;

	CASE(C_Mod) 
		if ( !right->number )
		{
			WARN(gettxt(":533","modulus by zero"));
			tree->number = 0;
		}
    		else if ( unsgnd(tree) )
		{
			/* Workaround an Amdahl compiler bug: force it
			** to generate an unsigned mod.
			*/
			unsigned long l = left->number;
			unsigned long r = right->number;
			tree->number = l % r;
		}
    		else
		{
			tree->number =
			  left->number %
			  right->number;
		}
		break;

	CASE(C_UnaryPlus) 
#ifdef DEBUG
		fprintf(stderr, "UnaryPlus in eval()\n");
#endif
		break;

	CASE(C_UnaryMinus) 
		if ( left->aux.type == T_unsigned )
			tree->number = - (unsigned long) left->number;
		else
			tree->number = - left->number;
		break;

	CASE(C_Complement) 
		if ( left->aux.type == T_unsigned )
			tree->number = ~ (unsigned long) left->number;
		else
			tree->number = ~ left->number;
		break;

	CASE(C_Not) 
		tree->number = ! left->number;
		break;

	CASE(C_LParen)
		FATAL(gettxt(":736","C_LParen in eval()"), "");
		break;
	}
	tree->code = C_Operand;
	if (left)
		freetree(left);
	if (right)
		freetree(right);
	tree->left = tree->right = 0;
#ifdef DEBUG
	if ( DEBUG('x') > 2 )
	{
		(void)fprintf(stderr, "eval() returns %ld\n", tree->number);
	}
#endif
}

static void
freetree(t)
register Tree *t;
/* Recursively free tree rooted at t. */ 
{
	if (t->left)
		freetree(t->left);
	if (t->right)
		freetree(t->right);
	(void) tk_rm((Token *)t);
	t->left = t->right = 0;
}

static unsigned long
escape(p) 
char **p;
/* Return the escaped value of *p.  Also, make sure that *p is the
** next input character upon return.
*/
{
	register char *cp = *p;
	register unsigned long val;

	switch( *cp )
	{
#ifdef DEBUG
	case '\n':
		FATAL("new-line gotten by bf_tokenize()", "");
		/*NOTREACHED*/
#endif
	default: if (isprint(*cp))
			WARN(gettxt(":259", "dubious escape: \\%c"), *cp );
		else
			WARN(gettxt(":260", "dubious escape: \\<%#x>"), *cp );
		val = *cp;
		break;
	case '?':
	case '\'':
	case '"':
	case '\\':
		val = *cp;
		break;
	case 'n':
		val = '\n';
		break;
	case 'r':
		val = '\r';
		break;
	case 'a':
#ifdef __STDC__
		val = '\a';
#else
		val = '\007';
#endif
#ifdef TRANSITION
		if ( pp_flags & F_Xt )
			WARN(gettxt(":258","\\a is ANSI C \"alert\" character"));
#endif
		break;
	case 'b':
		val = '\b';
		break;
	case 't':
		val = '\t';
		break;
	case 'f':
		val = '\f';
		break;
	case 'v':
		val = '\v';
		break;
	case '0': case '1': case '2': case '3':
	case '4': case '5': case '6': case '7':
		val = *cp-'0';
		cp++;
		if( *cp >= '0' && *cp <= '7' ) {
			val = (val << 3) | (*cp-'0');
			cp++; 
			if( *cp >= '0' && *cp <= '7' )
				val = (val << 3) | (*cp-'0');
			else 
				cp--;
		}
		else 
			cp--;
		break;

	case 'x':
	{
		int n = 0;	/* seen digit? */
		int overflow = 0;
		unsigned long digit;

		val = 0;
		for (;;)
		{
			switch (*++cp)
			{
			case '0': case '1': case '2': case '3':
			case '4': case '5': case '6': case '7':
			case '8': case '9':
				digit = *cp - '0';
				break;

			case 'a': case 'b': case 'c':
			case 'd': case 'e': case 'f':
				digit = 10 + *cp - 'a';
				break;

			case 'A': case 'B': case 'C':
			case 'D': case 'E': case 'F':
				digit = 10 + *cp - 'A';
				break;

			default:cp--;
				goto end_hex;
			}
			n = 1;
			if (val & (~((unsigned long)(~0L) >> 4)))
				overflow = 1;
			val = (val << 4) | digit;
		}
end_hex: 	
#ifdef TRANSITION
		if ( pp_flags & F_Xt )
			WARN(gettxt(":261", "\\x is ANSI C hex escape" ));
		else
#endif
		if (n == 0) 
			WARN(gettxt(":262", "no hex digits follow \\x" ));
		if (n == 0)	
			val = 'x';
		else if (overflow)
			WARN(gettxt(":263", "overflow in hex escape" ));
		break;
	} /* end case */
	} /* end switch */
	*p = cp;
	return val;
}

static void
lxchar()
/* Evaluate character constant input Token.
** The behavior of multiple characters in a character constant
** is implementation defined, as is an escape sequence not represented
** in the execution character set.
*/
{
    	char * cp;	/* character in char-constant */
	register unsigned long val;	/* value of char-constant */
	register i;	/* number of characters in char-constant */
	
	static const char toolong[] = "character constant too long"; /* WARN */
	static const char empty[] = "empty character constant"; /* UERROR */

	DBGCALL(4,lxchar,curtoken);
	COMMENT(curtoken->code == C_C_Constant);
    	curtoken->number = 0;
    	curtoken->code  = C_Operand;
    	curtoken->aux.type  = T_signed;
	cp = curtoken->ptr.string;
	i=0;
	if (*cp == 'L')		/* multibyte char constant */
	{
		unsigned int len;
		int wclen;
		wchar_t wc;

		len = curtoken->rlen - 3;
		cp += 2;
		if (len == 0) 
			UERROR(gettxt(":111",empty));
		else if (*cp == '\\') {
			++cp;
			curtoken->number = escape(&cp);
			if (*cp != '\'')
				WARN(gettxt(":397",toolong));
		}
		else {
			wclen = mbtowc(&wc, cp, len);
			curtoken->number = wc;
			if (len > 1)
				WARN(gettxt(":397",toolong));
			else if (wclen <= 0)  {
				UERROR(gettxt(":256", "invalid multibyte character"));
				curtoken->number = 0;
			}
		}
		DBGRET(0, lxchar, curtoken);
		return;
	}
		
	while( *++cp  != '\'' )
	{
		switch( *cp )
		{
		case '\\':
			++cp;
			val = escape(&cp);
			if (val & (~BITMASK(SZCHAR))) {
				WARN(gettxt(":255","character escape does not fit in character"));
				val &= BITMASK(SZCHAR);	/* truncate value */
			}
			break;
		default:
			val = *cp;
			break;
		}
		/* sign extend */
		if (chars_signed && (val & (1 << (SZCHAR - 1))))
			val |= ~BITMASK(SZCHAR);
		if (i == 0)
			curtoken->number = val;
		else if (i < SZINT/SZCHAR)
		{
#ifdef RTOLBYTES
			curtoken->number <<= SZCHAR;
			curtoken->number |= val & BITMASK(SZCHAR);
#else
			curtoken->number &= BITMASK(SZCHAR*i);
			curtoken->number |= val << (SZCHAR*i);
#endif
		}
		++i;
	}
	if( i == 0 )
		UERROR(gettxt(":111",empty));
	else if( i > 1 )  {
		TKWARN(gettxt(":534","more than one character honored in character constant"),
								curtoken);
		if( i > SZINT/SZCHAR)
			WARN(gettxt(":397",toolong));
	}
	DBGRET(3,lxchar,curtoken);
    	return;
}

static int
precedence(code)
	int code;
/* Given a code that identifies a Token, this routine returns
** a precedence of that Token.
*/
{
	return prec[TK_ENUMNO(code) - TK_ENUMNO(C_RParen)];
}

#ifdef DEBUG

static void
prifval(tp)
	Token * tp;
/* Given a pointer to a Token on the number stack,
** this routine prints out both fields,
** their decimal interpretation, and a newline for debugging purposes.
*/
{
	(void)fprintf(stderr, " %#lx", tp->number);
	switch ( tp->aux.type )
	{
	case T_unsigned:
		(void)fprintf(stderr, "(unsigned) = %lu\n", tp->number );
		return;

	case T_signed:
		(void)fprintf(stderr, "(signed) = %ld\n", tp->number );
		return;

	default:FATAL( "bad type code", "" );
	}
}
#endif

static void
primary()
/* This routine returns the value of what the ANSI standard grammar
** calls "primary expressions." This assumes that the current
** input Token is valid - the tokenization routines are
** counted on to catch all syntax errors before this stage of processing.
*/
{
	register Token * tp;	/* current input Token */
	
	DBGCALL(3,primary,curtoken);
    	switch((tp = curtoken)->code)
	{
    	case C_Identifier:
		tp->number = 0x0;
		tp->code = C_Operand;
		tp->aux.type = T_signed;
		DBGRET(1,primary,tp);
		return;

    	case C_C_Constant:
		lxchar();
		return;

	case C_I_Constant:
	{
#define	T_LONG_MAX	2147483647	/* assumes target machine has 32 bit int */
		char * buffer;	/* space for current Token followed by `\0' */
		char * ptr;	/* last number where strtol() stopped */
		char ** pptr;	/* pointer to `ptr' */

		errno = 0;
		pptr = &ptr;
		buffer = ch_saven(tp->ptr.string, tp->rlen);
		buffer[tp->rlen] = '\0';
		tp->number = strtoul(buffer, pptr, 0);
#ifdef TRANSITION
		if (*ptr == '8' || *ptr == '9')
		{
			COMMENT(pp_flags & F_Xt);
			WARN(gettxt(":330", "bad octal digit: '%c'"), *ptr);
			tp->number = 0;
			for (ptr = buffer; *ptr != '\0'; ptr++)
			{
				if (isdigit(*ptr) == 0)
					break;
				tp->number = (tp->number <<= 3) | (*ptr - '0');
			}
		}
#endif
		tp->code = C_Operand;
		tp->aux.type = T_signed;
		switch ( tp->rlen - (ptr - buffer) )
		{
		case 0: break;

		case 2: switch (ptr[1])
			{
			case 'u': 
			case 'U':
				tp->aux.type = T_unsigned;
				/*FALLTHRU*/
			case 'l': 
			case 'L':
				break;
			}
		case 1: switch (ptr[0])
			{
			case 'u': 
			case 'U':
				tp->aux.type = T_unsigned;
				/*FALLTHRU*/
			case 'l': 
			case 'L':
				break;
			}
			break;
#ifdef DEBUG
		default:FATAL("bad constant suffix got by bf_tokenize()", "");
#endif
   		}
		if (((unsigned)tp->number) > T_LONG_MAX)
		{
#ifdef TRANSITION
			if ((pp_flags & F_Xt) && (tp->aux.type == T_signed))
				TKWARN(gettxt(":535", "operand treated as unsigned"), tp);
#endif
			tp->aux.type = T_unsigned;
		}
		DBGRET(2,primary,tp);
	}
    	}
}

static  void
reduce()
/* Reduces by popping the operator stack and (if applicable) popping
** operands off of the number stack and pushing the resulting expression tree.
** Keeps track whether the next token is expected to be an operand.
*/
{
	register Tree *t;
	int isunsigned = -1;	/* -1 not yet check; 0 signed; 1 unsigned */
#ifdef DEBUG
	if ( DEBUG('x') > 1 )
		(void)fprintf(stderr, "reduce() called on: %13s <-> %-13s\n",
			tk_saycode(ostp->code), tk_saycode(input_op));
#endif
	switch (TK_ENUMNO(ostp->code))
	{
	CASE(C_Colon) 
		/* t1 ? t2 : t3;
		** Form tree that looks like
		** 	?
		**     / \
		**    t1  :
		**	 / \
		**      t2  t3
		*/
		t = ostp->next;			/* ? */
		t->left = nstp->next->next;	/* t1 */
		t->right = ostp;		/* :  */
		t->right->left = nstp->next;	/* t2 */
		t->right->right = nstp;		/* t3 */
		t->aux.type = t->right->aux.type = 
			unsgnd (t->right) ? T_unsigned : T_signed;
		ostp = ostp->next->next;
		t->next = nstp->next->next->next; 
		nstp->next = nstp->next->next = nstp->next->next->next = 0;
		nstp = t;
		break;

	CASE(C_LogicalOR) 
	CASE(C_LogicalAND) 
	CASE(C_Equal) 
	CASE(C_NotEqual) 
	CASE(C_GreaterThan) 
	CASE(C_GreaterEqual) 
	CASE(C_LessThan) 
	CASE(C_LessEqual) 
		isunsigned = 0;
		/*FALLTHROUGH*/
	CASE(C_InclusiveOR) 
	CASE(C_ExclusiveOR) 
	CASE(C_BitwiseAND) 
	CASE(C_LeftShift) 
	CASE(C_RightShift) 
	CASE(C_Plus) 
	CASE(C_Minus) 
	CASE(C_Mult) 
	CASE(C_Div) 
	CASE(C_Mod) 

		/* Binary Operators */

		t = ostp;
		ostp = ostp->next;
		t->left = nstp->next;
		t->right = nstp;
		if (isunsigned == -1)
			isunsigned = unsgnd(t) == B_true;
		t->aux.type = isunsigned ? T_unsigned : T_signed;
		t->next = nstp->next->next;
		nstp->next = nstp->next->next = 0;
		nstp = t;
		break;

	CASE(C_UnaryPlus) 
		
		t = ostp;
		ostp = t->next;
		freetree(t);
		break;

	CASE(C_UnaryMinus) 
	CASE(C_Complement) 
	CASE(C_Not) 
		t = ostp;
		ostp = t->next;
		t->left = nstp;
		t->next = nstp->next;
		nstp->next = 0;
		nstp = t;
		t->aux.type = t->left->aux.type;
		break;

	CASE(C_LParen)
		t = ostp;
		ostp = t->next->next;
		freetree(t->next);
		freetree(t);
		break;
#ifdef DEBUG
	default:
		fprintf(stderr, "Internal error: illegal code in reduce()\n");
		break;
#endif
	}
	expect = E_operator;
}

static Action
reducechk()
/* At this point, action() has decided the automaton should reduce.
** This routine, extending the error checking in action(),
** looks for syntax errors in the use of the paired operators:
** '(',')' and '?',':'. Some syntax errors are manifested in attempts
** to reduce on mismatched operator pairs.
** This routine will return A_error upon finding a syntax violation,
** else it will return A_reduce.
*/
{
	switch (ostp->code)
	{
	case C_LParen:
		if ( ostp->next && ostp->next->code == C_RParen )
			break;
		/*FALLTHRU*/
	case C_RParen:
		UERROR(gettxt(":536","mismatched parentheses"));
		return A_error;

	case C_Colon:
		if ( ostp->next && ostp->next->code == C_Question )
			break;
		/*FALLTHRU*/
	case C_Question:
		UERROR(gettxt(":537","mismatched \"?\" and \":\""));
		return A_error;
	}
	return A_reduce;
}

static	void  
shift()
/* This routine pushes the Tree representing the input Token on the correct 
** stack (either the number stack or the operator stack)
** and makes sure the next token is pointed to.
** Keeps track whether the next token is expected to be an operand.
*/
/* for now - expain `(' vs. `(' */
{
	Token *next = curtoken->next;
	DBGCALL(1,shift ,curtoken);
	switch ( input_op )
	{
	case C_Operand:
		primary();
		curtree->next = nstp;
		nstp = curtree;
		expect = E_operator;
		break;

	case C_LParen:
		curtree->next = ostp;
		ostp = curtree;
		ostp->code = C_RParen;
		expect = E_number;
		break;

	case C_RParen:
		curtree->next = ostp;
		ostp = curtree;
		ostp->code= C_LParen;
		expect = E_operator;
		break;

	default:
		curtree->next = ostp;
		ostp = curtree;
		ostp->code= (unsigned short) input_op;
		expect = E_number;
	}
	curtree->left = curtree->right = 0;
	curtoken = next;
}

static Boolean
unsgnd(t)
Tree *t;
/* Returns "B_true" if either of the operands of a binary
** operator is unsigned.
*/
{
    	if (t->left->aux.type == T_unsigned || 
            t->right->aux.type == T_unsigned)
		return B_true;
	else
		return B_false;
}

int
xp_value(tp)
	Token * tp;
/* Given a pointer to a Token list that comprises a
** pre-macro expansion constant expression
** in a conditional inclusion directive,
** This routine initializes and runs the automaton
** to evaluate the constant expression.
** It return false if expression evaluates to 0, else it returns true.
** It returns true on a syntax error (C Issue 4.1 compatible behavior).
*/
{
	long retval;	/* return value */

	/* The following check insures that Tree and Token structures
	** are compatible.  If they are, no code should be generated
	** for the following check.  If they aren't, we should execute this 
	** once and then exit.
	*/
#ifndef __STDC__
#define offsetof(s, m)	((unsigned int)(&(((s *)0)->m)))
#endif
	/*CONSTANTCONDITION*/
	if(	(offsetof(Token,next)	!=	offsetof(Tree,next))
	    ||	(offsetof(Token,code)	!=	offsetof(Tree,code))
	    ||	(offsetof(Token,aux)	!=	offsetof(Tree,aux))
	    ||	(offsetof(Token,number)	!=	offsetof(Tree,number))
	    ||	(sizeof(Token)		<	sizeof(Tree))
	)
		pp_internal("Tree structure incompatible with Token structure");

	DBGCALLL(1,xp_value,tp);
	COMMENT(tp != 0);
	COMMENT(nstp == 0);
	COMMENT(ostp == 0);
	if ((curtoken = tk_rmws(ex_condexpr(tp))) == 0)
	{
		UERROR(gettxt(":538", "empty constant expression after macro expansion" ));
		DBGRETN(0,xp_value,B_true); 
		return B_true;
	}
	for(expect = E_number; ; )
	{
		switch (action())
		{
		case A_shift:
			curtree = (Tree *) curtoken;
			shift();
			curtoken = tk_rmws(curtoken);
			continue;

		case A_reduce:
			reduce();
			continue;

		case A_accept:
			COMMENT(curtoken == 0);
			COMMENT(ostp == 0);
			COMMENT(nstp != 0 && nstp->next == 0);
			eval(nstp);
			retval = nstp->number;
			DBGRET(0,xp_value,nstp);
			freetree(nstp);
			nstp = 0;
		    	return retval;

		case A_error:
			if (curtoken != 0)	tk_rml(curtoken);
			while (nstp != 0) {
				register Tree *t = nstp->next;
				freetree(nstp);
				nstp = t;
			}
			while (ostp != 0) {
				register Tree *t = ostp->next;
				freetree(ostp);
				ostp = t;
			}
			DBGRETN(0,xp_value,B_true); 
			return B_true; 
		}
	}
}
