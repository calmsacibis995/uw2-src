/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)ksh:sh/word.c	1.5.6.4"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/ksh/sh/word.c,v 1.1 91/02/28 17:41:35 ccs Exp $"

/*
 * UNIX shell
 *
 * S. R. Bourne
 * Rewritten by David Korn
 * AT&T Bell Laboratories
 *
 */

#include	"defs.h"
#include	"sym.h"
#include	"builtins.h"
#ifdef	NEWTEST
#   include	"test.h"
#endif	/* NEWTEST */



static void setupalias();
static int here_copy();
static int here_tmp();
static int qtrim();

/* This module defines the following routines */
void	match_paren();

/* This module references these external routines */
extern char	*sh_tilde();

/* ========	character handling for command lines	========*/

/*
 * Get the next word and put it on the top of the stak
 * Determine the type of word and set sh.wdnum and sh.wdset accordingly
 * Returns the token type
 */

sh_lex()
{
	register int c;
	register int d;
	register char *argp;
	register int tildp;
	int offset;
	char chk_keywd;
	int 	alpha = 0;
	sh.wdnum=0;
	sh.wdval = 0;
	/* condition needed to check for keywords, name=value */
	chk_keywd = (sh.reserv!=0 && !(sh.wdset&IN_CASE)) || (sh.wdset&KEYFLG);
	sh.wdset &= ~KEYFLG;
	sh.wdarg = (struct argnod*)stakseek(ARGVAL);
	sh.wdarg->argnxt.ap = 0;
	offset = staktell();
	tildp = -1;
	while(1)
	{
		while((c=io_nextc(), isblank(c)));
		if(c==COMCHAR)
		{
			while((c=io_readc()) != NL && c != ENDOF);
			io_unreadc(c);
		}
		else	 /* out of comment - white space loop */
			break;
	}
	if(c=='~')
		tildp = offset;
	if(!ismeta(c))
	{
		do
		{
			if(c==LITERAL)
			{
				match_paren(c,c);
				alpha = -1;
			}
			else
			{
				if(staktell()==offset && chk_keywd && isalpha(c))
					alpha++;
				stakputc(c);
				if(c == ESCAPE)
					stakputc(io_readc());
				if(alpha>0)
				{
					if(c == '[')
						match_paren('[',']');
					else if(c=='=')
					{
						sh.wdset |= KEYFLG;
						tildp = staktell();
						alpha = 0;
					}
					else if(!isalnum(c))
						alpha = 0;
				}
				if(qotchar(c))
					match_paren(c,c);
			}
			d = c;
			c = io_nextc();
			if(d==DOLLAR && c==LBRACE)
			{
				stakputc(c);
				match_paren(LBRACE, RBRACE);
				c = io_nextc();
			}
			else if(c==LPAREN && patchar(d))
			{
				stakputc(c);
				match_paren(LPAREN, RPAREN);
				c = io_nextc();
			}
			else if(tildp>=0 &&  (c == '/'  || c==':' || ismeta(c)))
			{
				/* check for tilde expansion */
				stakputc(0);
				argp=sh_tilde(stakptr(tildp));
				if(argp)
				{
					stakset(stakptr(0),tildp);
					stakputs(argp);
				}
				else
					stakset(stakptr(0),staktell()-1);
				tildp = -1;
			}
			/* tilde substitution after : in variable assignment */
			/* left in as unadvertised compatibility feature */
			if(c==':' && (sh.wdset&KEYFLG))
				tildp = staktell()+1;
		}
		while(!ismeta(c));
		sh.wdarg = (struct argnod*)stakfreeze(1);
		argp = sh.wdarg->argval;
		io_unreadc(c);
#ifdef	NEWTEST
		if(sh.wdset&IN_TEST)
		{
			if(sh.wdset&TEST_OP1)
			{
				if(argp[0]=='-' && argp[2]==0 &&
					strchr(test_unops,argp[1]))
				{
					sh.wdnum = argp[1];
					sh.wdval = TESTUNOP;
				}
				else if(argp[0]=='!' && argp[1]==0)
				{
					sh.wdval = '!';
				}
				else
					sh.wdval = 0;
				sh.wdset &= ~TEST_OP1;
				return(sh.wdval);
			}
			c = sh_lookup(argp, test_optable);
			switch(c)
			{
			case TEST_END:
				return(sh.wdval=ETSTSYM);

			default:
				if(sh.wdset&TEST_OP2)
				{
					sh.wdset &= ~TEST_OP2;
					sh.wdnum = c;
					return(sh.wdval=TESTBINOP);	
				}

			case TEST_OR: case TEST_AND:
			case 0:
				return(sh.wdval = 0);
			}
		}
#endif	/*NEWTEST */
		if(argp[1]==0 &&
			(d=argp[0],isdigit(d)) &&
			(c=='>' || c=='<'))
		{
			sh_lex();
			sh.wdnum |= (IODIGFD|(d-'0'));
		}
		else
		{
			/*check for reserved words and aliases */
			sh.wdval = (sh.reserv!=0?sh_lookup(argp,tab_reserved):0);
			/* for unity database software, allow select to be aliased */
			if((sh.reserv!=0 && (sh.wdval==0||sh.wdval==SELSYM)) || (sh.wdset&CAN_ALIAS))
			{
				/* check for aliases */
				struct namnod* np;
				if((sh.wdset&(IN_CASE|KEYFLG))==0 &&
					(np=nam_search(argp,sh.alias_tree,N_NOSCOPE))
					&& !nam_istype(np,M_FLAG)
					&& (argp=nam_strval(np)))
				{
					setupalias(argp,np);
					st.peekn = 0;
					nam_ontype(np,M_FLAG);
					sh.wdset |= KEYFLG;
					return(sh_lex());
				}
			}
		}
	}
	else if(dipchar(c))
	{
		sh.wdval = c;
		d = io_nextc();
		if(d==c)
		{
			sh.wdval = c|SYMREP;
			if(c=='<')
			{
				if((d=io_nextc())=='-')
					sh.wdnum |= IOSTRIP;
				else
					io_unreadc(d);
			}
			/* arithmetic evaluation ((expr)) */
			else if(c == LPAREN && sh.reserv != 0)
			{
				stakputc(DQUOTE);
				match_paren(LPAREN, RPAREN);
				*stakptr(staktell()-1) = DQUOTE;
				c = io_nextc();
				if(c != ')')
				{
					/*
					 * process as nested () command
					 * for backward compatibility
					 */
					stakputc(')');
					stakputc(c);
					sh.wdarg = (struct argnod*)stakfreeze(1);
					qtrim(argp = sh.wdarg->argval);
					setupalias(argp,(struct namnod*)0);
					sh.wdval = st.peekn = '(';
				}
				else
				{
					sh.wdarg= (struct argnod*)stakfreeze(1);
					return(EXPRSYM);
				}
			}
		}
		else if(c=='|')
		{
			if(d=='&')
				sh.wdval = COOPSYM;
			else
				io_unreadc(d);
		}
#ifdef DEVFD
		else if(d==LPAREN && iochar(c))
			sh.wdval = (c=='>'?OPROC:IPROC);
#endif	/* DEVFD */
		else if(c==';' && d=='&')
			sh.wdval = ECASYM;
		else
			io_unreadc(d);
	}
	else
	{
		if((sh.wdval=c)==ENDOF)
		{
			sh.wdval=EOFSYM;
			if(st.standin->ftype==F_ISALIAS)
				io_pop(1);
		}
		if(st.iopend && eolchar(c))
		{
			if(sh.owdval || is_option(NOEXEC))
				c = getlineno(1);
			if(here_copy(st.iopend)<=0 && sh.owdval)
			{
				sh.owdval = ('<'|SYMREP);
				sh.wdval = EOFSYM;
				sh.olineno = c;
				sh_syntax();
			}
			st.iopend=0;
		}
	}
	sh.reserv=0;
	return(sh.wdval);
}

static void setupalias(string,np)
char *string;
struct namnod *np;
{
	register struct fileblk *f;
	register int line;
	f = new_of(struct fileblk,0);
	line = st.standin->flin;
	io_push(f);
	io_sopen(string);
	f->flin = line-1;
	f->ftype = F_ISALIAS;
	f->feval = (char**)np;
	/* add trailing new-line if needed to avoid recursion */
	if((f->flast=st.peekn)==MARK && np)
		f->flast = '\n';
}

/*
 * read until matching <close>
 */

void match_paren(open,close)
register int open;
{
	register int c;
	register int count = 1;
	register int quoted = 0;
	int was_dollar=0;
	int line = st.standin->flin;
	if(open==LITERAL)
		stakputc(DQUOTE);
	while(count)
	{
		/* check for unmatched <open> */
		if(quoted || open==LITERAL)
			c = io_readc();
		else
			c = io_nextc();
		if(c==0)
		{
			/* eof before matching quote */
			/* This keeps old shell scripts running */
			if(filenum(st.standin)!=F_STRING || is_option(NOEXEC))
			{
				sh.olineno = line;
				sh.owdval = open;
				sh.wdval = EOFSYM;
				sh_syntax();
			}	
			io_unreadc(0);
			c = close;
		}
		if(c == NL)
		{
			if(open=='[')
			{
				io_unreadc(c);
				break;
			}
			sh_prompt(0);
		}
		else if(c == close)
		{
			if(!quoted)
				count--;
		}
		else if(c == open && !quoted)
			count++;
		if(open==LITERAL && (escchar(c) || c=='"'))
			stakputc(ESCAPE);
		stakputc(c);
		if(open==LITERAL)
			continue;
		if(!quoted)
		{
			switch(c)
			{
				case '<':
				case '>':
					if(open==LBRACE)
					{
						/* reserved for future use */
						sh.wdval = c;
						sh_syntax();
					}
					break;
				case LITERAL:
				case '"':
				case '`':
					/* check for nested '', "", and `` */
					if(open==close)
						break;
					if(c==LITERAL)
						stakset(stakptr(0),staktell()-1);
					match_paren(c,c);
					break;
				case LPAREN:
					if(was_dollar && open!=LPAREN)
						match_paren(LPAREN,RPAREN);
					break;
			}
			was_dollar = (c==DOLLAR);
		}
		if(c==ESCAPE)
			quoted = 1 - quoted;
		else
			quoted = 0;
	}
	if(open==LITERAL)
		*stakptr(staktell()-1) = DQUOTE;
	return;
}

/*
 * read in here-document from script
 * small non-quoted here-documents are stored as strings 
 * quoted here documents, and here-documents without special chars are
 * treated like file redirection
 */

static int here_copy(ioparg)
struct ionod	*ioparg;
{
	register int	c;
	register char	*bufp;
	register struct ionod *iop;
	register char	*dp;
	int		fd = -1;
	int		match;
	int		savec = 0;
	int		special = 0;
	int		nosubst;
	char		obuff[IOBSIZE+1];
	if(iop=ioparg)
	{
		int stripflg = iop->iofile&IOSTRIP;
		register int nlflg;
		here_copy(iop->iolst);
		iop->iodelim=iop->ioname;
		/* check for and strip quoted characters in ends */
                nosubst = qtrim(iop->iodelim);
		if(stripflg)
			while(*iop->iodelim=='\t')
				iop->iodelim++;
		dp = iop->iodelim;
		match = 0;
		nlflg = stripflg;
		bufp = obuff;
		sh_prompt(0);	
		do
		{
			if(nosubst || savec==ESCAPE)
				c = io_readc();
			else
				c = io_nextc();
			if((savec = c)<=0)
				break;
			else if(c!=ESCAPE || savec==ESCAPE)
				special |= escchar(c);
			if(c=='\n')
			{
				if(match>0 && iop->iodelim[match]==0)
				{
					savec =1;
					break;
				}
				if(match>0)
					goto trymatch;
				sh_prompt(0);	
				nlflg = stripflg;
				match = 0;
				goto copy;
			}
			else if(c=='\t' && nlflg)
				continue;
			nlflg = 0;
			/* try matching delimiter when match>=0 */
			if(match>=0)
			{
			trymatch:
				if(iop->iodelim[match]==c)
				{
					match++;
					continue;
				}
				else if(--match>=0)
				{
					io_unreadc(c);
					dp = iop->iodelim;
					c = *dp++;
				}
			}
		copy:
			do
			{
				*bufp++ = c;
				if(bufp >= &obuff[IOBSIZE])
				{
					if(fd < 0)
						fd = here_tmp(iop);
					write(fd,bufp=obuff,(unsigned)IOBSIZE);
				}
			}
			while(c!='\n' && --match>=0 && (c= *dp++));
		}
		while(savec>0);
		if(c = (nosubst|!special))
                        iop->iofile &= ~IODOC;
		if(fd < 0)
		{
	                if(c)
				fd = here_tmp(iop);
			else
			{
	                        iop->iofile |= IOSTRG;
				*bufp = 0;
				iop->ioname = stakcopy(obuff);
				return(savec);
			}
		}
		if(bufp > obuff)
			write(fd, obuff, (unsigned)(bufp-obuff));
		close(fd);
	}
	return(savec);
}

/*
 * create a temporary file for a here document
 */

static int here_tmp(iop)
register struct ionod *iop;
{
	register int fd = io_mktmp((char*)0);
	iop->ioname = stakcopy(io_tmpname);
	iop->iolst=st.iotemp;
	st.iotemp=iop;
	return(fd);
}


/*
 * trim quotes and the escapes
 * returns non-zero if string is quoted 0 otherwise
 */

static int qtrim(string)
char *string;
{
	register char *sp = string;
	register char *dp = sp;
	register int c;
	register int quote = 0;
	while(c= *sp++)
	{
		if(c == ESCAPE)
		{
			quote = 1;
			c = *sp++;
		}
		else if(c == '"')
		{
			quote = 1;
			continue;
		}
		*dp++ = c;
	}
	*dp = 0;
	return(quote);
}
