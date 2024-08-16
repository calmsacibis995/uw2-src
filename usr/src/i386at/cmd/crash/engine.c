/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:i386at/cmd/crash/engine.c	1.1"

#include <sys/types.h>
#include <sys/param.h>
#include <a.out.h>
#include <signal.h>
#include <stdio.h>
#include <memory.h>
#include <sys/fs/s5dir.h>
#include <sys/file.h>
#include <sys/user.h>
#include <sys/immu.h>
#include <sys/var.h>
#include <sys/ioctl.h>
#include <sys/sysi86.h>
#include <sys/plocal.h>
#include <sys/vmparam.h>
#include <sys/disp_p.h>
#include "crash.h"

extern int Cur_eng;
extern pte_t *KPD;

int
getengine()
{
	int c;
	struct syment *sp;
	int reset = 0;
	int proc = Cur_proc;
	long value=(-1);
	engine_t * e;
	engine_t engine;
	long new_kpd;

	optind = 1;
	while((c = getopt(argcnt,args,"w:s:r")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 's' :	proc = setproc();
					break;
			case 'r' :	reset = 1;
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(args[optind]) {
		if((value = (long)strcon(args[optind],'d')) == -1)
			error("illegal value\n");
	}

	if( value == (-1) ) {
		pr_context();
		return;
	}

	if( (e = id_to_eng(value)) == (engine_t*) (-1) ) {
		fprintf(fp,"illegal engine number\n");
		return;
	}

	readmem((unsigned long)e,1,-1,(char *)&engine,sizeof(struct engine),
						 "engine structure");
	if( engine.e_flags & E_OFFLINE ) {

		fprintf(fp,"engine %d is offline\n",value);
		return;
	}
	new_kpd =  cr_vtop(&engine.e_local->pp_kl1pt[0][0]);

	KPD = new_kpd;
	get_context();
	pr_context();
}

int
getplocal()
{
	int c;
	extern  struct syment *L;
	int reset = 0;
	int proc = Cur_proc;
	long value=(-1);
	struct plocal l;
	char spin;
	

	optind = 1;
	while((c = getopt(argcnt,args,"w:s:r")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(args[optind]) {
		if((value = (long)strcon(args[optind],'d')) == -1)
			error("illegal value\n");
	}

	
	readmem(L->n_value,1,-1,(char *)&l,sizeof(struct plocal),
						 "plocal structure");

	fprintf(fp,"Plocal  structure for engine %d\n\n",Cur_eng);

	fprintf(fp,"engine number:\t%d\t",l.eng_num);
	fprintf(fp,"engine mask:\t0x%x\n",l.eng_mask);
	fprintf(fp,"fpuon:\t0x%x\t",l.fpuon);
	fprintf(fp,"fpuoff:\t0x%x\n",l.fpuoff);
	fprintf(fp,"usingfpu:\t0x%x\t",l.usingfpu);
	fprintf(fp,"cpuspeed:\t0x%x\n",l.cpu_speed);
#ifdef notdef
	fprintf(fp,"eventflags:\t");
	if( l.eventflags | EVT_RUNRUN )
		fprintf(fp,"EVT_RUNRUN");
	if( l.eventflags | EVT_KPRUNRUN )
		fprintf(fp," | EVT_KPRUNRUN");
#endif
	fprintf(fp,"\n");
	fprintf(fp,"one_sec:\t0x%x\t",l.one_sec);
	fprintf(fp,"holdfastlock:\t%s\n",
			(l.holdfastlock==B_TRUE)?" B_TRUE":"B_FALSE");

	if( l.fspin )
		readmem(l.fspin,1,-1,(char *)&spin,sizeof(fspin_t),"fspin_t");
	if( l.fspin )
		fprintf(fp,"fspin:\t0x%x \t*(l.fspin) = 0x%x \n",l.fspin,spin);
	else
		fprintf(fp,"fspin:\t0x%x\n",l.fspin);
}
