/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:common/cmd/crash/stream.c	1.3"
#ident	"$Header: stream.c 1.1 91/07/23 $"

/*
 * This file contains code for the crash functions:  stream, queue, 
 * strstat, linkblk, qrun.
 */

#include <sys/param.h>
#include <a.out.h>
#include <stdio.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/var.h>
#include <sys/proc.h>
#include <sys/fstyp.h>
#include <sys/fsid.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/vnode.h>
#include <sys/list.h>

#ifdef notdef
#include <sys/rf_messg.h>
#include <sys/rf_comm.h>
#endif

#include <sys/vfs.h>
#define _KERNEL
#include <sys/poll.h>
#undef _KERNEL
#include <sys/stream.h>
#define DEBUG
#include <sys/strsubr.h>
#undef DEBUG
#include <sys/strstat.h>
#include <sys/stropts.h>
#include "crash.h"

static struct syment *Strinfop, *Strst, *Qsvc;
extern struct syment *Vnode, *Streams;
struct dbinfo *prdblk();
struct dbinfo *realprdblk();
struct mbinfo *prmess();
struct mbinfo *realprmess();
struct shinfo *prstream();
struct linkinfo *prlinkblk();

/*
 * Data Structure added to display the streams in a proper way
 */

struct str {
        queue_t *qup;
        queue_t *qnxt;
        char    modn[20];
        struct str *s_next;
        int     qpair_f;        /* flag for the queue pair */
};

#define MXSTR  500              /* Max entries in the table
                                 * Should always be greater than
                                 * Max number of streams that can
                                 * be open at any time
                                 */

struct str *strtb[MXSTR];       /* Table of pointers to str structs */
int st_index;


/* get arguments for stream function */
int
getstream()
{
	int all = 0;
	int full = 0;
	int phys = 0;
	long addr = -1;
	int c;
	register int i;
	struct shinfo *sp;
	struct strinfo strinfo[NDYNAMIC];
 	char *heading = " ADDRESS      WRQ     IOCB    VNODE  PUSHCNT  RERR/WERR FLAG\n";

	optind = 1;
	while((c = getopt(argcnt,args,"efpw:")) !=EOF) {
		switch(c) {
			case 'e' :	all = 1;
					break;
			case 'f' :	full = 1;
					break;
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 1;
					break;
			default  :	longjmp(syn,0);
		}
	}
	if (!Strinfop)
		if(!(Strinfop = symsrch("Strinfo")))
			error("Strinfo not found in symbol table\n");
	readmem((long)Strinfop->n_value,1,-1,(char *)&strinfo,
		sizeof(strinfo),"Strinfo");
	fprintf(fp,"STREAM TABLE SIZE = %d\n",strinfo[DYN_STREAM].st_cnt);
	if(!full)
		fprintf(fp,"%s",heading);
	if(args[optind]) {
		all = 1;
		do {
			if((addr = strcon(args[optind++], 'h')) == -1)
				error("\n");
			(void) prstream(all,full,addr,phys,heading);
		}while(args[++optind]);
	}
	else {
		sp = (struct shinfo *) strinfo[DYN_STREAM].st_head;
		while(sp) {
			sp = prstream(all,full,sp,phys,heading);
		}
	}
}

/* print streams table */
struct shinfo *
prstream(all,full,addr,phys,heading)
int all,full,phys;
long addr;
char *heading;
{
	struct shinfo strm;
	register struct stdata *stp;
	struct strevent evbuf;
	struct strevent *next;
	struct pollhead ph;
	struct polldat *pdp;
	struct polldat pdat;
	struct lwp lwp;
	struct sess sess;
	struct proc proc;

	readmem(addr,1,-1,(char *)&strm,sizeof(strm),"streams table slot");
	stp = (struct stdata *) &strm;
	if (!stp->sd_wrq && !all) 
		return(NULL);
	if(full)
		fprintf(fp,"%s",heading);
	fprintf(fp,"%8x %8x %8x %8x %6d    %d/%d",addr,stp->sd_wrq,stp->sd_strtab,
	    stp->sd_vnode,stp->sd_pushcnt,stp->sd_rerror,stp->sd_werror);
	fprintf(fp,"       %s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
		((stp->sd_flag & IOCWAIT) ? "iocw " : ""),
		((stp->sd_flag & RSLEEP) ? "rslp " : ""),
		((stp->sd_flag & WSLEEP) ? "wslp " : ""),
		((stp->sd_flag & STRPRI) ? "pri " : ""),
		((stp->sd_flag & STRHUP) ? "hup " : ""),
		((stp->sd_flag & STRTOHUP) ? "thup " : ""),
		((stp->sd_flag & STWOPEN) ? "stwo " : ""),
		((stp->sd_flag & STPLEX) ? "plex " : ""),
		((stp->sd_flag & STRISTTY) ? "istty " : ""),
		((stp->sd_flag & RMSGDIS) ? "mdis " : ""),
		((stp->sd_flag & RMSGNODIS) ? "mnds " : ""),
		((stp->sd_flag & STRDERR) ? "rerr " : ""),
		((stp->sd_flag & STWRERR) ? "werr " : ""),
		((stp->sd_flag & STRTIME) ? "sttm " : ""),
		((stp->sd_flag & UPF) ? "upf " : ""),
		((stp->sd_flag & STR3TIME) ? "s3tm " : ""),
		((stp->sd_flag & UPBLOCK) ? "upb " : ""),
		((stp->sd_flag & SNDMREAD) ? "mrd " : ""),
		((stp->sd_flag & OLDNDELAY) ? "ondel " : ""),
		((stp->sd_flag & STRSNDZERO) ? "sndz " : ""),
		((stp->sd_flag & STRTOSTOP) ? "tstp " : ""),
		((stp->sd_flag & RDPROTDAT) ? "pdat " : ""),
		((stp->sd_flag & RDPROTDIS) ? "pdis " : ""),
		((stp->sd_flag & STRMOUNT) ? "mnt " : ""),
		((stp->sd_flag & STRDELIM) ? "delim " : ""),
		((stp->sd_flag & FLUSHWAIT) ? "flshw " : ""),
		((stp->sd_flag & STRLOOP) ? "loop " : ""),
		((stp->sd_flag & STRPOLL) ? "spoll " : ""),
		((stp->sd_flag & STRNOCTTY) ? "noctty " : ""),
		((stp->sd_flag & STRSIGPIPE) ? "spip " : ""));

	if(full) {
		if (stp->sd_sessp)
			readmem(stp->sd_sessp,1,-1,(char *)&sess,sizeof(sess),"session structure");
		fprintf(fp,"\t     SID     PGID   IOCBLK    IOCID  IOCWAIT\n");
		fprintf(fp,"\t%8d %8d %8x %8d %8d\n",
			(stp->sd_sessp) ? readpid(sess.s_sidp) : 0,
			readpid(stp->sd_pgidp),
			stp->sd_iocblk,stp->sd_iocid,stp->sd_iocwait);
		fprintf(fp,"\t  WOFF     MARK CLOSTIME\n");
		fprintf(fp,"\t%6d %8x %8d\n",stp->sd_wroff,stp->sd_mark,
		    stp->sd_closetime);
		fprintf(fp,"\tSIGFLAGS:  %s%s%s%s%s%s%s%s%s\n",
			((stp->sd_sigflags & S_INPUT) ? " input" : ""),
			((stp->sd_sigflags & S_HIPRI) ? " hipri" : ""),
			((stp->sd_sigflags & S_OUTPUT) ? " output" : ""),
			((stp->sd_sigflags & S_RDNORM) ? " rdnorm" : ""),
			((stp->sd_sigflags & S_RDBAND) ? " rdband" : ""),
			((stp->sd_sigflags & S_WRBAND) ? " wrband" : ""),
			((stp->sd_sigflags & S_ERROR) ? " err" : ""),
			((stp->sd_sigflags & S_HANGUP) ? " hup" : ""),
			((stp->sd_sigflags & S_MSG) ? " msg" : ""));
		fprintf(fp,"\tSIGLIST:\n");
		next = stp->sd_siglist;
		while(next) {
			readmem((long)next,1,-1,(char *)&evbuf,
				sizeof evbuf,"stream event buffer");
			readmem((long)evbuf.se_lwpp,1,-1,(char *)&lwp,
				sizeof lwp,"lwp");
			readmem((long)lwp.l_procp,1,-1,(char *)&proc,
				sizeof proc,"proc");
			fprintf(fp,"\t\tPROC:  %3d  LWP:  %3d  %s%s%s%s%s%s%s%s%s\n",
				proc_to_slot((long)lwp.l_procp),
				lwp_to_slot((long)evbuf.se_lwpp, &proc),
				((evbuf.se_events & S_INPUT) ? " input" : ""),
				((evbuf.se_events & S_HIPRI) ? " hipri" : ""),
				((evbuf.se_events & S_OUTPUT) ? " output" : ""),
				((evbuf.se_events & S_RDNORM) ? " rdnorm" : ""),
				((evbuf.se_events & S_RDBAND) ? " rdband" : ""),
				((evbuf.se_events & S_WRBAND) ? " wrband" : ""),
				((evbuf.se_events & S_ERROR) ? " err" : ""),
				((evbuf.se_events & S_HANGUP) ? " hup" : ""),
				((evbuf.se_events & S_MSG) ? " msg" : ""));
			next = evbuf.se_next;	
		}
		readmem((long)stp->sd_pollist,1,-1,(char *)&ph,
			sizeof ph,"pollhead");
		fprintf(fp,"\tPOLLFLAGS:  %s%s%s%s%s%s\n",
			((ph.ph_events & POLLIN) ? " in" : ""),
			((ph.ph_events & POLLPRI) ? " pri" : ""),
			((ph.ph_events & POLLOUT) ? " out" : ""),
			((ph.ph_events & POLLRDNORM) ? " rdnorm" : ""),
			((ph.ph_events & POLLRDBAND) ? " rdband" : ""),
			((ph.ph_events & POLLWRBAND) ? " wrband" : ""));
		fprintf(fp,"\tPOLLIST:\n");
		pdp = ph.ph_list;
		while(pdp) {
			readmem((long)pdp,1,-1,(char *)&pdat,
				sizeof pdat,"poll data buffer");
			fprintf(fp,"\t\tFUNC:  %#.8x   ARG:  %#.8x   %s%s%s%s%s%s\n",
				pdat.pd_fn, pdat.pd_arg,
				((pdat.pd_events & POLLIN) ? " in" : ""),
				((pdat.pd_events & POLLPRI) ? " pri" : ""),
				((pdat.pd_events & POLLOUT) ? " out" : ""),
				((pdat.pd_events & POLLRDNORM) ? " rdnorm" : ""),
				((pdat.pd_events & POLLRDBAND) ? " rdband" : ""),
				((pdat.pd_events & POLLWRBAND) ? " wrband" : ""));
			pdp = pdat.pd_next;
		}
		fprintf(fp,"\n");
	}
	return(strm.sh_next);
}

char *qheading = " QUEADDR     INFO     NEXT     LINK      PTR     RCNT FLAG\n";

/* get arguments for queue function */
int
getqueue()
{
	int all = 0;
	int phys = 0;
	long addr = -1;
	int c, j;
	register int i;
	struct queinfo *qip;
	struct queinfo queinfo;
	struct strinfo strinfo[NDYNAMIC];
        struct str *nstrp;
	int full = 0, style = 0;

	optind = 1;
	while((c = getopt(argcnt,args,"efpsw:")) !=EOF) {
		switch(c) {
			case 'e' :	all = 1;
					break;
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 1;
					break;
			case 'f' :	full = 1;
					break;
                        case 's' :      style = 1;
                                        st_index = 0;
                                        break;
			default  :	longjmp(syn,0);
		}
	}
	if (!Strinfop)
		if(!(Strinfop = symsrch("Strinfo")))
			error("Strinfo not found in symbol table\n");
	readmem((long)Strinfop->n_value,1,-1,(char *)&strinfo,
		sizeof(strinfo),"Strinfo");
	fprintf(fp,"QUEUE TABLE SIZE = %d\n",strinfo[DYN_QUEUE].st_cnt);
	if(args[optind]) {
		all = 1;
		if (!full && !style)
			fprintf(fp, qheading);
		do {
			if((addr = strcon(args[optind++], 'h')) == -1)
				error("\n");
			prqueue(all,addr,NULL,NULL,NULL,phys,full,0);
		}while(args[optind]);
	}
	else {
		if (!full && !style)
			fprintf(fp, qheading);
		qip = (struct queinfo *) strinfo[DYN_QUEUE].st_head;
		while(qip) {
			readmem((long)qip,1,-1,&queinfo,sizeof(queinfo),"queue");
			prqueue(all,qip,NULL,&queinfo.qu_rqueue,NULL,phys,full,style);
			prqueue(all,(long)qip+sizeof(queue_t),NULL,&queinfo.qu_wqueue,NULL,phys,full,style);
			qip = queinfo.qu_next;
		}
                /* Print stream table */
                if(style){

        /* Pass 2 */
                        qip = (struct queinfo *) strinfo[DYN_QUEUE].st_head;
                        while(qip) {
                                readmem((long)qip,1,-1,&queinfo,sizeof(queinfo), "queue");

#ifdef _STYPES
                                prqueue(all,qip,queinfo.qu_rqueue.q_eq,&queinfo.qu_rqueue,&queinfo.qu_requeue,phys,full,2);
                                prqueue(all,(long)qip+sizeof(queue_t),queinfo.qu_wqueue.q_eq,&queinfo.qu_wqueue,&queinfo.qu_wequeue,phys,full,2);
#else
                                prqueue(all,qip,NULL,&queinfo.qu_rqueue,NULL,phys,full,2);
                                prqueue(all,(long)qip+sizeof(queue_t),NULL,&queinfo.qu_wqueue,NULL,phys,full,2);
#endif
                                qip = queinfo.qu_next;
                        }
                        fprintf(fp,"STREAMS CONFIGURATION > (write side)\n");
                        i = 0;
                        while(strtb[i] != NULL){
                                nstrp = strtb[i];
                                fprintf(fp,"<STRHEAD> ");
                                while(nstrp != NULL)
                                        if(!nstrp->qpair_f){
                                                fprintf(fp,"%-8s(%8x) > ",nstrp->modn,nstrp->qup);
                                                nstrp = nstrp->s_next;
                                        }
                                fprintf(fp,"\n");
                                i++;
                        }
                }  /* style */
	}
}


/* print queue table */
int
prqueue(all,qaddr,eqaddr,uqaddr,ueqaddr,phys,full,style)
int all,phys,full,style;
long qaddr,eqaddr;
long uqaddr,ueqaddr;
{
	register queue_t *qp;
        register struct qinit *qi;
        register struct module_info *qim;
        char mn[20];

	queue_t q;
        struct qinit q_i;
        struct module_info m_info;
        int found ,i;
        struct str *strp, *nstrp;

	if (uqaddr) {
		qp = (queue_t *)uqaddr;
	} else {
		readmem((long)qaddr,1,-1,&q, sizeof(q),"queue");
		qp = &q;
	}
	if (!(qp->q_flag & QUSE) && !all)
		return;

	if(!style){
		if (full)
			fprintf(fp,qheading);
		fprintf(fp,"%8x ",qaddr);
		fprintf(fp,"%8x ",qp->q_qinfo);
	}
        readmem((long)qp->q_qinfo,1,-1,&q_i,sizeof(q_i),"qinit");
        qi= &q_i;
        if(qi->qi_minfo){
                readmem((long)qi->qi_minfo,1,-1,&m_info,sizeof(m_info),"Module");
                qim = &m_info;
                if (qim->mi_idname == 0)
                        mn[0] = '\0';
                else
                        readmem((long)qim->mi_idname,1,-1,&mn[0],sizeof(mn),"Mod Name");
                if(!style)
                        fprintf(fp,"%-8s ",&mn[0]);
                else {
                        strp = (struct str *)malloc(sizeof(struct str));

                        strp->qup = (queue_t *)qaddr;
                        strp->qnxt = (queue_t *)qp->q_next;
                        strcpy(strp->modn,mn);
                        strp->s_next = NULL;
                        strp->qpair_f = 0;
                        if(((strcmp(mn,"NWstrWriteHead") == 0) || (strcmp(mn,"strwhead") == 0)) && (style ==1)){
                                strtb[st_index] = strp;
                                st_index++;
                                strtb[st_index] = NULL;
                        }
                        else {
                                found = 0;
                                        for(i = 0; i < st_index; i++){
                                                nstrp = strtb[i];
                                                while(nstrp != NULL)
                                                        if(nstrp->qnxt == (queue_t *) qaddr)
{
                                                                nstrp->s_next = strp;
                                                                found = 1;
                                                                break;
                                                        }

                                                        else nstrp = nstrp->s_next;
                                                if (found)
                                                        break;
                                        } /* for loop */
                                if(!found)
                                        free(strp);
                        }
                }
        }
        else{
                if(!style)
                        fprintf(fp,"       - ");
        }

        if(!style){
                if (qp->q_next)
                        fprintf(fp,"%8x ",qp->q_next);
                else fprintf(fp,"       - ");


		if (qp->q_next)
			fprintf(fp,"%8x ",qp->q_next);
		else fprintf(fp,"       - ");

		if (qp->q_link)
			fprintf(fp,"%8x ",qp->q_link);

		else fprintf(fp,"       - ");
		fprintf(fp,"%8x",qp->q_ptr);
		fprintf(fp," %8d ",qp->q_count);
		fprintf(fp,"%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
			((qp->q_svcflag & QENAB) ? "en " : ""),
			((qp->q_svcflag & QSVCBUSY) ? "bu " : ""),
			((qp->q_flag & QWANTR) ? "wr " : ""),
			((qp->q_flag & QWANTW) ? "ww " : ""),
			((qp->q_flag & QFULL) ? "fl " : ""),
			((qp->q_flag & QREADR) ? "rr " : ""),
			((qp->q_flag & QUSE) ? "us " : ""),
			((qp->q_flag & QUP) ? "up " : ""),
			((qp->q_flag & QBACK) ? "bk " : ""),
			((qp->q_flag & QPROCSON) ? "on " : ""),
			((qp->q_flag & QTOENAB) ? "te " : ""),
			((qp->q_flag & QFREEZE) ? "fr " : ""),
			((qp->q_flag & QBOUND) ? "bd " : ""),
			((qp->q_flag & QNOENB) ? "ne " : ""));
		if (!full)
			return;
		fprintf(fp,"\t    HEAD     TAIL     MINP     MAXP     HIWT     LOWT BAND BANDADDR\n");
		if (qp->q_first)
			fprintf(fp,"\t%8x ",qp->q_first);
		else fprintf(fp,"\t       - ");
		if (qp->q_last)
			fprintf(fp,"%8x ",qp->q_last);
		else fprintf(fp,"       - ");
		fprintf(fp,"%8d %8d %8d %8d ",
			qp->q_minpsz,
			qp->q_maxpsz,
			qp->q_hiwat,
			qp->q_lowat);
		fprintf(fp," %3d %8x\n\n",
			qp->q_nband, qp->q_bandp);
	}
        else{ /* style */
        /*Print table on return */
        }
}


/* get arguments for qrun function */
int
getqrun()
{
	int c;
	int value;

	value = -1;
	if(!Qsvc)
		if(!(Qsvc = symsrch("qsvc")))
			error("qsvc not found in symbol table\n");
	optind = 1;
	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if (args[optind]) {
		if ((value = (long) strcon(args[optind],'d')) == -1)
			error("illegal value\n");
	}
	prqrun(value);
}

/* print qrun information */
int
prqrun(value)
int value;
{
	struct qsvc qsvc;
	queue_t que, *q;
	struct plocal l;
	extern struct syment *L;

	readmem((long)Qsvc->n_value,1,-1,(char *)&qsvc,sizeof(qsvc),"qsvc");
	fprintf(fp,"Queue slots scheduled for service (global): ");
	q = qsvc.qs_head;
	while (q) {
		fprintf(fp,"%8x ",q);
		readmem((long)q,1,-1,(char *)&que,
			sizeof que,"scanning queue list");
		q = que.q_link;
	}
	fprintf(fp,"\n");
	readmem(L->n_value,1,-1,(char *)&l, sizeof(struct plocal),
						"plocal structure");
	fprintf(fp,"Queue slots scheduled for service (processor %d): ", l.eng_num);
	q = l.qsvc.qs_head;
	while (q) {
		fprintf(fp,"%8x ",q);
		readmem((long)q,1,-1,(char *)&que,
			sizeof que,"scanning queue list");
		q = que.q_link;
	}
	fprintf(fp,"\n");
}


/* initialization for namelist symbols */
int
streaminit()
{
	static int strinit = 0;

	if(strinit)
		return;
	if(!Strinfop)
		if(!(Strinfop = symsrch("Strinfo")))
			error("Strinfo not found in symbol table\n");
	if(!Qsvc)
		if(!(Qsvc = symsrch("qsvc")))
			error("qsvc not found in symbol table\n");

	strinit = 1;
}


/* get arguments for strstat function */
int
getstrstat()
{
	int c;

	streaminit();
	optind = 1;
	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	prstrstat();
}


/* get arguments for linkblk function */
int
getlinkblk()
{
	int all = 0;
	int phys = 0;
	long addr = -1;
	int c;
	register int i;
	struct linkinfo *lp;
	struct strinfo strinfo[NDYNAMIC];

	optind = 1;
	while((c = getopt(argcnt,args,"epw:")) !=EOF) {
		switch(c) {
			case 'e' :	all = 1;
					break;
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 1;
					break;
			default  :	longjmp(syn,0);
		}
	}
	if (!Strinfop)
		if(!(Strinfop = symsrch("Strinfo")))
			error("Strinfo not found in symbol table\n");
	readmem((long)Strinfop->n_value,1,-1,(char *)&strinfo,
		sizeof(strinfo),"Strinfo");
	fprintf(fp,"LINKBLK TABLE SIZE = %d\n",strinfo[DYN_LINKBLK].st_cnt);
	fprintf(fp,"LBLKADDR     QTOP     QBOT FILEADDR    MUXID\n");
	if(args[optind]) {
		all = 1;
		do {
			if((addr = strcon(args[optind++], 'h')) == -1)
				error("\n");
			(void) prlinkblk(all,addr,phys);
		}while(args[optind]);
	}
	else {
		lp = (struct linkinfo *) strinfo[DYN_LINKBLK].st_head;
		while(lp) {
			lp = prlinkblk(all,lp,phys);
		}
	}
}

/* print linkblk table */
struct linkinfo *
prlinkblk(all,addr,phys)
int all,phys;
long addr;
{
	struct linkinfo linkbuf;
	struct linkinfo *lp;

	readmem(addr,1,-1,(char *)&linkbuf,sizeof(linkbuf),"linkblk table");
	lp = &linkbuf;
	if(!lp->li_lblk.l_qbot && !all)
		return;
	fprintf(fp,"%8x", addr);
	fprintf(fp," %8x",lp->li_lblk.l_qtop);
	fprintf(fp," %8x",lp->li_lblk.l_qbot);
	fprintf(fp," %8x",lp->li_fpdown);
	if (lp->li_lblk.l_qbot)
		fprintf(fp," %8d\n",lp->li_lblk.l_index);
	else
		fprintf(fp,"        -\n");
	return(lp->li_next);
}
