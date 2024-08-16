/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */
#ident	"@(#)debugger:libexp/common/CClalex.C	1.6"

#include "CC.h"
#include "CCtokdefs.h"
#include "Interface.h"

// History: The lalex routines came from the C++ compiler.
//          Though it has undergone much revision, the bulk of
//          the logic and most names from the original code have
//          been kept.

int CCpos;

struct toknode {
	int         tok;	/* token for parser */
	YYSTYPE     retval;
	int         place;
	toknode    *next;
	toknode    *last;

	toknode(int, YYSTYPE, int&);
	~toknode() {}
};

static toknode *front = 0;
static toknode *rear  = 0;
static toknode *latok;		// current lookahead token

static void addtok(int, YYSTYPE, int&);  /* add tok to rear of Q     */
static int  deltok();			  /* take tok from front of Q */


void
CCreset_lalex()
{
    while (front != 0) {
	toknode *next = front->next;
	delete front;
	front = next;
    }
    rear  = 0;
    latok = 0;
}

static void
add_tokens()
/*
    extend lookahead token queue when depleted
*/
{
int pos;

    int tk = CCtlex(pos);
    addtok(tk, CCtretval, pos);
    if ( tk != ID )
	return;

    while (tk == ID || tk == MEM || tk == DOT ) {
	// shouldn't use DOT as a scope operator
	tk = CCtlex(pos);
	addtok(tk, CCtretval, pos);
    }
}


int
CCla_look()
/*
    sneak a peek at head of token queue
*/
{
    if ( front == 0 )
	add_tokens();

    latok = front;
    return latok->tok;
}

static int
lookahead()
/*
    advance lookahead pointer, lexing at end of Q
    handle occurrences of TSCOPE
    (should be kept up to date with lalex())
*/
{
    int tk;

    if ( latok == rear ) {
	add_tokens();
	if ( latok )
	    latok = latok->next;
	else
	    latok = front;
    }
    else
	latok = latok->next;

    tk = latok->tok;

    if ( tk == ID && latok->next->tok == MEM ) {      // can't be empty
	// ID :: => TSCOPE
	latok = latok->next;
	tk    = TSCOPE;
    }

    return tk;
}

int
CClalex()
/*
    return next token to grammar
*/
{
    register int tk;

    if ( front == 0 )
	add_tokens();		// extend lookahead queue

    tk = deltok();

    if ( tk == ID )
    {
	switch( CCla_look() )
	{
	case MEM:
	{
      		// ID :: => TSCOPE
      		YYSTYPE saveval = CC_yylval; // save CC_yylval and CCpos
      		int    savepos = CCpos;
      		deltok();
      		CC_yylval = saveval;	// restore token value after removing ::
      		CCpos     = savepos;	//    from the look ahead queue.
      		tk = TSCOPE;
		break;
	}
	case DOT:
		if( lookahead()==ID && lookahead()==AT_SIGN )
		{
			tk = FN_SUFFIX;
		}
		break;
	}
    }

    return tk;
}

void
CCla_backup( int t, YYSTYPE r, int& pos )
/*
    called by parser to push token back on front of queue
*/
{
    register toknode* T = new toknode(t, r, pos);
    if (front) {
	front->last = T;
	T->next = front;
	T->last = 0;
	front = T;
    } else
	front = rear = T;
}

int
CCla_bracket()
//
// NOTE: This routine is new.
//       It is called in reduction of term_lp to determine when an
//       ID followed by a left bracket is actually a TNAME.
{
    int tk = latok->tok;
    for (;;) {
	switch (tk) {
	case LB:
	    if (lookahead() == RB)
		// ( T [] )
		return 1;
	    else {
		while (lookahead() != RB);
		tk = lookahead();
		continue;
	    }
	default:
	    return 0;
	}
    }
}

int
CCla_cast()
//
// called in reduction of term_lp to check for ambiguous prefix-style cast
// if result is 1, caller inserts DECL_MARKER to force reduction of cast.
{
	// yychar already designates TYPE or ID
	// LP must start the lookahead queue!
	//
	// NOTE: This is now also called when the
	//      lookahead queue contains MUL or AND.
	int tk, tk2 = latok->tok;

	for (;;) {
	    tk = tk2;
	    tk2 = lookahead();

	    switch (tk) {
	    case LP:
		if ( tk2 == MUL || tk2 == AND || tk2 == TSCOPE )
		    // T ( * ...
		    continue;
		else
		    // T ( exp )
		    return 0;
	    case MUL: case AND:
		if ( tk2 == SCTYPE )
		    // T ( * const ...
		    // T ( * volatile ...
		    tk2 = lookahead();
		continue;
	    case TSCOPE:
		if ( tk2 == MUL )
		    // T ( C :: * ...
		    continue;
		else
		    // T ( exp )
		    return 0;
	    case RP: case LB:
		// T (*)()
		// T (*[])()
		return 1;
	    }

	    return 0;
	}
}

//----------------------- token queue support --------------------------

toknode::toknode(int t, YYSTYPE r, int& pos) : place(pos)
{
    tok = t;
    retval = r;
    next = last = 0;
}

static void
addtok(int t, YYSTYPE r, int& pos)
{
	toknode* T = new toknode(t, r, pos);
	if (front == 0)
		front = rear = T;
	else {
		rear->next = T;
		T->last = rear;
		rear = T;
	}
}


static int
deltok()
{
	register toknode* T = front;
	register int tk = T->tok;

	CC_yylval = T->retval;
	CCpos     = T->place;

	if (front = front->next) {
	    front->last = 0;
	} else {
	    latok = rear = 0;
	}
	delete T;

	return tk;
}
