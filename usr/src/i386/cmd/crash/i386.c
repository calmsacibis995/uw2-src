/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:i386/cmd/crash/i386.c	1.2.1.6"
#ident "$Header: i386.c 1.1 91/07/23 $"

/*
 * This file contains code for the crash functions: idt, ldt, gdt
 */

#include <sys/param.h>
#include <a.out.h>
#include <stdio.h>
#include <signal.h>
#include <ctype.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/fs/s5dir.h>
#include <sys/immu.h>
#include <sys/tss.h>
#include <sys/seg.h>
#include <sys/user.h>
#include <sys/var.h>
#include <sys/acct.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/lock.h>
#include <sys/reg.h>
#include <sys/vmparam.h>
#include "crash.h"

#define HEAD1 "SLOT     BASE/SEL LIM/OFF  TYPE       DPL  ACESSBITS\n"
#define HEAD2 "SLOT     SELECTOR OFFSET   TYPE       DPL  ACESSBITS\n"

extern	struct user *ubp;
extern  struct proc procbuf;

extern char *malloc();

/* get arguments for ldt function */
int getldt()
{
	int slot = Cur_proc;
	int all = 0;
	int c;
	int first = 0;
	int last;
	struct desctab_info di;

	readmem((long)ubp->u_dt_infop[DT_LDT], 1, -1,
		&di, sizeof di, "desctab info for ldt");
	last = di.di_size - 1;

	optind = 1;
	while((c = getopt(argcnt,args,"ew:")) !=EOF) {
		switch(c) {
			case 'e' :	all = 1;
					break;
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(args[optind]) {
		if((slot = strcon(args[optind],'d')) == -1)
			error("\n");
		if((slot < 0) || (slot >= vbuf.v_proc)) 
			error("proc %d is out of range\n",slot);
		optind++;
	}
	if(args[optind]) {
		if((first= strcon(args[optind],'d')) == -1)
			error("\n");
		if((first < 0) || (first > last))
			error("entry %d is out of range\n",slot);
		last=first;
		optind++;
	}
	if(args[optind]) {
		if((last = strcon(args[optind],'d')) == -1)
			error("\n");
		last=first+last-1;
		if (last>=di.di_size) last=di.di_size-1;
	}
	if (first==last) all=1;
	prdt("LDT",&di,all,slot,first,last);
}

/* get arguments for idt function */
int getidt() {

	int all = 0;
	int c;
	int first=0;
	int last=IDTSZ-1;
	optind = 1;
	while ((c = getopt(argcnt,args,"ew:")) !=EOF) {
		switch(c) {
			case 'e' :	all = 1;
					break;
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(args[optind]) {
		if ((first= strcon(args[optind],'d')) == -1)
			error("\n");
		if ((first < 0) || (first >= IDTSZ))
			error("entry %d is out of range\n",first);
		last=first;
		optind++;
	}
	if(args[optind]) {
		if((last = strcon(args[optind],'d')) == -1)
			error("\n");
		last=first+last-1;
		if (last>=IDTSZ) last=IDTSZ-1;
	}
	if (first==last) all=1;
	pridt(all,first,last);
}

/* print interrupt descriptor table */
int pridt(all,first,last)
int all;
int first,last;
{
	int i;
	struct gate_desc *idtp;
	struct gate_desc idt[IDTSZ];
	extern struct syment *L;

	readmem((long)L->n_value + offsetof(struct plocal, idtp),1,-1,
			(char *)&idtp,sizeof idtp,"IDT pointer");
	readmem((long)idtp,1,-1,(char *)idt,sizeof idt,"IDT");
	fprintf(fp,"iAPX386 IDT\n");
	fprintf(fp,HEAD2);
	for (i=first;i<=last;i++)
		prdescr(i,(struct segment_desc *)&idt[i],all);
}

/* get arguments for gdt function */
int getgdt()
{
	int all = 0;
	int c;
	int first=0;
	int last;
	struct desctab_info di;

	readmem((long)ubp->u_dt_infop[DT_GDT], 1, -1,
		&di, sizeof di, "desctab info for gdt");
	last = di.di_size - 1;

	optind = 1;
	while((c = getopt(argcnt,args,"ew:")) !=EOF) {
		switch(c) {
			case 'e' :	all = 1;
					break;
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(args[optind]) {
		if((first= strcon(args[optind],'d')) == -1)
			error("\n");
		if((first < 0) || (first >= last))
			error("entry %d is out of range\n",first);
		last=first;
		optind++;
	}
	if(args[optind]) {
		if((last = strcon(args[optind],'d')) == -1)
			error("\n");
		last=first+last-1;
		if (last>=di.di_size) last=di.di_size-1;
	}
	if (first==last) all=1;
	prdt("GDT",&di,all,first,last);
}

/* print global or local descriptor table */
int prdt(tabname,dip,all,slot,first,last)
char *tabname;
struct desctab_info *dip;
int all,slot;
int first,last;
{
	struct segment_desc dtent;
	int i;

	fprintf(fp,"iAPX386 %s for process %d\n",tabname,slot);
	fprintf(fp,HEAD1);
	for (i=first;i<=last;i++) {
		readmem(dip->di_table + i, 1, slot, &dtent, sizeof dtent,
			tabname);
		prdescr(i,&dtent,all);
	}
}

prdescr(i,t,all)
int i;
struct segment_desc *t;
int all;
{
	int	selec=0;	/* true if selector, false if base */
	int	gran4;		/* true if granularity = 4K */
	char	acess[100];	/* Description of Accessbytes */
	long	base;		/* Base or Selector */
	long	offset;		/* Offset or Limit */
	char	*typ;		/* Type */
	struct gate_desc *gd = (struct gate_desc *)t;

	if (!(all | (SD_GET_ACC1(t) & 0x80))) return(0);

	base = SD_GET_BASE(t);
	offset = SD_GET_LIMIT(t);
	gran4 = SD_GET_ACC2(t) & 0x8;
	sprintf(acess,"  %d ",(SD_GET_ACC1(t) >> 5) & 3);

	if (!(SD_GET_ACC1(t) & 0x10)) {
	/* Segment Descriptor */
		selec=1;
		switch (SD_GET_ACC1(t)&0xf) {
		case 0:	typ="SYS 0 ?  "; selec=0; break;
		case 3:
		case 1:	typ="TSS286   "; selec=0; break;
		case 2: typ="LDT      "; selec=0; break;
		case 4: typ="CGATE    ";
			sprintf(acess,"%s CNT=%d",acess,gd->gd_arg_count);
			break;
		case 5: typ="TASK GATE";
			sprintf(acess,"%s CNT=%d",acess,gd->gd_arg_count);
			break;
		case 6: typ="IGATE286 ";
			sprintf(acess,"%s CNT=%d",acess,gd->gd_arg_count);
			break;
		case 7: typ="TGATE286 ";
			sprintf(acess,"%s CNT=%d",acess,gd->gd_arg_count);
			break;
		case 9:
		case 11:typ="TSS386   "; selec=0; break;
		case 12:typ="CGATE386 ";
gate386:
			offset = gd->gd_offset_low + (gd->gd_offset_high << 16);
			gran4=0;
			sprintf(acess,"%s CNT=%d",acess,gd->gd_arg_count);
			break;
		case 14:typ="IGATE386 ";
			goto gate386;
		case 15:typ="TGATE386 ";
			goto gate386;
		default:typ="SYS???   ";
		}
	} else if (SD_GET_ACC1(t) & 0x8) {
	/* executable Segment */
		typ="XSEG     ";
		sprintf(acess,"%s%s%s%s%s",acess,
			SD_GET_ACC1(t) & 1 ? " ACCS'D":"",
			SD_GET_ACC1(t) & 2 ? " R&X":" XONLY",
			SD_GET_ACC1(t) & 4 ? " CONF":"",
			SD_GET_ACC2(t) & 4 ? " DFLT":"");
	} else {
	/* Data Segment */
		typ="DSEG     ";
		sprintf(acess,"%s%s%s%s%s",acess,
			SD_GET_ACC1(t) & 1 ? " ACCS'D":"",
			SD_GET_ACC1(t) & 2 ? " R&W":" RONLY",
			SD_GET_ACC1(t) & 4 ? " EXDOWN":"",
			SD_GET_ACC2(t) & 4 ? " BIG ":"");
	}

	fprintf(fp,"%4d     ",i);
	if (selec) fprintf(fp,"    %04x",base&0xffff);
	else fprintf(fp,"%08x",base);
	if (gran4) fprintf(fp," %08x ",offset<<12 );
	else fprintf(fp," %08x ",offset);
/*	fprintf(stderr," %01x%02x ",SD_GET_ACC2(t),SD_GET_ACC1(t)); */
	fprintf(fp,"%s  %s%s%s%s\n",typ,acess,gran4 ? " G4096":"",
			SD_GET_ACC2(t) & 1 ? " AVL":"",
			SD_GET_ACC1(t) & 0x80 ? "":"** nonpres **");
}


/* Test command */
int debugmode=0;

int gettest()
{
	int p1;
	char c;

	optind = 1;
	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(args[optind]) {
		if((p1 = strcon(args[optind],'d')) == -1) error("\n");
		debugmode=p1;
	}
	fprintf(fp,"Debug Mode %d\n",debugmode);
}

/* get arguments for panic function */
int getpanic()
{
	int slot = Cur_proc;
	int lwp =  Cur_lwp;
	char c;

	optind = 1;
	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(args[optind]) {
		if((slot = strcon(args[optind],'d')) == -1)
			error("\n");
		if ((slot < 0) || (slot >= vbuf.v_proc)) 
			error("proc %d is out of range\n",slot);
		optind++;
	}
	if(args[optind]) {
		if((lwp = strcon(args[optind],'d')) == -1)
			error("\n");
		optind++;
	}
	prpanic(slot,lwp);
}

prpanic(slot,lwp)
int	slot;
{
	extern  struct syment *Panic_data;
	struct	syment *Putbuf,*Putbufsz,*Save_r0ptr;
	kcontext_t kc;
	long	panicstr;
	char	panicbuf[200];
	char	*putbuf;
	int	putbufsz;
	unsigned long save_r0ptr;
	unsigned long stkhi,stklo;
	struct panic_data panic_data;

	if (!(Putbuf=symsrch("putbuf")))
		error("symbol putbuf not found\n");
	if (!(Putbufsz=symsrch("putbufsz")))
		error("symbol putbufsz not found\n");

	readmem((long)Putbufsz->n_value,1,-1,&putbufsz,sizeof(int),"putbufsz");
	if ((putbuf = malloc(putbufsz+1)) == NULL)
		prerrmes("Insufficient memory for putbuf");
	else {
		register char *cp, *ce;
		readmem((long)Putbuf->n_value,1,-1,putbuf,putbufsz,"putbuf");
		putbuf[putbufsz] = '\0';
		fprintf(fp,"System Messages:\n\n%s\n\n",putbuf);
		fprintf(fp,"System Messages:\n\n");
		for(ce = &putbuf[putbufsz-1]; ce >= putbuf; ce--)
			if(*ce != '\0')
				break;
		for(cp = putbuf; cp < ce; cp++)
			if(isprint(*cp) || isspace(*cp) || *cp == '\n')
				fprintf(fp,"%c",*cp);
			else
				fprintf(fp,"[0x%x]",*cp&0xff);
		fprintf(fp,"\n\n");
		free(putbuf);
	}
	fprintf(fp,"===============END OF KERNEL MESSAGES===============\n");

	readmem((long)Panic_data->n_value,1,-1,&panic_data,
				sizeof(struct panic_data), "panic_data");
	if( panic_data.pd_rp ) {
	readmem((long)panic_data.pd_rp,1,-1,&kc, 
				sizeof(kcontext_t), "panic_kcontext");

	fprintf(fp,"PANIC DATA:\n");
	fprintf(fp,"Panic engine_t *= %d  engine(%d) \n",panic_data.pd_engine,
					eng_to_id(panic_data.pd_engine));
	fprintf(fp,"Saved kcontext_t * = %x\n",panic_data.pd_rp);
	fprintf(fp,"Saved kcontext_t * = %x\n\n",panic_data.pd_rp);

	fprintf(fp,"ESP= %08x  EBX= %08x  EBP= %08x  ESI= %08x\n",
			kc.kctx_esp, kc.kctx_ebx, kc.kctx_ebp, kc.kctx_esi);

	fprintf(fp,"EDI= %08x  EIP= %08x  EAX= %08x  ECX= %08x\n",
			kc.kctx_edi, kc.kctx_eip, kc.kctx_eax, kc.kctx_ecx);

	fprintf(fp,"EDX= %08x  FS= %08x  GS= %08x\n",
			kc.kctx_edx, kc.kctx_fs, kc.kctx_gs);
	} else {

		fprintf(fp,"panic_data structure empty\n");
	}
	prktrace(slot, lwp, 0, 0);
}
