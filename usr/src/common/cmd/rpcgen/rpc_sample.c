/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)rpcgen:rpc_sample.c	1.3"
#ident  "$Header: $"

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*	PROPRIETARY NOTICE (Combined)
*
* This source code is unpublished proprietary information
* constituting, or derived under license from AT&T's UNIX(r) System V.
* In addition, portions of such source code were derived from Berkeley
* 4.3 BSD under license from the Regents of the University of
* California.
*
*
*
*	Copyright Notice 
*
* Notice of copyright on this source code product does not indicate 
*  publication.
*
*	(c) 1986,1987,1988,1989,1990  Sun Microsystems, Inc
*	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
*	(c) 1990,1991,1992  UNIX System Laboratories, Inc.
*          All rights reserved.
*/ 

/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 * 
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 * 
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 * 
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 * 
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 * 
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

/*
 * rpc_sample.c, Sample client-server code outputter for the RPC protocol compiler
 * Copyright (C) 1987, Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <string.h>
#include "rpc_parse.h"
#include "rpc_util.h"


static char RQSTP[] = "rqstp";

void printarglist();

void
write_sample_svc(def)
     definition *def;
{

	if (def->def_kind != DEF_PROGRAM) 
	  return;
	write_sample_server(def);
}


int
write_sample_clnt(def)
     definition *def;
{
        version_list *vp;
	int count = 0;

	if (def->def_kind != DEF_PROGRAM) 
	  return(0);
	/* generate sample code for each version */
	for (vp = def->def.pr.versions; vp != NULL; vp = vp->next) {
	  write_sample_client(def->def_name, vp);
	  ++count;
	}
	return(count);
}


static
write_sample_client(program_name, vp)
     char *program_name;
     version_list *vp;
{
	proc_list *proc;
	int i;
	decl_list *l;

	f_print(fout, "\n\nvoid\n");
	pvname(program_name, vp->vers_num);
	if(Cflag)
		f_print(fout,"(char *host)\n{\n");
	else
		f_print(fout, "(host)\n\tchar *host;\n{\n");
	f_print(fout, "\tCLIENT *clnt;\n");

	i = 0;
	for (proc = vp->procs; proc != NULL; proc = proc->next) {
		f_print(fout, "\t");
		ptype(proc->res_prefix, proc->res_type, 1);
		f_print(fout, " *result_%d;\n",++i);
		/* print out declarations for arguments */
		if(proc->arg_num < 2 && !newstyle) {
			f_print(fout, "\t");
			if(!streq(proc->args.decls->decl.type, "void"))
				ptype(proc->args.decls->decl.prefix, 
				      proc->args.decls->decl.type, 1);
			else
				f_print(fout, "char * "); /* cannot have "void" type */
			f_print(fout, " ");
			pvname(proc->proc_name, vp->vers_num);
			f_print(fout, "_arg;\n");
		} else if (!streq(proc->args.decls->decl.type, "void")) {
			for (l = proc->args.decls; l != NULL; l = l->next) {
				f_print(fout, "\t");
				ptype(l->decl.prefix, l->decl.type, 1);
				if (strcmp(l->decl.type,"string") == 1)
				    f_print(fout, " ");
				pvname(proc->proc_name, vp->vers_num);
				f_print(fout, "_%s;\n", l->decl.name);
			}
		}
	}

	/* generate creation of client handle */
	f_print(fout, "\n#ifndef\tDEBUG\n");
	f_print(fout, "\tclnt = clnt_create(host, %s, %s, \"%s\");\n", 
		program_name, vp->vers_name, tirpcflag? "netpath" : "udp");
	f_print(fout, "\tif (clnt == (CLIENT *) NULL) {\n");
	f_print(fout, "\t\tclnt_pcreateerror(host);\n");
	f_print(fout, "\t\texit(1);\n\t}\n");
	f_print(fout, "#endif\t/* DEBUG */\n\n");

	/* generate calls to procedures */
	i = 0;
	for (proc = vp->procs; proc != NULL; proc = proc->next) {
		f_print(fout, "\tresult_%d = ",++i);      
		pvname(proc->proc_name, vp->vers_num);
		if (proc->arg_num < 2 && !newstyle) {
			f_print(fout, "(");
			if(streq(proc->args.decls->decl.type, "void")) 
				/* cast to void * */
				f_print(fout, "(void *)");
			f_print(fout, "&");
			pvname(proc->proc_name, vp->vers_num);
			f_print(fout, "_arg, clnt);\n");
		} else if (streq(proc->args.decls->decl.type, "void")) {
			f_print(fout, "(clnt);\n");
		}
		else {
			f_print(fout, "(");
			for (l = proc->args.decls;  l != NULL; l = l->next) {
				pvname(proc->proc_name, vp->vers_num);
				f_print(fout, "_%s, ", l->decl.name);
			}
			f_print(fout, "clnt);\n");
		}
		f_print(fout, "\tif (result_%d == (", i);
		ptype(proc->res_prefix, proc->res_type, 1);
		f_print(fout, "*) NULL) {\n");
		f_print(fout, "\t\tclnt_perror(clnt, \"call failed\");\n");
		f_print(fout, "\t}\n");
	}

	f_print(fout, "#ifndef\tDEBUG\n");
	f_print(fout, "\tclnt_destroy(clnt);\n");
	f_print(fout, "#endif\t/* DEBUG */\n");
	f_print(fout, "}\n");
}

static
write_sample_server(def)
	definition *def;
{
	version_list *vp;
	proc_list *proc;

	for (vp = def->def.pr.versions; vp != NULL; vp = vp->next) {
		for (proc = vp->procs; proc != NULL; proc = proc->next) {
			f_print(fout, "\n");
/*			if(Cflag)
			  f_print(fout, "extern \"C\"{\n");
*/
			return_type(proc);
			f_print(fout, "*\n");
			if(Cflag)
			  pvname_svc(proc->proc_name, vp->vers_num);
			else
			  pvname(proc->proc_name, vp->vers_num);
			printarglist(proc, RQSTP, "struct svc_req *");

			f_print(fout, "{\n");
			f_print(fout, "\tstatic ");
			if(!streq(proc->res_type, "void"))
			  return_type(proc);
			else
			  f_print(fout, "char *");  /* cannot have void type */
			f_print(fout, " result;\n", proc->res_type);
			f_print(fout, 
				"\n\t/*\n\t * insert server code here\n\t */\n\n");
			if(!streq(proc->res_type, "void"))
			  f_print(fout, "\treturn (&result);\n}\n");
			else  /* cast back to void * */
			  f_print(fout, "\treturn((void *) &result);\n}\n"); 
/*			if(Cflag)
			  f_print(fout, "};\n");
*/

		} 		
	}
}



static 
return_type(plist)
	proc_list *plist;
{
  ptype(plist->res_prefix, plist->res_type, 1);
}

add_sample_msg()
{
	f_print(fout, "/*\n");
	f_print(fout, " * This is sample code generated by rpcgen.\n");
	f_print(fout, " * These are only templates and you can use them\n");
	f_print(fout, " * as a guideline for developing your own functions.\n");
	f_print(fout, " */\n\n");
}

void
write_sample_clnt_main()
{
	list *l;
	definition *def;
	version_list *vp;

	f_print(fout, "\n\n");
	if(Cflag)
		f_print(fout,"main(int argc, char *argv[])\n{\n");
	else
		f_print(fout, "main(argc, argv)\n\tint argc;\n\tchar *argv[];\n{\n");

	f_print(fout, "\tchar *host;");
	f_print(fout, "\n\n\tif (argc < 2) {");
	f_print(fout, "\n\t\tprintf(\"usage:  %%s server_host\\n\", argv[0]);\n");
	f_print(fout, "\t\texit(1);\n\t}");
	f_print(fout, "\n\thost = argv[1];\n");

	for (l = defined; l != NULL; l = l->next) {
		def = l->val;
		if (def->def_kind != DEF_PROGRAM) {
			continue;
		}
		for (vp = def->def.pr.versions; vp != NULL; vp = vp->next) {
		        f_print(fout, "\t");
			pvname(def->def_name, vp->vers_num);
			f_print(fout, "(host);\n");
		}
	}
	f_print(fout, "}\n");
}
