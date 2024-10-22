/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)sh:common/cmd/sh/args.c	1.10.10.4"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/sh/args.c,v 1.1 91/02/28 20:08:02 ccs Exp $"

/*
 *	UNIX shell
 */

#include	"defs.h"

static void freedolh();
static struct dolnod *copyargs();

static struct dolnod *dolh;

extern int close();
extern struct dolnod *freeargs();

/* Used to save outermost positional parameters */
static struct dolnod *globdolh;
static unsigned char **globdolv;
static int globdolc;

unsigned char	flagadr[16];

static unsigned char	flagchar[] =
{
	'x',
	'n', 
	'v', 
	't', 
	STDFLG, 
	'i', 
	'e', 
	'r', 
	'k', 
	'u', 
	'h',
	'f',
	'a',
	'm',
	'p',
	 0
};

static long	flagval[]  =
{
	execpr,	
	noexec,	
	readpr,	
	oneflg,	
	stdflg,	
	intflg,	
	errflg,	
	rshflg,	
	keyflg,	
	setflg,	
	hashflg,
	nofngflg,
	exportflg,
	monitorflg,
	privflg,
	  0
};

void	setargs();

static	struct	dolnod	*clean_args();

/* ========	option handling	======== */

int
options(argc,argv)
	unsigned char	**argv;
	int	argc;
{
	register unsigned char *cp;
	register unsigned char **argp = (unsigned char **)argv;
	register unsigned char *flagc;
	unsigned char	*flagp;

	if (argc > 1 && *argp[1] == '-')
	{
		/*
		 * if first argument is "--" then options are not
		 * to be changed. Fix for problems getting 
		 * $1 starting with a "-"
		 */

		cp = argp[1];
		if (cp[1] == '-')
		{
			argp[1] = argp[0];
			argc--;
			return(argc);
		}
		if (cp[1] == '\0')
			flags &= ~(execpr|readpr);

		/*
		 * Step along 'flagchar[]' looking for matches.
		 * 'sicrp' are not legal with 'set' command.
		 */

		while (*++cp)
		{
			flagc = flagchar;
			while (*flagc && *flagc != *cp)
				flagc++;
			if (*cp == *flagc)
			{
				if (eq(argv[0], "set") && any(*cp, "sicrp"))
					failed(SYSSET, argv[1], badopt, badoptid);
				else
				{
					flags |= flagval[flagc-flagchar];
					if (flags & errflg)
						eflag = errflg;
				}
			}
			else if (*cp == 'c' && argc > 2 && comdiv == 0)
			{
				comdiv = argp[2];
				argp[1] = argp[0];
				argp++;
				argc--;
			}
			else
				failed(eq(argv[0], "set") ? SYSSET : 0, argv[1],
					badopt, badoptid);
		}
		argp[1] = argp[0];
		argc--;
	}
	else if (argc > 1 && *argp[1] == '+')	/* unset flags x, k, t, n, v, e, u */
	{
		cp = argp[1];
		while (*++cp)
		{
			flagc = flagchar;
			while (*flagc && *flagc != *cp)
				flagc++;
			/*
			 * step through flags
			 */
			if (!any(*cp, "sicrp") && *cp == *flagc)
			{
				/*
				 * only turn off if already on
				 */
				if ((flags & flagval[flagc-flagchar]))
				{
					flags &= ~(flagval[flagc-flagchar]);
					if (*cp == 'e')
						eflag = 0;
				}
			}
		}
		argp[1] = argp[0];
		argc--;
	}
	/*
	 * set up $-
	 */
	flagp = flagadr;
	if (flags)
	{
		flagc = flagchar;
		while (*flagc)
		{
			if (flags & flagval[flagc-flagchar])
				*flagp++ = *flagc;
			flagc++;
		}
	}
	*flagp = 0;
	return(argc);
}

/*
 * sets up positional parameters
 */
void
setargs(argi)
	unsigned char	*argi[];
{
	register unsigned char **argp = argi;	/* count args */
	register int argn = 0;

	while (*argp++ != (unsigned char *)ENDARGS)
		argn++;
	/*
	 * free old ones unless on for loop chain
	 */
	freedolh();
	dolh = copyargs(argi, argn);
	dolc = argn - 1;
}


static void
freedolh()
{
	register unsigned char **argp;
	register struct dolnod *argblk;

	argblk = dolh;
	if (argblk)
	{
		if ((--argblk->doluse) == 0)
		{
			for (argp = argblk->dolarg; *argp != (unsigned char *)ENDARGS; argp++)
				free(*argp);
			free(argblk->dolarg);
			free(argblk);
		}
	}
}

struct dolnod *
freeargs(blk)
	struct dolnod *blk;
{
	register unsigned char **argp;
	register struct dolnod *argr = 0;
	register struct dolnod *argblk;
	int cnt;

	argblk = blk;
	if (argblk)
	{
		argr = argblk->dolnxt;
		cnt  = --argblk->doluse;

		if (argblk == dolh)
		{
			if (cnt == 1)
				return(argr);
			else
				return(argblk);
		}
		else
		{			
			if (cnt == 0)
			{
				for (argp = argblk->dolarg; *argp != (unsigned char *)ENDARGS; argp++)
					free(*argp);
				free(argblk->dolarg);
				free(argblk);
			}
		}
	}
	return(argr);
}

static struct dolnod *
copyargs(from, n)
	unsigned char	*from[];
{
	register struct dolnod *np = (struct dolnod *)alloc(sizeof(struct dolnod));
	register unsigned char **fp = from;
	register unsigned char **pp;

	np -> dolnxt = 0;
	np->doluse = 1;	/* use count */
	pp = np->dolarg = (unsigned char **)alloc((n+1)*sizeof(char *));
	dolv = pp;
	
	while (n--)
		*pp++ = make(*fp++);
	*pp++ = ENDARGS;
	return(np);
}


static struct dolnod *
clean_args(blk)
	struct dolnod *blk;
{
	register unsigned char **argp;
	register struct dolnod *argr = 0;
	register struct dolnod *argblk;

	argblk = blk;
	if (argblk)
	{
		argr = argblk->dolnxt;

		if (argblk == dolh)
			argblk->doluse = 1;
		else
		{
			for (argp = argblk->dolarg; *argp != (unsigned char *)ENDARGS; argp++)
				free(*argp);
			free(argblk->dolarg);
			free(argblk);
		}
	}
	return(argr);
}

void
clearup()
{
	/*
	 * force `for' $* lists to go away
	 */
	if(globdolv) 
		dolv = globdolv;
	if(globdolc)
		dolc = globdolc;
	if(globdolh)
		dolh = globdolh;
	globdolv = 0;
	globdolc = 0;
	globdolh = 0;
	while (argfor = clean_args(argfor))
		;
	/*
	 * clean up io files
	 */
	while (pop())
		;

	/*
	 * Clean up pipe file descriptor
	 * from command substitution
	 */
	
	if(savpipe != -1) {
		(void)close(savpipe);
		savpipe = -1;
	}

	/*
	 * clean up tmp files
	*/
	while (poptemp())
		;
}

/*
 * Save positiional parameters before outermost function invocation 
 * in case we are interrupted. 
 * Increment use count for current positional parameters so that they
 * aren't thrown away. 
 */

struct dolnod *savargs(fcount)
int fcount;
{
	if (!fcount) {
		globdolh = dolh;
		globdolv = dolv;
		globdolc = dolc;
	}
	(void)useargs();
	return(dolh);
}

/* After function invocation, free positional parameters, 
 * restore old positional parameters, and restore
 * use count.
 */

void
restorargs(olddolh, fcount)
struct dolnod *olddolh;
int fcount;
{
	if(argfor != olddolh)
		while ((argfor = clean_args(argfor)) != olddolh && argfor);
	if(!argfor)
		return;
	freedolh();
	dolh = olddolh;
	if(dolh)
		dolh -> doluse++; /* increment use count so arguments aren't freed */
	argfor = freeargs(dolh);
	if(fcount == 1) { 
		globdolh = 0;
		globdolv = 0;
		globdolc = 0;
	}
}

struct dolnod *
useargs()
{
	if (dolh)
	{
		if (dolh->doluse++ == 1)
		{
			dolh->dolnxt = argfor;
			argfor = dolh;
		}
	}
	return(dolh);
}


