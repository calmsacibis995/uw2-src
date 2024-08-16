/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)rpcgen:rpc_util.h	1.2.9.5"
#ident  "$Header: rpc_util.h 1.3 91/07/01 $"

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
*       (c) 1986,1987,1988,1989,1990  Sun Microsystems, Inc                     
*       (c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.                      
*       (c) 1990,1991,1992  UNIX System Laboratories, Inc
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
 * rpc_util.h, Useful definitions for the RPC protocol compiler 
 * Copyright (C) 1987, Sun Microsystems, Inc.
 */

#include <stdlib.h>

#define alloc(size)		malloc((unsigned)(size))
#define ALLOC(object)   (object *) malloc(sizeof(object))

#define s_print	(void) sprintf
#define f_print (void) fprintf

struct list {
	definition *val;
	struct list *next;
};
typedef struct list list;

struct xdrfunc {
	char *name;
	int pointerp;
	struct xdrfunc *next;
};
typedef struct xdrfunc xdrfunc;

struct commandline {
	int cflag;		/* xdr C routines */
	int hflag;		/* header file */
	int lflag;		/* client side stubs */
	int mflag;		/* server side stubs */
	int nflag;		/* netid flag */
	int sflag;		/* server stubs for the given transport */
	int tflag;		/* dispatch Table file */
	int Ssflag;		/* produce server sample code */
	int Scflag;		/* produce client sample code */
	int makefileflag;       /* Generate a template Makefile */
	char *infile;		/* input module name */
	char *outfile;		/* output module name */
};

#define PUT 1
#define GET 2

/*
 * Global variables 
 */
#define MAXLINESIZE 1024
extern char curline[MAXLINESIZE];
extern char *where;
extern int linenum;

extern char *infilename;
extern FILE *fout;
extern FILE *fin;

extern list *defined;


extern bas_type *typ_list_h;
extern bas_type *typ_list_t;
extern xdrfunc *xdrfunc_head, *xdrfunc_tail;

/*
 * All the option flags
 */
extern int inetdflag;
extern int pmflag;   
extern int tblflag;
extern int logflag;
extern int newstyle;
extern int Cflag;     /* ANSI-C/C++ flag */
extern int CCflag;     /* C++ flag */
extern int tirpcflag; /* flag for generating tirpc code */
extern int inline; /* if this is 0, then do not generate inline code */

/*
 * Other flags related with inetd jumpstart.
 */
extern int indefinitewait;
extern int exitnow;
extern int timerflag;

extern int nonfatalerrors;

/*
 * rpc_util routines 
 */
void storeval();

#define STOREVAL(list,item)	\
	storeval(list,item)

definition *findval();

#define FINDVAL(list,item,finder) \
	findval(list, item, finder)

char *fixtype();
char *stringfix();
char *locase();
void pvname_svc();
void pvname();
void ptype();
int isvectordef();
int streq();
void error();
void expected1();
void expected2();
void expected3();
void tabify();
void record_open();
bas_type *find_type();
/*
 * rpc_cout routines 
 */
void cprint();
void emit();

/*
 * rpc_hout routines 
 */
void print_datadef();
void print_funcdef();
void print_xdr_func_def();

/*
 * rpc_svcout routines 
 */
void write_most();
void write_register();
void write_rest();
void write_programs();
void write_svc_aux();
void write_inetd_register();
void write_netid_register();
void write_nettype_register();
/*
 * rpc_clntout routines
 */
void write_stubs();

/*
 * rpc_tblout routines
 */
void write_tables();
