/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:common/cmd/crash/fpriv.c	1.2"
#ident	"$Header: fpriv.c 1.1 91/07/23 $"


/*
 * This file contains code for the crash function: filepriv, fprv.
 */

#include	<syms.h>
#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/sysmacros.h>
#include	<priv.h>
#include	"crash.h"
#include	"sys/sum.h"
#include	"sys/lpm.h"


struct	syment	*Privf;
void	SUM_pr_privs();
void	LPM_pr_privs();

/* get arguments for filepriv function */
int
getfpriv()
{
	long	addr;
	int symbolic = 0;
	int c;
	void	SUM_prprvtbl(), LPM_prprvtbl();

	if (!Privf)
		if ((!(Privf = symsrch("pm_SUMktab")))
		 && (!(Privf = symsrch("pm_LPMktab"))))
			error("kernel privilege table not found in symbol table\n");

	optind = 1;
	while((c = getopt(argcnt,args,"nw:")) !=EOF) {
		switch(c) {
		case 'n' :	symbolic = 1;
				break;
		case 'w' :	redirect();
				break;
		default  :	longjmp(syn,0);
		}
	}

	/* Get the address of the first mounted device entry. */
	readmem(Privf->n_value, 1, -1, (char *)&addr, sizeof(addr),
		"address of kernel privilege table");

	if (symsrch("privid"))
		SUM_prprvtbl(addr, symbolic);
	else
		LPM_prprvtbl(addr, symbolic);
}

/* print kernel privilege table */
void
SUM_prprvtbl(daddr, symb)
long	daddr;
int	symb;
{
	SUMktab_t	devtbl;
	SUMftab_t	flist;
	SUMdtab_t	fstbl;
	long		fsaddr,
			laddr;

	while (daddr) {			/* list of mounted devices loop */
		readmem(daddr, 1, -1, (char *)&devtbl, sizeof(devtbl),
			"kernel privilege table (mounted devices)");
		/* set ``fsaddr'' to the address of the file system list. */
		fsaddr = (long)devtbl.list;

		/*
		 * if there's another device in the list then
		 * set ``daddr'' to it.  Otherwise, it's at the
		 * end of the devices list.
		*/
		if (devtbl.next)
			daddr = (long)devtbl.next;
		else
			daddr = 0;
		while (fsaddr) {	/* list of file systems loop */
			readmem(fsaddr, 1, -1, (char *)&fstbl, sizeof(fstbl),
				"kernel privilege table (file systems)");
			/*
			 * set ``laddr'' to the address of the privilege file
			 * list.
			*/
			laddr = (long)fstbl.list;
			/*
			 * if there's another file system in the list then
			 * set ``fsaddr'' to it.  Otherwise, it's at the
			 * end of the file systems list.
			*/
			if (fstbl.next)
				fsaddr = (long)fstbl.next;
			else
				fsaddr = 0;
			while (laddr) {		/* list of files loop */
				readmem(laddr, 1, -1, (char *)&flist,
					sizeof(flist),
					"kernel privilege table (files)");
				fprintf(fp, "maj,min device: %4u,%-5u",
					getemajor(devtbl.dev),
					geteminor(devtbl.dev));
				fprintf(fp,"\tfile id: %.8x",flist.nodeid);
				fprintf(fp,"\tvalidity: %.8x\n",
					flist.validity);
				SUM_pr_privs(flist, symb);
				/*
				 * If there's another file in the list then 
				 * set ``laddr'' to that address.  Otherwise,
				 * indicate it's at the end.
				 */
				if (flist.next)
					laddr = (long)flist.next;
				else
					laddr = 0;
			}
		}
	}
}


static	void
SUM_pr_privs(lst, symb)
SUMftab_t	lst;
register int	symb;
{
	extern	void	prt_symbprvs();

	if (symb) {
		prt_symbprvs("fixed: ", lst.fixpriv);
	} else {
		fprintf(fp, "fixed: %.8x", lst.fixpriv);
		fprintf(fp, "\n");
	}
}


/* print kernel privilege table */
void
LPM_prprvtbl(daddr, symb)
long	daddr;
int	symb;
{
	LPMktab_t	devtbl;
	LPMftab_t	flist;
	LPMdtab_t	fstbl;
	long		fsaddr,
			laddr;

	while (daddr) {		/* list of mounted devices loop */
		readmem(daddr, 1, -1, (char *)&devtbl, sizeof(devtbl),
			"kernel privilege table (mounted devices)");
		/* set ``fsaddr'' to the address of the file system list. */
		fsaddr = (long)devtbl.list;

		/*
		 * if there's another device in the list then
		 * set ``daddr'' to it.  Otherwise, it's at the
		 * end of the devices list.
		 */
		if (devtbl.next)
			daddr = (long)devtbl.next;
		else
			daddr = 0;
		while (fsaddr) {	/* list of file systems loop */
			readmem(fsaddr, 1, -1, (char *)&fstbl, sizeof(fstbl),
				"kernel privilege table (file systems)");
			/*
			 * set ``laddr'' to the address of the privilege file
			 * list.
			 */
			laddr = (long)fstbl.list;
			/*
			 * if there's another file system in the list then
			 * set ``fsaddr'' to it.  Otherwise, it's at the
			 * end of the file systems list.
			 */
			if (fstbl.next)
				fsaddr = (long)fstbl.next;
			else
				fsaddr = 0;
			while (laddr) {		/* list of files loop */
				readmem(laddr, 1, -1, (char *)&flist,
					sizeof(flist),
					"kernel privilege table (files)");
				fprintf(fp, "maj,min device: %4u,%-5u",
					getemajor(devtbl.dev),
					geteminor(devtbl.dev));
				fprintf(fp,"\tfile id: %.8x",flist.nodeid);
				fprintf(fp,"\tvalidity: %.8x\n",
					flist.validity);
				LPM_pr_privs(flist, symb);
				/*
				 * If there's another file in the list then 
				 * set ``laddr'' to that address.  Otherwise,
				 * indicate it's at the end.
				 */
				if (flist.next)
					laddr = (long)flist.next;
				else
					laddr = 0;
			}
		}
	}
}


static	void
LPM_pr_privs(lst, symb)
LPMftab_t	lst;
register int	symb;
{
	extern	void	prt_symbprvs();

	if (symb) {
		prt_symbprvs("fixed: ", lst.fixpriv);
		prt_symbprvs("inher: ", lst.inhpriv);
	} else {
		fprintf(fp, "fixed: %.8x", lst.fixpriv);
		fprintf(fp, "\tinher: %.8x", lst.inhpriv);
		fprintf(fp, "\n");
	}
}
