/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)tfm:adminrole.c	1.12.2.4"
#ident  "$Header: adminrole.c 1.2 91/06/27 $"

#include <sys/types.h>
#include <limits.h>
#include <stdio.h>
#include <priv.h>
#include <locale.h>
#include <pfmt.h>
#include "err.h"
#include "tfm.h"

/***************************************************************************
 * Command: adminrole
 * Inheritable Privileges: None
 *       Fixed Privileges: None
 * Notes: Administer the TFM role list.  While this command does not
 *        currently inherit privilege, it is designed to work correctly
 *        with the P_MACWRITE, P_SETFLEVEL privileges.  When these are
 *        applied, and all administrative logins are moved above
 *        SYS_PRIVATE it will be possible to make TFM database
 *        operations privileged.
 *
 ***************************************************************************/

/*
** Error messages generated by adminrole
*/
#define	MALLOC	":2:Memory allocation failed\n"
#define	CMDXST	":3:Command name \"%s\" already exists\n"
#define	ROLEXST	":16:Role name \"%s\" already exists\n"
#define	MISSROL	":17:Undefined role name \"%s\"\n"
#define	FEWPRV	":7:Process privilege \"%s\" does not exist in command \"%s\"\n"
#define	BADUSG	":8:Incorrect usage\n"
#define	USAGE	":18:\nusage: adminrole [-n] [-a cmd:path[:priv[:priv...]][,...]] role ...\n       adminrole [-r cmd[:priv[:priv...]][,...]] \n                 [-a cmd:path[:priv[:priv...]][,...]] role ...\n       adminrole [-d] role ... \n       adminrole \n"
extern	char	*strncpy(),*privname();
extern	int	misspriv(),hasprivs();
extern  void    *malloc();
extern	void	exit(),free();

typedef	struct	{
	tfm_cmdlst	*add;
	tfm_cmdlst	*rem;
	int		increase;
	int		delflg;
	int		nopt;
} syntax;

extern	void	showprivs();

static	void	addcmds(),remcmds(),usage(),showrole(),showcmd();
static	void	showall();
static	int	options();

static	int	exitval = 0;

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
	tfm_name	role;
	int		i;
	syntax		opt;

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxes");
	(void)setlabel("UX:adminrole");
	if(tfm_setup(TFM_ROOT) < 0){
		(void)tfm_err(ERR_QUIT);
	}
/*
**If there are no arguments, show the entire database
*/
	if(argc < 2){
		showall();
		exit(exitval);
	}
/*
**If there were arguments, then there must be a list of roles.
*/
	if((i = options(&opt,argc,argv)) >= argc){
		usage();
	}
/*
**Process the list of roles
*/
	if(opt.increase && (tfm_ckdb() < 0) && (tfm_newdb() < 0)){
		(void)tfm_err(ERR_QUIT);
	}
	for(;i < argc; ++i){
		(void)strncpy(role,argv[i],sizeof(tfm_name));
		if(opt.nopt){
			if(tfm_ckrole(&role) > -1){
				(void)pfmt(stderr,MM_ERROR,ROLEXST,role);
				exitval = 1;
				continue;
			} else if(tfm_newrole(&role)<0) {
				(void)tfm_err(ERR_CONTINUE);
				exitval = 1;
				continue;
			}
		}
		if(opt.delflg){
			if(tfm_ckrole(&role) < 0){
				(void)tfm_err(ERR_CONTINUE);
				exitval = 1;
			} else if(tfm_killrole(&role) < 0){
				(void)tfm_err(ERR_CONTINUE);
				exitval = 1;
			}
		} else if(opt.add || opt.rem) {
			remcmds(&role,opt.rem);
			addcmds(&role,opt.add);
		} else if(!opt.nopt){
			showrole(&role);
		}
	}
	exit(exitval);
/*NOTREACHED*/
}

/*
** Function: addcmds()
**
** Notes:
** Add the list of commands pointed to by 'l' to the role specified by
** 'r'. If an error is encountered, report it and exit with a 1.  If no
** list is specified in 'l', just return.
*/
static	void
addcmds(r,l)
tfm_name	*r;
tfm_cmdlst	*l;
{
	int		i;
	tfm_cmd		tmp;

	if(!l){
		return;
	}
	if(tfm_ckrole(r)<0){
		(void)tfm_err(ERR_QUIT);
	}
	for(i = 0; i < l->ncmds; ++i){
		if(tfm_getrcmd(r,&(l->names[i]),&tmp) > -1){
			(void)pfmt(stderr,MM_ERROR,CMDXST,l->names[i]);
			exitval = 1;
		} else if(tfm_putrcmd(r,&(l->names[i]),&(l->cmds[i])) < 0){
			(void)tfm_err(ERR_CONTINUE);
			exitval = 1;
		}
	}
}

/*
** Function: remcmds()
**
** Notes:
** Remove the commands in the list pointed to by 'l', from the role 'r'.
** The procedure for removing a command is as follows:
**	1) If privileges are specified, remove the specified privileges
**	   from the command definition, and leave the definition otherwise
**	   unchanged.
**	2) If no privileges are specified, then remove the entire command
**	   definition.
*/
static	void
remcmds(r,l)
tfm_name	*r;
tfm_cmdlst	*l;
{
	int		i,pr;
	tfm_cmd		cbuf;
	tfm_invoke	in,out;
	unsigned	outvect,invect;
	char		buf[BUFSIZ];

	if(!l){
		return;
	}
	for(i = 0; i < l->ncmds; ++i){
		if(tfm_gotpriv(&(l->cmds[i]))){
			if(tfm_getrcmd(r,&(l->names[i]),&cbuf) < 0){
				(void)tfm_err(ERR_QUIT);
			}
			cmd2inv(&out,&cbuf);
			cmd2inv(&in,&(l->cmds[i]));
			inv_vect(&outvect,&out);
			inv_vect(&invect,&in);
			if(!hasprivs(invect, outvect)){
				pr = -1;
				while((pr=misspriv(invect, outvect,pr)) > -1){
					(void)pfmt(stderr,MM_WARNING,FEWPRV,
						privname(buf,pr),l->names[i]);
				}
				exitval = 1;
			}
			tfm_rmpriv(&cbuf,&(l->cmds[i]));
			if(tfm_putrcmd(r,&(l->names[i]),&cbuf) < 0){
				(void)tfm_err(ERR_QUIT);
			}
		} else if(tfm_getrcmd(r,&(l->names[i]),&cbuf) < 0){
			(void)tfm_err(ERR_CONTINUE);
			exitval = 1;
		} else if(tfm_putrcmd(r,&(l->names[i]),(tfm_cmd *)0) < 0){
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
int	argc;
char	**argv;
syntax	*opt;
{
	tfm_cmdlst	*l;
	int		i,j;
	char		*p;

	opt->delflg = opt->nopt = 0;
	opt->rem = opt->add = (tfm_cmdlst *)0;
	for(i = 1; i < argc; ++i){
		p = argv[i];
		if(*(p++) != '-'){
			break;
		}
		switch(*(p++)){
		case 'n':
			if(opt->delflg || opt->rem || opt->nopt){
				usage();	/*-d -n role -- or*/
						/*-r <cmd> -n role*/
			}
			opt->nopt = 1;
			opt->increase = 1;
			continue;
		case 'a':
			if(opt->delflg || opt->add){
				usage();	/*-d -a <cmd> role*/
			}
			l=opt->add=(tfm_cmdlst *)malloc(sizeof(tfm_cmdlst));
			break;
		case 'r':
			if(opt->delflg || opt->nopt || opt->rem){
				usage();	/*-d -r <cmd> role -- or*/
						/*-n -r <cmd> role	*/
			}
			l=opt->rem=(tfm_cmdlst *)malloc(sizeof(tfm_cmdlst));
			break;
		case 'd':
			if(opt->add || opt->rem || opt->nopt || opt->delflg){
				usage();	/*-[nra] <cmd> -d role*/
			}
			opt->delflg = 1;
			continue;
		default:
			usage();		/*bad option letter*/
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
		if(parselist(argv[i],l,(l == opt->add)) < 0) {
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
** Display the contents of all roles in the TFM database.
*/
static	void
showall()
{
	long		sz;
	int		i;
	tfm_name	*list;

	sz = tfm_getroles((char *)0,0);
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
	if((sz = tfm_getroles((tfm_namelist *)list,sz)) < 0){
		(void)tfm_err(ERR_CONTINUE);
		exitval = 1;
		free((void *)list);
		return;
	}
	for(i = 0; i < sz; ++i){
		showrole(&list[i]);
	}
	free((void *)list);
}

/*
** Function: showrole()
**
** Notes:
** Display the contents of the role 'r' in the TFM database.
*/
static	void
showrole(r)
tfm_name	*r;
{
	long		sz;
	int		i;
	tfm_cname	*list;

	if(tfm_ckrole(r) < 0){
		(void)pfmt(stderr,MM_WARNING,MISSROL,(char *)r);
		exitval = 1;
		return;
	}
	sz = tfm_rcmds(r,(char *)0,0);
	if(sz < 0){
		(void)tfm_err(ERR_CONTINUE);
		exitval = 1;
		return;
	}
	if(sz == 0){
		(void)pfmt(stdout,MM_NOSTD,":19:%s:\t<empty>\n",(char *)r);
		return;
	}
	list = (tfm_cname *)malloc((unsigned)(sz * sizeof(tfm_cname)));
	if(!list){
		(void)pfmt(stderr,MM_ERROR,MALLOC);
		exit(1);
	}
	if((sz = tfm_rcmds(r,(char *)list,sz)) < 0){
		(void)tfm_err(ERR_CONTINUE);
		exitval = 1;
		free((void *)list);
		return;
	}
	(void)pfmt(stdout,MM_NOSTD,":20:%s:",(char *)r);
	for(i = 0; i < sz; ++i){
		showcmd(r,&list[i]);
	}
	free((void *)list);
}

/*
** showcmd()
**
** Notes:
** Display the command definition 'c' from the role 'r' in the TFM
** database.
*/
static	void
showcmd(r,c)
tfm_name	*r;
tfm_cname	*c;
{
	tfm_cmd		cmd;
	tfm_invoke	inv;
	unsigned	vec;

	if(tfm_getrcmd(r,c,&cmd) < 0){
		(void)tfm_err(ERR_CONTINUE);
		exitval = 1;
		return;
	}
	(void)pfmt(stdout,MM_NOSTD,":21:\t%s:%s ",(char *)c,cmd.cmd_path);	
	cmd2inv(&inv,&cmd);
	inv_vect(&vec,&inv);
	showprivs(vec);
	(void)printf("\n");
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
