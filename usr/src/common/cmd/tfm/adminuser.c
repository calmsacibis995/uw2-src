/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)tfm:adminuser.c	1.12.2.4"
#ident  "$Header: adminuser.c 1.2 91/06/27 $"

#include <limits.h>
#include <stdio.h>
#include <sys/types.h>
#include <priv.h>
#include <locale.h>
#include <pfmt.h>
#include "err.h"
#include "tfm.h"

/***************************************************************************
 * Command: adminuser
 * Inheritable Privileges: None
 *       Fixed Privileges: None
 * Notes: Administer the TFM user list.  While this command does not
 *        currently inherit privilege, it is designed to work correctly
 *        with the P_MACWRITE, P_SETFLEVEL privileges.  When these are
 *        applied, and all administrative logins are moved above
 *        SYS_PRIVATE it will be possible to make TFM database
 *        operations privileged.
 *
 ***************************************************************************/

/*
** Error messages generated by adminuser
*/
#define	MALLOC	":2:Memory allocation failed\n"
#define	CMDXST	":3:Command name \"%s\" already exists\n"
#define	USERXST	":4:User \"%s\" already exists\n"
#define	DUPROLE	":5:Role name \"%s\" is not unique\n"
#define	MISSUSR	":6:Undefined user \"%s\"\n"
#define	FEWPRV	":7:Process privilege \"%s\" does not exist in command \"%s\"\n"
#define BADUSG	":8:Incorrect usage\n"
#define	USAGE	":9:\nusage: adminuser [-n] [-o role[,...]]\n                 [-a cmd:path[:priv[:priv...]][,...]]\n                 user ...\n       adminuser [-o role[,...]]\n                 [-r cmd[:priv[:priv...]][,...]]\n                 [-a cmd:path[:priv[:priv...]][,...]]\n                 user ...\n       adminuser [-d] user ...\n       adminuser\n"
extern	int	hasprivs(),misspriv(),strcmp();
extern	char	*strncpy(),*privname();
extern	void	*memset(),*malloc();
extern	void	exit(),free();

typedef	struct	{
	int		nopt;
	int		dopt;
	int		oopt;
	int		increase;
	long		nroles;
	tfm_name	*rlist;
	tfm_cmdlst	*add;
	tfm_cmdlst	*rem;
} syntax;

extern	void	showprivs();

static	void	addcmds(),remcmds(),usage(),showuser(),showcmd();
static	void	showall(),defuser();
static	int	options(),notthere();
static	tfm_name	*mkrlist();

static	int	exitval=0;

/*
** Function: main()
**
** Notes:
** Main program entry point, process arguments and call appropriate
** functions to handle request.
*/
main(argc,argv)
int	argc;
char	*argv[];
{
	tfm_name	user;
	int		i;
	syntax		opt;
	
	(void)setlocale(LC_ALL,"");
	(void)setcat("uxes");
	(void)setlabel("UX:adminuser");

	if(tfm_setup(TFM_ROOT) < 0){
		(void)tfm_err(ERR_QUIT);
	}
	if(argc < 2){
		showall();
		exit(exitval);
	}
	if((i = options(&opt,argc,argv)) >= argc){
		usage();
	}
	if(opt.increase && (tfm_ckdb() < 0) && (tfm_newdb() < 0)){
		(void)tfm_err(ERR_QUIT);
	}
	for(;i < argc; ++i){
		(void)strncpy(user,argv[i],sizeof(tfm_name));
 		if(opt.dopt){
			if((tfm_ckuser(&user) < 0) ||
						(tfm_killuser(&user) < 0)){
				(void)tfm_err(ERR_CONTINUE);
				exitval = 1;
			}
		} else if(opt.nopt || opt.rem || opt.add || opt.oopt) {
			defuser(&user,&opt);
		} else if(!opt.nopt){
			showuser(&user);
		}
	}
	exit(exitval);
/*NOTREACHED*/
}

/*
** Function: defuser()
**
** Notes:
** Define a new user or redefine an old one based on the information
** provided in 'opt.'
*/
static	void
defuser(user,opt)
tfm_name	*user;
syntax		*opt;
{
 	if(opt->nopt){
		if(tfm_ckuser(user) > -1){
			(void)pfmt(stderr,MM_ERROR,USERXST,(char *)user);
			exitval = 1;
			return;
		} else if(tfm_newuser(user)<0){
			(void)tfm_err(ERR_CONTINUE);
			exitval = 1;
			return;
		}
	} else if(tfm_ckuser(user) < 0){
		(void)tfm_err(ERR_CONTINUE);
		exitval = 1;
		return;
	}
	if(opt->oopt && (tfm_putroles(user,(tfm_namelist *)opt->rlist,
							opt->nroles) < 0)){
		(void)tfm_err(ERR_CONTINUE);
		exitval = 1;
	}
	remcmds(user,opt->rem);
	addcmds(user,opt->add);
}

/*
** Function: addcmds()
**
** Notes:
** Add a the list of commands pointed to by 'l' to the user specified by
** 'u'. If an error is encountered, report it and exit with a 1.  If no
** list is specified in 'l', just return.
*/
static	void
addcmds(u,l)
tfm_name	*u;
tfm_cmdlst	*l;
{
	int	i;
	tfm_cmd	tmp;

	if(!l){
		return;
	}
	if(tfm_ckuser(u) < 0){
		(void)tfm_err(ERR_QUIT);
	}
	for(i = 0; i < l->ncmds; ++i){
		if(tfm_getucmd(u,&(l->names[i]),&tmp) > -1){
			(void)pfmt(stderr,MM_ERROR,CMDXST,(char *)l->names[i]);
			exitval = 1;
		} else if(tfm_putucmd(u,&(l->names[i]),&(l->cmds[i])) < 0){
			(void)tfm_err(ERR_CONTINUE);
			exitval = 1;
		}
	}
}

/*
** Function: remcmds()
**
** Notes:
** Remove the commands in the list pointed to by 'l', from the user 'u'.
** The procedure for removing a command is as follows:
**	1) If privileges are specified, remove the specified privileges
**	   from the command definition, and leave the definition otherwise
**	   unchanged.
**	2) If no privileges are specified, then remove the entire command
**	   definition.
*/
static	void
remcmds(u,l)
tfm_name	*u;
tfm_cmdlst	*l;
{
	int		i,pr;
	tfm_cmd		cbuf;
	tfm_invoke	in,out;
	unsigned	invect,outvect;
	char		buf[BUFSIZ];

	if(!l){
		return;
	}
	for(i = 0; i < l->ncmds; ++i){
		if(tfm_gotpriv(&(l->cmds[i]))){
			if(tfm_getucmd(u,&(l->names[i]),&cbuf) < 0){
				(void)tfm_err(ERR_QUIT);
			}
			cmd2inv(&out,&cbuf);
			cmd2inv(&in,&(l->cmds[i]));
			inv_vect(&outvect,&out);
			inv_vect(&invect,&in);
			if(!hasprivs(invect,outvect)){
				pr = -1;
				while((pr=misspriv(invect, outvect ,pr)) > -1){
					(void)pfmt(stderr,MM_WARNING,FEWPRV,
							privname(buf,pr),
							(char *)l->names[i]);
				}
				exitval = 1;
			}
			tfm_rmpriv(&cbuf,&(l->cmds[i]));
			if(tfm_putucmd(u,&(l->names[i]),&cbuf) < 0){
				(void)tfm_err(ERR_QUIT);
			}
		} else if(tfm_getucmd(u,&(l->names[i]),&cbuf) < 0){
			(void)tfm_err(ERR_CONTINUE);
			exitval = 1;
		} else if(tfm_putucmd(u,&(l->names[i]),(tfm_cmd *)0) < 0){
			(void)tfm_err(ERR_QUIT);
		}
	}
}

/*
** Function: options()
**
** Notes:
** Process command line options and set up the 'opt' structure with the
** information from the command line.
*/
static	int
options(opt,argc,argv)
syntax	*opt;
int	argc;
char	**argv;
{
	tfm_cmdlst	*l;
	int		i,j;
	char		*p;

	(void)memset(opt,0,sizeof(syntax));
	for(i = 1; i < argc; ++i){
		p = argv[i];
		if(*(p++) != '-'){
			break;
		}
		switch(*(p++)){
		case 'n':
			if(opt->dopt || opt->rem || opt->nopt){
				usage();	/*Does not return*/
			}
			opt->increase = 1;
			opt->nopt = 1;
			continue;
		case 'o':
			if(opt->dopt || opt->oopt){
				usage();	/*Does not return*/
			}
			if(*p){
				opt->rlist = mkrlist(opt,p);
			} else if(++i < argc){
				opt->rlist = mkrlist(opt,argv[i]);
			} else {
				usage();
			}
			opt->oopt = 1;
			opt->increase = 1;
			continue;
		case 'a':
			if(opt->dopt || opt->add){
				usage();	/*Does not return*/
			}
			l=opt->add=(tfm_cmdlst *)malloc(sizeof(tfm_cmdlst));
			opt->increase = 1;
			break;
		case 'r':
			if(opt->dopt || opt->nopt || opt->rem){
				usage();	/*Does not return*/
			}
			l=opt->rem=(tfm_cmdlst *)malloc(sizeof(tfm_cmdlst));
			break;
		case 'd':
			if(opt->add || opt->rem || opt->nopt
			|| opt->oopt || opt->dopt){
				usage();	/*Does not return*/
			}
			opt->dopt = 1;
			continue;
		default:
			usage();	/*Does not return*/
		}
/*
** If we got here, we have the -a or -r option, space has been allocated,
** and we are ready to parse a command list.
*/
		if(!l){
			(void)pfmt(stderr,MM_ERROR,MALLOC);
			exit(1);
		}
		if(*p){	/*List is in current argument*/
			argv[i] = p;
		} else if(++i >= argc){
			usage();
		}
		if(parselist(argv[i],l,(l == opt->add)) < 0){
			(void)tfm_err(ERR_QUIT);
		}
	}
/*
** If we got here, there should be no more options on the line, check that
** this is true, and, if it is not, issue a usage message and exit with
** an error.
*/
	for(j = i; j < argc; ++j){
		if(*(argv[j]) == '-'){
			usage();
		}
	}
/*
** Everything looks fine, return the position of the next argument.
*/
	return(i);		
}

/*
** Function: showall()
**
** Notes:
** Display the contents of all users in the TFM database.
*/
static	void
showall()
{
	long		sz;
	int		i;
	tfm_name	*list;

	sz = tfm_getusers((char *)0,0);
	if(sz < 0){
		(void)tfm_err(ERR_CONTINUE);
		exitval = 1;
		return;
	}
	if(sz == 0){
		return;
	}
	list = (tfm_name *)malloc((unsigned)(sz * sizeof(tfm_name)));
	if(!list){
		(void)pfmt(stderr,MM_ERROR,MALLOC);
		exit(1);
	}
	if((sz = tfm_getusers((tfm_namelist *)list,sz)) < 0){
		(void)tfm_err(ERR_CONTINUE);
		exitval = 1;
		free((void *)list);
		return;
	}
	for(i = 0; i < sz; ++i){
		showuser(&list[i]);
	}
	free((void *)list);
}

/*
** Function: showuser()
**
** Notes:
** Display the contents of the user 'u' in the TFM database.
*/
static	void
showuser(u)
tfm_name	*u;
{
	long		sz;
	int		i;
	tfm_name	*list;
	tfm_cname	*clist;

	if(tfm_ckuser(u) < 0){
		(void)pfmt(stderr,MM_WARNING,MISSUSR,(char *)u);
		exitval = 1;
		return;
	}
	(void)pfmt(stdout,MM_NOSTD,":10:%s:\n",(char *)u);
	sz = tfm_roles(u,(char *)0,0);
	if(sz <= 0){
		(void)pfmt(stdout,MM_NOSTD,":11:roles:     <none>\n");
	} else {
		list = (tfm_name *)malloc((unsigned)(sz * sizeof(tfm_name)));
		if(!list){
			(void)pfmt(stderr,MM_ERROR,MALLOC);
			exit(1);
		}
		if((sz = tfm_roles(u,list,sz)) < 0){
			(void)tfm_err(ERR_CONTINUE);
			exitval = 1;
			free((void *)list);
			return;
		}
		(void)pfmt(stdout,MM_NOSTD,":12:roles:     ");
		for(i = 0; i < sz-1; ++i){
			(void)printf("%s,",(char *)list[i]);
		}
		(void)printf("%s\n",(char *)list[i]);
		free((void *)list);
	}
	sz = tfm_ucmds(u,(char *)0,0);
	if(sz < 0){
		(void)tfm_err(ERR_CONTINUE);
		exitval = 1;
		return;
	}
	if(sz == 0){
		(void)pfmt(stdout,MM_NOSTD,":13:Commands:\n           <none>\n");
	} else {
		clist=(tfm_cname *)malloc((unsigned)(sz * sizeof(tfm_cname)));
		if(!clist){
			(void)pfmt(stderr,MM_ERROR,MALLOC);
			exit(1);
		}
		if((sz = tfm_ucmds(u,(char *)clist,sz)) < 0){
			(void)tfm_err(ERR_CONTINUE);
			exitval = 1;
			free((void *)clist);
			return;
		}
		(void)pfmt(stdout,MM_NOSTD,":14:Commands:\n");
		for(i = 0; i < sz; ++i){
			showcmd(u,&clist[i]);
		}
		free((void *)clist);
	}
}

/*
** showcmd()
**
** Notes:
** Display the command definition 'c' from the user 'u' in the TFM
** database.
*/
static	void
showcmd(u,c)
tfm_name	*u;
tfm_cname	*c;
{
	tfm_cmd		cmd;
	tfm_invoke	inv;
	unsigned	vec;

	if(tfm_getucmd(u,c,&cmd) < 0){
		(void)tfm_err(ERR_CONTINUE);
		exitval = 1;
	} else {
		(void)pfmt(stdout,MM_NOSTD,":15:           %s:%s ",(char *)c,cmd.cmd_path);
	}
	cmd2inv(&inv,&cmd);
	inv_vect(&vec,&inv);
	showprivs(vec);
	(void)printf("\n");
}

/*
** Function: mkrlist()
**
** Notes:
** Given a comma separated list of roles: 'lst', allocate and fill out a
** role list buffer to put in a user's role list file.  This is an array
** of tfm_names which do not need to be null terminated since the tfmlib
** routines will truncate any names they get.
*/
static	tfm_name	*
mkrlist(opt,lst)
syntax	*opt;
char	*lst;
{
	char		*p;
	int		i;

	if(!*lst){
		usage();
	}
/*
** Scan the list to determine the number of roles.
*/
	p = lst;
	opt->nroles = 1;
	while(*p){
		if(*p == ','){
			++opt->nroles;
		}
		++p;
	}
/*
** Allocate the buffer once we know its size.
*/
	opt->rlist = (tfm_name *)malloc((unsigned)(opt->nroles * sizeof(tfm_name)));
	if(!opt->rlist){
		(void)pfmt(stderr,MM_ERROR,MALLOC);
		exit(1);
	}
/*
** Scan the list of roles again, and copy it into the buffer.
*/
	p = lst;
	i = 0;
	while(*p){
		while(*p && (*p != ',')){
			++p;
		}
		if(*p){
			*(p++) = 0;
		}
		if(notthere(lst,opt)){
			(void)strncpy(opt->rlist[i++], lst, sizeof(tfm_name));
		} else {
			(void)pfmt(stderr,MM_ERROR,DUPROLE,lst);
			exit(1);
		}
		lst = p;
	}
	return(opt->rlist);
}

/*
** Function: notthere()
**
** Notes:
** Scan the array of strings specified in 'list' for the string 'str'. 
** If 'str' is not in the list, return 1 (true), otherwise return 0
** (false).
*/
static	int
notthere(str,opt)
char	*str;
syntax	*opt;
{
	int	i;

	for(i = 0; i < opt->nroles; ++i){
		if(!strcmp(opt->rlist[i],str)){
			return(0);
		}
	}
	return(1);
}

/*
** Function: usage()
**
** Notes:
** Print the usage message and exit with an error.
*/
static	void
usage()
{
	pfmt(stderr, MM_ERROR, BADUSG);
	pfmt(stderr, MM_ACTION, USAGE);
	exit(1);
}