/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/src/regcomp.c	1.4.3.1"
#ident "@(#)regcomp.c	1.7 'attmail mail(1) command'"
#include "libmail.h"
#include "smtp.h"
#include "regprog.h"

extern void regerror proto((char *));

/*
 * Parser Information
 */
typedef struct Node {
	Inst	*first;
	Inst	*last;
} Node;
#define	NSTACK	20
static Node	andstack[NSTACK];
static Node	*andp;
static int	atorstack[NSTACK];
static int	*atorp;
static int	cursubid;		/* id of current subexpression */
static int	subidstack[NSTACK];	/* parallel to atorstack */
static int	*subidp;
static int	lastwasand;	/* Last token was operand */
static int	nbra;
static char	*exprp;		/* pointer to next character in source expression */
static int	nclass;
static Class	*classp;
static Inst	*freep;
static int	errors;

/* predeclared functions */
static void operator proto((int));
static void pushand proto((Inst*, Inst*));
static void pushator proto((int));
static void evaluntil proto((int));
static void bldcclass proto((void));

static void
rcerror(s)
	char *s;
{
	errors++;
	regerror(s);
}

static Inst *
newinst(t)
	int t;
{
	freep->type=t;
	freep->left=0;
	freep->right=0;
	return freep++;
}

static void
operand(t)
	int t;
{
	register Inst *i;
	if(lastwasand)
		operator(CAT);	/* catenate is implicit */
	i=newinst(t);
	if(t==CCLASS)	/* ugh */
		i->right=(Inst *)&(classp[nclass-1]);	/* UGH! */
	pushand(i, i);
	lastwasand=TRUE;
}

static void
operator(t)
	int t;
{
	if(t==RBRA && --nbra<0)
		rcerror(":63:regerror: unmatched right paren\n");
	if(t==LBRA) {
		if (++cursubid >= NSUBEXP)
			rcerror(":64:regerror: too many subexpressions\n");
		nbra++;
		if (lastwasand)
			operator(CAT);
	} else
		evaluntil(t);
	if(t!=RBRA)
		pushator(t);
	lastwasand=FALSE;
	if(t==STAR || t==QUEST || t==PLUS || t==RBRA)
		lastwasand=TRUE;	/* these look like operands */
}

#ifndef SVR4_1
static void
regerr2(s, c)
	char *s;
{
	char buf[100];
	char *cp = buf;
	while(*s)
		*cp++ = *s++;
	*cp++ = c;
	*cp = '\0'; 
	rcerror(buf);
}
#endif

static void
pushand(f, l)
	Inst *f, *l;
{
	if(andp >= &andstack[NSTACK])
		rcerror(":65:regerror: can't happen: operand stack overflow\n");
	andp->first=f;
	andp->last=l;
	andp++;
}

static void
pushator(t)
	int t;
{
	if(atorp >= &atorstack[NSTACK])
		rcerror(":66:regerror: can't happen: operator stack overflow\n");
	*atorp++=t;
	*subidp++=cursubid;
}

static Node *
popand(op)
{
	register Inst *inst;

	if(andp <= &andstack[0]) {
#ifdef SVR4_1
		rcerror(":71:regerror: missing operand\n");
#else
		regerr2("missing operand for ", op);
#endif
		inst=newinst(NOP);
		pushand(inst,inst);
	}
	return --andp;
}

static int
popator()
{
	if(atorp <= &atorstack[0])
		rcerror(":67:regerror: can't happen: operator stack underflow\n");
	--subidp;
	return *--atorp;
}

static void
evaluntil(pri)
	register pri;
{
	register Node *op1, *op2;
	register Inst *inst1, *inst2;

	while(pri==RBRA || atorp[-1]>=pri){
		switch(popator()){
		default:
			rcerror(":68:regerror: unknown operator in evaluntil\n");
			break;
		case LBRA:		/* must have been RBRA */
			op1=popand('(');
			inst2=newinst(RBRA);
			inst2->subid = *subidp;
			op1->last->next = inst2;
			inst1=newinst(LBRA);
			inst1->subid = *subidp;
			inst1->next=op1->first;
			pushand(inst1, inst2);
			return;
		case OR:
			op2=popand('|');
			op1=popand('|');
			inst2=newinst(NOP);
			op2->last->next=inst2;
			op1->last->next=inst2;
			inst1=newinst(OR);
			inst1->right=op1->first;
			inst1->left=op2->first;
			pushand(inst1, inst2);
			break;
		case CAT:
			op2=popand(0);
			op1=popand(0);
			op1->last->next=op2->first;
			pushand(op1->first, op2->last);
			break;
		case STAR:
			op2=popand('*');
			inst1=newinst(OR);
			op2->last->next=inst1;
			inst1->right=op2->first;
			pushand(inst1, inst1);
			break;
		case PLUS:
			op2=popand('+');
			inst1=newinst(OR);
			op2->last->next=inst1;
			inst1->right=op2->first;
			pushand(op2->first, inst1);
			break;
		case QUEST:
			op2=popand('?');
			inst1=newinst(OR);
			inst2=newinst(NOP);
			inst1->left=inst2;
			inst1->right=op2->first;
			op2->last->next=inst2;
			pushand(inst1, inst2);
			break;
		}
	}
}

static void
optimize(pp)
	regexp *pp;
{
	register Inst *inst, *target;

	for(inst=pp->firstinst; inst->type!=END; inst++){
		target=inst->next;
		while(target->type == NOP)
			target=target->next;
		inst->next=target;
	}
}

#ifdef	DEBUG
static void
dumpstack(){
	Node *stk;
	int *ip;

	printf("operators\n");
	for(ip=atorstack; ip<atorp; ip++)
		printf("0%o\n", *ip);
	printf("operands\n");
	for(stk=andstack; stk<andp; stk++)
		printf("0%o\t0%o\n", stk->first->type, stk->last->type);
}

static void
dump(pp)
	regexp *pp;
{
	Inst *l;

	l=pp->firstinst;
	do{
		printf("%d:\t0%o\t%d\t%d\n", l-pp->firstinst, l->type,
			l->left-pp->firstinst, l->right-pp->firstinst);
	}while(l++->type);
}
#endif

static void
startlex(s)
	char *s;
{
	exprp=s;
	nclass=0;
	nbra=0;
}

static Class *
newclass(){
	register Class *p;
	register n;

	if(nclass >= NCLASS)
#ifdef SVR4_1
		rcerror(":72:regerror: too many character classes\n");
#else
		regerr2("too many character classes; limit", NCLASS+'0');
#endif
	p = &(classp[nclass++]);
	for(n=0; n<16; n++)
		p->map[n]=0;
	return p;
}

static int
lex(){
	register c= *exprp++;

	switch(c){
	case '\\':
		if(*exprp)
			c= *exprp++;
		break;
	case 0:
		c=END;
		--exprp;	/* In case we come here again */
		break;
	case '*':
		c=STAR;
		break;
	case '?':
		c=QUEST;
		break;
	case '+':
		c=PLUS;
		break;
	case '|':
		c=OR;
		break;
	case '.':
		c=ANY;
		break;
	case '(':
		c=LBRA;
		break;
	case ')':
		c=RBRA;
		break;
	case '^':
		c=BOL;
		break;
	case '$':
		c=EOL;
		break;
	case '[':
		c=CCLASS;
		bldcclass();
		break;
	}
	return c;
}

static int
nextc(){
	if(exprp[0]==0 || (exprp[0]=='\\' && exprp[1]==0))
		rcerror(":69:regerror: malformed '[]'\n");
	if(exprp[0]=='\\'){
		exprp++;
		return *exprp++|0200;
	}
	return *exprp++;
}

static void
bldcclass(){
	register c1, c2;
	register Class *classp;
	register negate=FALSE;

	classp=newclass();
	/* we have already seen the '[' */
	if(*exprp=='^'){
		negate=TRUE;
		exprp++;
	}
	while((c1=c2=nextc()) != ']'){
		if(*exprp=='-'){
			exprp++;	/* eat '-' */
			if((c2=nextc()) == ']')
				rcerror(":69:regerror: malformed '[]'\n");
		}
		for((c1&=0177), (c2&=0177); c1<=c2; c1++)
			classp->map[c1/8] |= 1<<(c1&07);
	}
	if(negate)
		for(c1=0; c1<16; c1++)
			classp->map[c1]^=0377;
	classp->map[0] &= 0376;		/* exclude NUL */
}

extern regexp *
regcomp(s)
	char *s;
{
	register token;
	regexp *pp;

	/* get memory for the program */
	pp = (regexp *)malloc(sizeof(regexp) + 3*sizeof(Inst)*strlen(s));
	if (pp == NULL) {
		rcerror(":70:regerror: out of memory\n");
		return NULL;
	}
	freep = pp->firstinst;
	classp = pp->class;
	errors = 0;

	/* go compile the sucker */
	startlex(s);
	atorp=atorstack;
	andp=andstack;
	subidp=subidstack;
	lastwasand=FALSE;
	cursubid=0;

	/* Start with a low priority operator to prime parser */
	pushator(START-1);
	while((token=lex()) != END){
		if((token&0300) == OPERATOR)
			operator(token);
		else
			operand(token);
	}

	/* Close with a low priority operator */
	evaluntil(START);

	/* Force END */
	operand(END);
	evaluntil(START);
#ifdef DEBUG
	dumpstack();
#endif
	if(nbra)
		rcerror(":77:regerror: unmatched left paren\n");
	--andp;	/* points to first and only operand */
	pp->startinst=andp->first;
#ifdef DEBUG
	dump(pp);
#endif
	optimize(pp);
#ifdef DEBUG
	printf("start: %d\n", andp->first-pp->firstinst);
	dump(pp);
#endif
	if (errors) {
		free(pp);
		pp = NULL;
	}
	return pp;
}
