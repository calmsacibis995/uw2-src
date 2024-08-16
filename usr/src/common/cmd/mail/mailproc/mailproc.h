/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/mailproc/mailproc.h	1.1.1.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)mailproc.h	1.2 'attmail mail(1) command'"
/* from ferret SCCSid ferret.h 3.1 */

/* Structure of the mail file linked list */
typedef struct Msg
{
	struct	Msg	*next;			/* next message */
	int		processed;		/* has msg been processed? */
	int		keep;			/* keep this msg */
	char		date[29];		/* msg date */
	char		subject[80];		/* msg subject */
	char		status[8];		/* msg status */
	long		startmsg;		/* start of message */
	long		starttxt;		/* start of msg text */
	int		msize;			/* size of message */
	int		tsize;			/* size of text */
	struct	Fromlist *fhp;			/* host component list */
} Msg;

typedef struct Cmd
{
	struct	Cmd	*next;			/* next in line */
	char		cmdbuf[512];		/* command buffer */
	struct	Fromlist *flp;			/* pattern component list */
} Cmd;

/*
 * define parser return codes
 */
#define	P_MATCH		0
#define	P_NOMATCH	1
#define	P_EOF		2
#define	P_ERROR		3
#define	P_ERR_FROM	4
#define	P_ERR_MEM	5
#define	P_ERR_TSYS	6
#define	P_ERR_ASYS	7

/*
 * define message movement types
 */
#define M_MSG		0
#define	M_TXT		1
#define	M_DIR		2
#define	M_APP		4
#define	M_HDR		8

/*
 * define structures for matching names with patterns
 */
typedef struct Fhost
{
	char		*hostname;	/* host name component */
	struct	Fhost	*prev;		/* previous component */
	struct	Fhost	*next;		/* pointer to next */
} Fhost;

typedef struct Fromlist
{
	char		*user;		/* user name portion */
	int		domain;		/* uses domain syntax? */
	struct	Fhost	*match;		/* part that matches */
	struct	Fhost	*first;		/* Fhost list */
	struct	Fhost	*last;		/* last in list */
} Fromlist;

extern	int	matchstr ARGS((const char*, const char*));	/* match string against pattern */
extern	int	cmdparse ARGS((char*));				/* parse command file */
extern	int	copyback ARGS((int, int, Msg*));		/* copy back into mail file */
extern	int	do_system ARGS((char*));			/* front end to system(3) and coshell version */
extern	int	matchfp ARGS((Fromlist*, Fromlist*));		/* match from address patterns */
extern	int	mvmsg ARGS((Msg*, char*, int, int, int));	/* write piece of message to file */
extern	Cmd	*new_Cmd ARGS((char*));				/* alloc a new Cmd */
extern	Fhost	*new_Fhost ARGS((char *, int));			/* alloc a new Fhost */
extern	Fromlist *new_Fromlist ARGS((void));			/* alloc a new Fromlist */
extern	Msg	*new_Msg ARGS((int,int));			/* alloc a new Msg */
extern	void	parsefrom ARGS((char*, Msg*));			/* parse from lines */
extern	int	parsemail ARGS((FILE*, FILE*));			/* parse mail file */
extern	int	pfrom ARGS((Fromlist*, char*, char*));		/* parse a from address */
extern	int	process ARGS((Msg*));				/* process a message */
extern	void	prtfrom ARGS((FILE*, Fromlist*, int));		/* print the from address */
extern	const char	*quotechars ARGS((const char*));	/* quote special characters */
extern	int	setfile ARGS((char *, int));			/* get filename containing given type */
extern	void	sprtfrom ARGS((char*, Fromlist*, int));		/* create a string with the from address */

/* yacc and lex functions and variables */
extern	int	input ARGS((void));
extern	int	unput ARGS((int));
extern	int	yylex ARGS((void));
extern	int	yyparse ARGS((void));
extern	int	yywrap ARGS((void));
extern	void	yyless ARGS((int));
extern	void	yyerror ARGS((const char*));
extern	int	yydebug;
extern	char	yytext[];

extern	Cmd	*Cmdptr;		/* current command */
extern	Cmd	*Cmdlist;		/* command list */
extern	Msg	*Msgptr;		/* current message */
extern	Msg	*msglist;		/* message list */
extern	FILE	*Mailfp;		/* mail file pointer */
extern	int	Prtdomain;		/* use domain style address? */
extern	char	Quoted;			/* is this string to be quoted? */
extern	int	aflg;			/* process all messages? */
extern	int	dflg;			/* debug flag */
extern	int	lflg;			/* list only */
extern	int	nflg;			/* no action flag */
extern	int	oflg;			/* output flag, printf stdout of cmd */
extern	int	pflg;			/* read input as 1 msg */
extern	int	rflg;			/* read-only */
extern	int	sflg;			/* read stdin */
extern	unsigned char re_map[256];	/* use for regex case folding */
