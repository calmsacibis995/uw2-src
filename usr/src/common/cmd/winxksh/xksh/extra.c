/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)winxksh:xksh/extra.c	1.2"

#include	"defs.h"
#include	"sym.h"
#include	"builtins.h"
#include	"history.h"
#include	"timeout.h"
#include	"xksh.h"

#ifdef _locale_
#   include	<locale.h>
#   ifdef MULTIBYTE
#	include	"national.h"
	extern unsigned char _ctype[];
#   endif /* MULTIBYTE */
#endif /* _locale_ */

static struct Amemory *
toplev()
{
	struct Amemory *tmp;

	for (tmp = sh.var_tree; tmp->nexttree; tmp = tmp->nexttree)
		;
	return(tmp);
}

struct Amemory *
curlev()
{
	return(sh.var_tree);
}

void
env_set(var)
char *var;
{
	(VOID)env_namset(var, sh.var_tree, 0);
}

void
env_set_gbl(vareqval)
char *vareqval;
{
	env_namset(vareqval, toplev(), 0);
}

char *
env_get(var)
char *var;
{
	struct namnod *np;
	char varbuf[50];
	char *tmp;

	strcpy(varbuf, var);
	if (tmp = strrchr(varbuf, ']')) {
		*tmp = '\0';
		if (tmp = strchr(varbuf, '['))
			*tmp = '\0';
	}
	np = nam_search(varbuf,sh.var_tree,0);
	if (np) {
		if (nam_istype(np, N_ARRAY)) {
			int index;

			if (!tmp)
				return(NULL);
			index = sh_arith(tmp+1);
			return(np->value.namval.aray->val[index]->namval.cp);
		}
		else if (!np->value.namval.cp[0])
			return(NULL);
		else
			return(np->value.namval.cp);
	}
	return(NULL);
}

int
aliasload(name_vals)
struct name_value *name_vals;
{
	loadnames(sh.alias_tree, name_vals);
	return(0);
}

int
varload(name_vals)
struct name_value *name_vals;
{
	loadnames(toplev(), name_vals);
	return(0);
}

int
funcload(name_vals)
struct name_value *name_vals;
{
	loadnames(sh.fun_tree, name_vals);
	return(0);
}

int
int_varload(name, addr)
char *name;
char *addr;
{
	struct nonconst_name_value {
		char *name;
		char *value;
		unsigned short flags;
	} name_vals[2] = {
		{ NULL, NULL, 0 },
		{ NULL, NULL, 0 }
	};
	name_vals[0].name = strdup(name);
	name_vals[0].value = addr;
	name_vals[0].flags = N_INTGER|N_RDONLY;
	varload((struct name_value *) name_vals);
}

static
loadnames(treep, name_vals)
struct Amemory *treep;
struct name_value *name_vals;
{
	register struct namnod *np;
	register struct name_value *nv;
	{
		register unsigned flag = 0;
		for(nv=name_vals;*nv->nv_name;nv++)
			flag++;
		np = (struct namnod*)malloc(flag*sizeof(struct namnod));
		if(sh.bltin_nodes==0)
			sh.bltin_nodes = np;
		else if(name_vals==built_ins)
			sh.bltin_cmds = np;
	}
	{
		for(nv=name_vals;*nv->nv_name;nv++,np++)
		{
			np->namid = (char*)nv->nv_name;
			np->value.namval.cp = (char*)nv->nv_value;
#ifdef apollo
			if(*nv->nv_value==0)
				np->value.namval.cp = 0;
#endif	/* apollo */
			nam_typeset(np,nv->nv_flags);
			if(nam_istype(np,N_INTGER))
				np->value.namsz = 10;
			else
				np->value.namsz = 0;
			nam_link(np, treep);
		}
	}
}

VOID *
xkhash_init(num)
int num;
{
	return((VOID *) gettree(num));
}

void
xkhash_override(tbl, name, val)
struct Amemory *tbl;
const char *name;
VOID *val;
{
	struct namnod *nam;

	if (nam = nam_search(name, tbl, 0))
		nam->value.namval.cp = (char *) val;
}

VOID *
xkhash_find(tbl, name)
struct Amemory *tbl;
const char *name;
{
	struct namnod *nam;

	if (nam = nam_search(name, tbl, 0))
		return((VOID *) nam->value.namval.cp);
	return(NULL);
}

void
xkhash_add(tbl, name, val)
struct Amemory *tbl;
const char *name;
char *val;
{
	struct namnod *nam;

	if (nam = nam_search(name, tbl, N_ADD))
		nam->value.namval.cp = val;
}

int
ksh_eval(cmd)
char *cmd;
{
	char *sav;

	st.states &= ~MONITOR;
	if ((char *)cmd)
	{
		sav = stakptr(0);
		sh_eval((char *)cmd);
		stakset(sav, 0);
	}
	p_flush();
	return(sh.exitval);
}

void
env_set_var(var, val)
char *var, *val;
{
	register int len;
	char tmp[512];
	char *set = &tmp[0];

	if ((len = strlen(var) + strlen(val) + 2) > sizeof(tmp))
		set = malloc(len);
	strcpy(set, var);
	strcat(set, "=");
	strcat(set, val);
	env_set(set);
	if (set != &tmp[0])
		free(set);
}

void
env_blank(var)
char *var;
{
	env_set_var(var, "");
}

void
printerr(cmd, msg1, msg2)
char *cmd, *msg1, *msg2;
{
	mac_check();
	p_setout(ERRIO);
	p_prp(cmd);
	p_str(e_colon,0);
	p_str(msg1, 0);
	if (msg2)
		p_str(msg2, NL);
	else
		newline();
	p_flush();
}

void
printerrf(cmd, fmt, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
char *cmd, *fmt;
char *arg0, *arg1, *arg2, *arg3, *arg4, *arg5, *arg6, *arg7;
{
	char buf[4096];

	sprintf(buf, fmt, arg0, arg1, arg2, arg3,arg4, arg5, arg6, arg7);
	mac_check();
	p_setout(ERRIO);
	p_prp(cmd);
	p_str(e_colon,0);
	p_str(buf, 0);
	newline();
	p_flush();
}

/*
 * Trick: The normal ksh structure for name_value has constant
 * pointers for the name and value.  We have to make another
 * structure the same as that one that is writable.
 */

struct writable_name_value {
	char *nv_name;
	char *nv_value;
	unsigned short nv_flags;
};

int
do_cmdload(argc, argv)
int argc;
char *argv[];
{
	register char *p;
	register int i, numfunc;
	char funcname[512];
	char *cmdname;
	VOID *address;
	VOID *fsym();
#define MAXFUNCS 256
	struct writable_name_value newfuncs[MAXFUNCS];
	char *strdup();

	if (argc < 2) {
		printerrf(argv[0], (const char *) "usage: %s cmdname ...\n", argv[0]);
		return(1);
	}

	for (numfunc = 0, i = 1; i < argc && numfunc < MAXFUNCS-1; i++) {
		if ((p = strchr(argv[i], '=')) != NULL) {
			strncpy(funcname, argv[i], p - argv[i]);
			funcname[p-argv[i]] = '\0';
			cmdname = &p[1];
		} else {
			sprintf(funcname, (const char *) "b_%s", argv[i]);
			cmdname = argv[i];
		}
		if ((address = getaddr(funcname)) == NULL) {
			printerrf(argv[0], (const char *) "could not find function: %s", funcname);
		} else {
			newfuncs[numfunc].nv_name = strdup(cmdname);
			newfuncs[numfunc].nv_value = (char *) address;
			newfuncs[numfunc].nv_flags = (N_BLTIN|BLT_FSUB);
			numfunc++;
		}
	}
	newfuncs[numfunc].nv_name = newfuncs[numfunc].nv_value = (char *)e_nullstr;
	funcload(newfuncs);
	return(0);
}
