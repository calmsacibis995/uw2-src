/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:publik/Lexer.h	3.1" */
/******************************************************************************
*
* C++ Standard Components, Release 3.0.
*
* Copyright (c) 1991, 1992 AT&T and Unix System Laboratories, Inc.
* Copyright (c) 1988, 1989, 1990 AT&T.  All Rights Reserved.
*
* THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T and Unix System
* Laboratories, Inc.  The copyright notice above does not evidence
* any actual or intended publication of such source code.
*
******************************************************************************/

#include "Token.h"

#include <Objection.h>
#include <List.h>
#include <iostream.h>

typedef Token *TokenP;
//List_of_pdeclare(Token)  /* no more, using SC release 3.0 */
typedef void (*ContractAction)(Token *);

class Lexer 
{
public:
	static Objection DestroyingFrozenLexer, ContractingFrozenLexer, DiscardedToken, BadHandshake;

	enum { infinity = -1 };

	// Create a lexer with the given trail size (default = 0).  
	Lexer(int trail=0): 
		thetrailsize(trail), contractAction(0), lalineno(1), 
		la(0), inf(0), curTok(0), curToki(-1), frozen(0), verboselevel(0)
					{}
	~Lexer() 			{ if (frozen) DestroyingFrozenLexer.raise("Attempt to destroy a frozen lexer!"); }

	// Attach the lexer to an input character stream.
	void attach(istream *f, const char *nam)
				 	{ thefilename = nam; inf = f; la = 0; in(); move(); }

	// Returns true if there are no tokens in the current window.
	// (True before the lexer is attached to a file.)
	int emptyWindow() const		{ return curToki == -1; }

	// Access the name of the input file.
	void setFilename(const char *nam)	{ thefilename = nam; }
	const char *filename() const		{ return thefilename; }

	// Access the trail size.
	void setTrailsize(int l)	{ thetrailsize = l; }
	int trailsize() const		{ return thetrailsize; }

	// Move the current token pointer i tokens to the right (left if i is negative), 
	// extending and contracting the window as necessary.
	void move(int i=1)	 	{ if (i != 0 && resize(i)) {
					  	curToki += i; 
					  	curTok = theWindow[curToki]; 
					  	if (thetrailsize != infinity) implicitContract(curToki - thetrailsize);
					  	if (verboselevel == 1) cerr << *curTok << endl;
					}}

	// Same as move, but i is relative to beginning of window.
	void moveAbs(int i)	 	{ move(i-curToki); }

	// Add i tokens to the right of the window (but don't move the current token).
	//void extend(int i=1)		{ while (i-- > 0) { 
	int extend(int i=1)		{ while (i-- > 0) { 
						Token *t = gettok();
						theWindow.put(t); 
						if (verboselevel > 1) cerr << *t << endl;
					} return 0;}

	// Remove i tokens from the left of the window, or as many as possible up to the current token.
	// Return the number actually removed.
	int contract(int i=1)		{ return docontract(i, contractAction, 1); }

	// Remove enough tokens from the left of the window so that there are no more than i to the left of the current token.
	int contractTo(int i)		{ return docontract(curToki - i, contractAction, 1); }

	// Same as above two, but don't do contractAction.
	int discard(int i=1)		{ return docontract(i, 0, 1); }
	int discardTo(int i)		{ return docontract(curToki - i, 0, 1); }

	// Disable all contracting, both explicit and automatic.
	void freeze()			{ frozen = 2; }

	// Disable automatic contracting.
	void partialFreeze()		{ frozen = 1; }

	// Enable all contracting.
	void melt()			{ frozen = 0; }

	// 0 is quiet, 1 is "show each token as it becomes current," >1 is "show each token as it enters window."
	void verbose(int i)		{ verboselevel = i; }

	// Set the contract action.  Return the old value.
	ContractAction setContractAction(ContractAction ca)
					{ ContractAction cur = contractAction; 
					  contractAction = ca;
					  return cur; 
					}

	// Get the current contract action.
	ContractAction getContractAction() const
					{ return contractAction; }

	// Return the current size of the window.
 	int windowSize() const		{ return theWindow.length(); }

	// Return the index in the window of the current token.
	int curpos() const		{ return curToki; }

	// Functions for retreiving information from the window.  
	// Except for the *Abs functions, i is offset relative to the current token.
	//
 	// The first function returns a pointer to the *base part* of the i'th token.
	// This function is provided as a courtesy to the client, so she doesn't have to do
	// an inordinate amount of checking on the types of tokens.
	//
	Token *window(int i=0)		{ if (i == 0) return curTok;
					  else if (resize(i)) return theWindow[curToki+i]; 
					  else return 0;
					}

	Token *windowAbs(int i)		{ return window(i-curToki); }
	//
	// The rest of the functions are used to get tokens of particular types from the window.  
	// Since they are specific to the particular language being lexed, they must be defined in the derived class.
	// They shouldn't move the current token.

	// Line number access.
	void setLineno(int l)		{ lalineno = l; }
	int lineno() const		{ return lalineno; }

protected:
// methods:
	int implicitContract(int i)	{ return docontract(i, contractAction, 0); }

	// The implementation of contract, contractTo, discard, discardTo, and implicitContract.  
	// Returns the number of tokens actually removed.
	int docontract(int i, ContractAction ca, int explicit);  // defined in lexer.c.

	// The user-supplied gettok.  It should return a pointer to the next Token in the input stream.
	virtual Token *gettok() = 0;

	// Make i a valid offset from the current token.
	int resize(int i)		{ extend(curToki+i-theWindow.length()+1);
					  if (curToki+i < 0) {
						DiscardedToken.raise("Attempt to go to or get a discarded token!");
						return 0;
					  }
					  else  return 1;
					}

	// Make sure the i'th token has type type.
	// Does *not* do a resize(i).
	int handshake(int type, int i)	{ if (((Token *)(theWindow[curToki+i]))->type != type) {
						BadHandshake.raise("Bad handshake with Lexer!");
						return 0;
					  }
					  else  return 1;
					}

	// Get the next character from the input stream and put it in la, and also return it.  
	char in()			{ if (la == '\n') 
						lalineno++; 
					  if (inf == 0 || inf->eof() || !inf->get(la))
						la = 0;
					  return la;		
					}

	// Return what the next call to in() will return.
	char peek()			{ if (inf == 0 || inf->eof()) 
						return 0; 
					  else 
						return inf->peek(); 
					}
protected:
// data:
	// See above.
	int verboselevel;

	// 1 if automatic contracting is disabled, 2 if all contracting is disabled, 0 otherwise.
	int frozen;

	// User-specified action to perform on token when contracting it out of window
	// due to exceeding trail size, or explicit request from client.  
	ContractAction contractAction;

	// Maximum allowed number of tokens in window to the left of the current token.
	int thetrailsize;

	// Input character *following* the current token.
	char la;

	// The input file.
	istream *inf;

	// Name of input stream, and line number of la.
	const char *thefilename;
	int lalineno;

	// The token window.  curTok points to the current token, and curToki is its index in the window.
	List_of_p<Token> theWindow;
	Token *curTok;
	int curToki;
};


#define TOK	lexer->window()->type
#define PREVTOK	lexer->window(-1)->type
#define NEXTTOK	lexer->window(1)->type
#define LEXEME	lexer->window()->lexeme
#define LINENO	lexer->window()->lineno
#define WS	lexer->window()->ws
#define ADV	lexer->move()


#define DEFINE_GET(typename, typecode)		\
	void get(typename &x, int i=0)		\
		{ if (resize(i) && handshake(typecode, i)) 	\
			x = *(typename*)window(i); 		\
		}	
