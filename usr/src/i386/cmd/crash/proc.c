/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:i386/cmd/crash/proc.c	1.1.1.4"
#ident  "$Header: $"

/*
 * This file contains code for the crash functions:  proc, defproc, hrt.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <a.out.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <sys/sysmacros.h>
#include <sys/fs/s5dir.h>
#include <sys/cred.h>
#include <sys/user.h>
#include <sys/var.h>
#include <vm/as.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/ioctl.h>
#include <sys/sysi86.h>
#ifdef notdef
#include <sys/hrtcntl.h>
#include <sys/hrtsys.h>
#endif
#include <sys/priocntl.h>
#include <sys/procset.h>
#include <sys/vnode.h>
#include <sys/session.h>
#include "crash.h"
#include <priv.h>
#include <sys/secsys.h>
#include <sys/mac.h>
#include <audit.h>
#include <sys/stropts.h>
#include <sys/exec.h>

#include <sys/vmparam.h>
#include <sys/lwp.h>

extern struct user *ubp;		/* pointer to the ublock */
extern struct syment *U;
void	pr_aproc();
void	pr_alwp();
char *proc_clk[] = {
		"CLK_STD",
		"CLK_USERVIRT",
		"CLK_PROCVIRT"
};


int procdebug=0;
#define PDB if(procdebug)

/* token name flag for privileges and audit events */
int tokename = 0;

extern char	*cnvemask();

/* get arguments for proc function */
int
getproc()
{
	int slot = -1;
	int all = 0;
	int full = 0;
	int phys = 0;
	int run = 0;
	int alarm = 0;
	long addr = -1;
	long arg1 = -1;
	long arg2 = -1;
	pid_t id = -1;
	int c;
	char *heading = "SLOT ST  PID  PPID   SID   UID   LWP PRI ENG   EVENT     NAME        FLAGS\n";
	char *prprocalarm_hdg = "    CLOCK       TIME     INTERVAL    CMD    EID     PREVIOUS     NEXT    \n\n";

	optind = 1;
	tokename = 0;
	while((c = getopt(argcnt,args,"efpanrw:")) !=EOF) {
		switch(c) {
			case 'a' :	alarm = 1;
					break;
			case 'e' :	all = 1;
					break;
			case 'f' :	full = 1;
					break;
			case 'w' :	redirect();
					break;
			case 'r' :	run = 1;
					break;
			case 'p' :	phys = 1;
					break;
			case 'n' :	tokename = 1;
					break;
			default  :	longjmp(syn,0);
		}
	}
	fprintf(fp,"PROC TABLE SIZE = %d\n",vbuf.v_proc);
	if(!full && !alarm)
		fprintf(fp,"%s",heading);
	if(alarm)
		fprintf(fp,"%s", prprocalarm_hdg);
	if(args[optind]) {
		all = 1;
		do {
			if(*args[optind] == '#') {
				if((id = (pid_t)strcon(++args[optind],'d')) == -1)
					error("\n");
				prproc(all,full,slot,id,tokename,phys,run,alarm,addr,heading);
			}
			else {
				getargs(vbuf.v_proc,&arg1,&arg2);
				if (arg1 == -1) 
					continue;
				if (arg2 != -1)
					for (slot = arg1; slot <= arg2; slot++)
						prproc(all, full, slot, id,
							tokename,phys, run,
							alarm, addr, heading);
				else {
					if ((arg1 < vbuf.v_proc) &&
						(arg1 >= 0))
						slot = arg1;
					else
						addr = arg1;
					prproc(all, full, slot, id, tokename,
						phys, run, alarm, addr,
						heading);
				}
			}
			id = slot = addr = arg1 = arg2 = -1;
		} while(args[++optind]);
	}
	else{
		/*
		 * Create a new slot table to
		 * reflect the current state
		 */
		if(active)
			makeslottab();

		for (slot = 0; slot < vbuf.v_proc; slot++)
			prproc(all, full, slot, id, tokename,
				phys, run, alarm, addr, heading);
	}
}

/* print proc table */
int
prproc(all, full, slot, id, symprv, phys, run, alarm, addr, heading)
int all, full, slot, phys, run, alarm, symprv;
pid_t id;
long addr;
char *heading;
{
	char ch,*typ;
	char cp[PSCOMSIZ+1];
	struct proc procbuf, *procaddr;
	struct sess sess;
	int i,j,cnt,seqnum;
	extern long lseek();
	char buf[40];
	int type = 0;
	void	pr_privs();
	proc_t *slot_to_proc();

	pte_t *ubpte;
	
	lwp_t lwp;
	lwp_t *lwpp;
	user_t *up;
	char*  ublock;
	struct cred cred;
	struct execinfo exec;
	int lwp_num=0;
	

	if (id != -1) {
		for (slot = 0; ; slot++) {
			if (slot == vbuf.v_proc) {
				fprintf(fp,"%d not valid process id\n",id);
				return;
			}
			if (slot_to_pid(slot) == id) {
				procaddr = slot_to_proc(slot);
				break;
			}
		}
	} else if (slot != -1) 
		procaddr = slot_to_proc(slot);
	else for (slot = 0; ; slot++) {
		if (slot == vbuf.v_proc) {
			fprintf(fp,"%x not valid process address\n", addr);
			return;
		}
		procaddr = slot_to_proc(slot);
		if (phys) {
			if (addr==(cr_vtop(procaddr,-1)))
				break;
		} else {
			if (addr == (long)procaddr)
				break;
		}
	}
	if (!procaddr)
		return;

	/* get the proc structure */

	readmem((long)procaddr,!phys,-1,
		    (char *)&procbuf,sizeof procbuf,"proc table");

	/* read in execinfo structure to get command name */
	if (procbuf.p_execinfo)
		readmem((long)procbuf.p_execinfo, !phys, -1,
			(char *)&exec,sizeof(struct execinfo ),
			"execinfo structure");

	/* lwp structure */

	if (procbuf.p_lwpp)
		readmem((long)procbuf.p_lwpp, !phys, -1,
			(char *)&lwp, sizeof(lwp_t), "lwp structure");

	/* read in cred structure to get uid */

	if (lwp.l_cred)
		readmem((long)lwp.l_cred, !phys, -1,
		    (char *)&cred, sizeof(struct cred), "cred structure");

	if (getublock(slot,lwp_num) < 0 )
		return;

	/*up = (user_t*) (ublock + (int)((int)(U->n_value) - (int)KVUBLK)); */
	up = (user_t*)UBLOCK_TO_UAREA(ublock);

	if (full)
		fprintf(fp,"%s",heading);

	switch (lwp.l_stat) {
	case SSLEEP:	ch = 's'; break;
	case SRUN:	ch = 'r'; break;
	case SIDL:	ch = 'i'; break;
	case SSTOP:	ch = 't'; break;
	case SONPROC:	ch = 'p'; break;
	default:	ch = '?'; break;
	}

	if (slot == -1)
		fprintf(fp,"  - ");
	else fprintf(fp,"%2d",slot);

	if (lwp.l_eng )
		fprintf(fp,"   %c %5u %5u %5u %4u %5u   %2u %3u",
			ch,
			slot_to_pid(slot),
			procbuf.p_ppid,
			readsid(procbuf.p_sessp),
			cred.cr_ruid,0,
			lwp.l_pri,
			eng_to_id(lwp.l_eng));
	else
		fprintf(fp,"   %c %5u %5u %5u %4u %5u   %2u ???",
			ch,
			slot_to_pid(slot),
			procbuf.p_ppid,
			readsid(procbuf.p_sessp),
			cred.cr_ruid,0,
			lwp.l_pri,
			eng_to_id(lwp.l_eng));

	switch(lwp.l_stype) {
		case ST_NONE:    ch = ' '; break;
		case ST_COND:    ch = 'c'; break;
		case ST_EVENT:   ch = 'e'; break;
		case ST_RDLOCK:  ch = 'r'; break;
		case ST_WRLOCK:  ch = 'w'; break;
		case ST_SLPLOCK: ch = 's'; break;
		case ST_USYNC:   ch = 'u'; break;
		default:	 ch = '?'; break;
	}
	if( lwp.l_stype != ST_NONE) {
		fprintf(fp,"  %c-%08x",ch,lwp.l_syncp);
	} else
		fprintf(fp,"            ");

	fprintf(fp,"  %-12s\n",exec.ei_comm);

	if (full) {
		int s;
		fprintf(fp,"\tProcess Credentials: ");
		fprintf(fp,"uid: %u, gid: %u, real uid: %u, real gid: %u lid: %x\n",
			cred.cr_uid, cred.cr_gid, cred.cr_ruid,
			cred.cr_rgid, cred.cr_lid);
		fprintf(fp, "\tProcess Privileges: ");
		pr_privs(cred, symprv);
		fprintf(fp,"\tSignal state:"
			" pend: %08X %08X ignore: %08X %08X",
			procbuf.p_sigs.ks_sigbits[0],
			procbuf.p_sigs.ks_sigbits[1],
			procbuf.p_sigignore.ks_sigbits[0],
			procbuf.p_sigignore.ks_sigbits[1]);
		for (s=0; s<MAXSIG; ++s)
		{
			if ((s%3) == 0)
				fprintf(fp, "\n\t");
			fprintf(fp,"  [%02d]: %02x %02x %08x",
				s,
				procbuf.p_sigstate[s].sst_cflags,
				procbuf.p_sigstate[s].sst_rflags,
				procbuf.p_sigstate[s].sst_handler);
		}
		putc('\n', fp);

		{
			int i, p, n = procbuf.p_fdtab.fdt_size;
			fd_entry_t buf[1000];
			if (n > 1000)
				n = 1000; /* too bad */
			readmem((long)procbuf.p_fdtab.fdt_entrytab, !phys, -1,
				(char *)buf,
				n * sizeof(fd_entry_t), "fdt_entrytab");

			fprintf(fp,"\tFile descriptors: entrytab=%x",
				procbuf.p_fdtab.fdt_entrytab);
			p = 0;
			for (i=0; i<n; ++i)
				if (buf[i].fd_status)
				{
					if ((p++%3) == 0)
						fprintf(fp, "\n\t");
					fprintf(fp,"  [%02d]: %08x %2u %1x %1x",
						i,
						buf[i].fd_file,
						buf[i].fd_lwpid,
						buf[i].fd_flag,
						buf[i].fd_status);
				}
			putc('\n', fp);
		}
	}

	if (full && procbuf.p_auditp) {
		fprintf(fp,"\tProcess Audit Structure:\n");
		pr_aproc(procbuf.p_auditp, phys);
	}

	/* print info about each lwp */

	if (full) {
		fprintf(fp,"\tLWP signal state:"
			" pend: %08X %08X mask: %08X %08X\n",
			lwp.l_sigs.ks_sigbits[0],
			lwp.l_sigs.ks_sigbits[1],
			lwp.l_sigheld.ks_sigbits[0],
			lwp.l_sigheld.ks_sigbits[1]);

		if (lwp.l_auditp) {
			fprintf(fp,"\tLWP Audit Structure:\n");
			pr_alwp(lwp.l_auditp, phys);
		}
	}

	for (i= 1; i < procbuf.p_ntotallwp; i++) {

		if ((lwpp = lwp.l_next) == NULL)
			continue;
		readmem((long)lwpp, !phys, -1, (char *)&lwp,
			sizeof(lwp_t), "lwp structure");

		switch (lwp.l_stat) {
		case SSLEEP:	ch = 's'; break;
		case SRUN:	ch = 'r'; break;
		case SIDL:	ch = 'i'; break;
		case SSTOP:	ch = 't'; break;
		case SONPROC:	ch = 'p'; break;
		default:	ch = '?'; break;
		}

		fprintf(fp, "     %c                           %2u   %2u  %2u",
			ch, i, lwp.l_pri, eng_to_id(lwp.l_eng));

		switch(lwp.l_stype) {
			case ST_NONE:    ch = ' '; break;
			case ST_COND:    ch = 'c'; break;
			case ST_EVENT:   ch = 'e'; break;
			case ST_RDLOCK:  ch = 'r'; break;
			case ST_WRLOCK:  ch = 'w'; break;
			case ST_SLPLOCK: ch = 's'; break;
			case ST_USYNC:   ch = 'u'; break;
			default:     ch = '?'; break;
		}
		if (lwp.l_stype != ST_NONE) {
			fprintf(fp,"  %c-%08x",ch,lwp.l_syncp);
		} else
			fprintf(fp,"            ");

		if (lwp.l_name != NULL) {
			char buf[20];
			readmem((long)lwp.l_name, !phys, -1, (char *)buf,
				sizeof buf, "lwp name");
			fprintf(fp, "  %.20s\n", buf);
		} else
			fprintf(fp, "\n");

		if (full) {
			fprintf(fp,"\tLWP signal state:"
				" pend: %08X %08X mask: %08X %08X\n",
				lwp.l_sigs.ks_sigbits[0],
				lwp.l_sigs.ks_sigbits[1],
				lwp.l_sigheld.ks_sigbits[0],
				lwp.l_sigheld.ks_sigbits[1]);

			if (lwp.l_auditp) {
				fprintf(fp,"\tLWP Audit Structure:\n");
				pr_alwp(lwp.l_auditp, phys);
			}
		}
	}

	return;


}

/* get arguments for defproc function */
int
getdefproc()
{
	int c;
	int proc = -1;
	int reset = 0;

	optind = 1;
	while((c = getopt(argcnt,args,"cw:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'c' :	reset = 1;
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(args[optind]) 
		if((proc = strcon(args[optind],'d')) == -1)
			error("\n");
	prdefproc(proc,reset);
}

int
getdeflwp()
{
	int c;
	int lwp = -1;
	int reset = 0;

	optind = 1;
	while ((c = getopt(argcnt,args,"cw:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'c' :	reset = 1;
					break;
			default  :	longjmp(syn,0);
		}
	}
	if (args[optind]) 
		if ((lwp = strcon(args[optind],'d')) == -1)
			error("\n");
	prdeflwp(lwp, reset);
}

int
prdeflwp(lwp, reset)
int lwp, reset;
{
	if (reset)
		get_context();
	else if (lwp > -1) {
		Cur_lwp = lwp;
	}
	pr_context();
}

/* print results of defproc function */
int
prdefproc(proc,reset)
int proc,reset;
{
	if (reset)
		get_context();

	else if (proc > -1) {
		if ((proc > vbuf.v_proc) || (proc < 0))
			error("%d out of range\n",proc);
		Cur_proc = proc;
		Cur_lwp =0;
	}
	pr_context();
}

#ifdef notdef

/* print the high resolution timers */
int
gethrt()
{
	int c;
	static struct syment *Hrt;
	timer_t hrtbuf;
	timer_t *hrp;
	extern timer_t hrt_active;
	char *prhralarm_hdg = "    PROCP       TIME     INTERVAL    CMD    EID     PREVIOUS     NEXT    \n\n";


	optind = 1;
	while((c=getopt(argcnt, args,"w:")) != EOF) {
		switch(c) {
			case 'w'  :	redirect();
					break;
			default   :	longjmp(syn,0);
		}
	}

	if (!(Hrt = symsrch("hrt_active")))
		fatal("hrt_active not found in symbol table\n");


	readmem ((long)Hrt->n_value, 1, -1, (char *)&hrtbuf,
		sizeof hrtbuf, "high resolution alarms");

	fprintf(fp, "%s", prhralarm_hdg);
	hrp=hrtbuf.hrt_next;
	for (; hrp != (timer_t *)Hrt->n_value; hrp=hrtbuf.hrt_next) {
		readmem((long)hrp, 1, -1, (char *)&hrtbuf,
			sizeof hrtbuf, "high resolution alarms");
		fprintf(fp, "%12x %7d%11d %7d %13x %10x\n",
			hrtbuf.hrt_proc,
			hrtbuf.hrt_time,
			hrtbuf.hrt_int,
			hrtbuf.hrt_cmd,
			hrtbuf.hrt_prev,
			hrtbuf.hrt_next);
	}
}

#endif

int
readsid(sessp)
	struct sess *sessp;
{
	struct sess s;

	readmem((char *)sessp,1,-1,(char *)&s,sizeof(struct sess),
		"session structure");

	return readpid(s.s_sidp);
}

int
readpid(pidp)
	struct pid *pidp;
{
	struct pid p;

#ifdef notdef
	readmem((char *)pidp,1,-1,(char *)&p,sizeof(struct pid),
		"pid structure");
#endif
	p.pid_id =0;

	return p.pid_id;
}

static	void
pr_privs(lst, symb)
cred_t	lst;
register int	symb;
{
	extern	void	prt_symbprvs();

	if (symb) {
		prt_symbprvs("\n\t\tworking: ", lst.cr_wkgpriv);
		prt_symbprvs("\t\tmaximum: ", lst.cr_maxpriv);
	}
	else {
		fprintf(fp, "working: %.8x", lst.cr_wkgpriv);
		fprintf(fp, "\tmaximum: %.8x", lst.cr_maxpriv);
		fprintf(fp, "\n");
	}
}

runq()
{
	fprintf(fp,"runq not yet implemented\n");
}


static	void
pr_aproc(ap, phys)
aproc_t	*ap;
int	phys;
{
	aproc_t aproc;
	adtpath_t cdp;
	adtpath_t rdp;
	adtkemask_t emask;
	char *evtstr, *cwdp, *crdp;	

	readmem((long)ap, !phys, -1,
    		(char *)&aproc, sizeof(aproc_t),"aproc structure");

	if (aproc.a_cdp) {
		readmem((long)aproc.a_cdp,1,-1,(char *)&cdp,sizeof cdp, 
			"audit current working directory structure");
		if ((cwdp = ((char *)calloc(cdp.a_len+1,1))) ==  NULL)
			exit(ADT_MALLOC);
		readmem((long)cdp.a_path, 1, -1,(char *)cwdp, cdp.a_len,
			"audit current working directory string");
		fprintf(fp,"\t\tCurrent Working Directory: %s\n",cwdp); 
	} else
		fprintf(fp,"\t\tCurrent Working Directory: ?\n"); 

	if (aproc.a_rdp) {
		readmem((long)aproc.a_rdp,1,-1,(char *)&rdp,sizeof rdp, 
			"audit current root directory structure");
		if ((crdp = ((char *)calloc(rdp.a_len+1,1))) ==  NULL)
			exit(ADT_MALLOC);
		readmem((long)rdp.a_path, 1, -1,(char *)crdp, rdp.a_len,
			"audit current root directory string");
		fprintf(fp,"\t\tCurrent Root Directory: %s\n",crdp); 
	}

	readmem((long)aproc.a_emask, 1, -1,(char *)&emask, sizeof emask,
		"audit process event mask structure");

	if (tokename) {
		if (aproc.a_flags > 0) 
			fprintf(fp,"\t\tFlags: %s %s\n",
			aproc.a_flags & AOFF ? "AOFF," : "",
			aproc.a_flags & AEXEMPT ? "AEXEMPT," : "");
		else
			fprintf(fp,"\t\tFlags: NONE\n");
		if ((evtstr=(char *)cnvemask(&emask.ad_emask)) != NULL)
			fprintf(fp,"\t\tProcess Event Mask:\t%s\n",evtstr);
		else
			fprintf(fp,"\t\tProcess Event Mask:\tEVTSTR=NULL ERROR\n");
		if ((evtstr=(char *)cnvemask(&aproc.a_useremask)) != NULL)
			fprintf(fp,"\t\tUser    Event Mask:\t%s\n",evtstr);
		else
			fprintf(fp,"\t\tUser    Event Mask:\tEVTSTR=NULL ERROR\n");
	}else  {
		fprintf(fp,"\t\tFlags: %u\n",aproc.a_flags);
		fprintf(fp,"\t\tProcess Event Mask: %08lx %08lx %08lx %08lx\n",
			emask.ad_emask[0], emask.ad_emask[1],
			emask.ad_emask[2], emask.ad_emask[3]);
		fprintf(fp,"\t\t                    %08lx %08lx %08lx %08lx\n",
			emask.ad_emask[4], emask.ad_emask[5],
			emask.ad_emask[6], emask.ad_emask[7]);
		fprintf(fp,"\t\tUser    Event Mask: %08lx %08lx %08lx %08lx\n",
			aproc.a_useremask[0], aproc.a_useremask[1],
			aproc.a_useremask[2], aproc.a_useremask[3]);
		fprintf(fp,"\t\t                    %08lx %08lx %08lx %08lx\n",
			aproc.a_useremask[4], aproc.a_useremask[5],
			aproc.a_useremask[6], aproc.a_useremask[7]);
	}
}

static	void
pr_alwp(alwpp, phys)
alwp_t	*alwpp;
int	phys;
{
	alwp_t alwp;
	adtpath_t cdp;
	adtpath_t rdp;
	adtkemask_t emask;
	register struct tm * td;
	char *evtstr, *cwdp, *crdp;	
	ulong_t	seqnum;

	readmem((long)alwpp, !phys, -1,
    		(char *)&alwp, sizeof(alwp_t),"alwp structure");

	readmem((long)alwp.al_emask, 1, -1,(char *)&emask, sizeof emask,
		"lwp audit event mask structure");

	if (tokename) {
		if (alwp.al_flags > 0) 
			fprintf(fp,"\t\tFlags: %s %s\n",
			alwp.al_flags & AUDITME ? "AUDITME," : "",
			alwp.al_flags & ADT_NEEDPATH ? "ADT_NEEDPATH," : "",
			alwp.al_flags & ADT_OBJCREATE ? "ADT_OBJCREATE," : "");
		else
			fprintf(fp,"\t\tFlags: NONE\n");
		if ((evtstr=(char *)prevtnam(alwp.al_event))!=NULL)
			fprintf(fp,"\tEvent Number: %s\n", evtstr);
		else
			fprintf(fp,"\tEvent Number NULL: ERROR\n");
		seqnum=EXTRACTSEQ(alwp.al_seqnum);
		fprintf(fp,"\tRecord Sequence Number: %u\n",seqnum);
		if (alwp.al_time.tv_sec == 0)
			fprintf(fp,"\tStarting time of Event: 0\n");
		else {
			td = gmtime((const time_t *)&alwp.al_time);
			fprintf(fp,"\tStarting time of Event: %02d/%02d/%02d%  02d:%02d:%02d GMT\n",
				td->tm_mon + 1, td->tm_mday, td->tm_year,
				td->tm_hour, td->tm_min, td->tm_sec);
		}
	
		if (emask.ad_refcnt == 1) {
		    if ((evtstr=(char *)cnvemask(&emask.ad_emask)) != NULL)
			fprintf(fp,"\t\tLWP     Event Mask:\t%s\n",evtstr);
		    else
			fprintf(fp,"\t\tLWP     Event Mask:\tEVTSTR=NULL ERROR\n");
		}
	} else  {
		fprintf(fp,"\t\tFlags: %u\n",alwp.al_flags);
		fprintf(fp,"\t\tEvent Number: %u\n",alwp.al_event);
		seqnum=EXTRACTSEQ(alwp.al_seqnum);
		fprintf(fp,"\t\tRecord Sequence Number: %u\n",seqnum);
		fprintf(fp,"\t\tStarting time of Event: %ld\n",alwp.al_time);
		if (emask.ad_refcnt == 1) {
		  fprintf(fp,"\t\tLWP     Event Mask: %08lx %08lx %08lx %08lx\n",
			emask.ad_emask[0], emask.ad_emask[1],
			emask.ad_emask[2], emask.ad_emask[3]);
		  fprintf(fp,"\t\t                    %08lx %08lx %08lx %08lx\n",
			emask.ad_emask[4], emask.ad_emask[5],
			emask.ad_emask[6], emask.ad_emask[7]);
		}
	}

	if (alwp.al_cdp) {
		readmem((long)alwp.al_cdp,1,-1,(char *)&cdp,sizeof cdp, 
			"audit current working directory structure");
		if ((cwdp = ((char *)calloc(cdp.a_len+1,1))) ==  NULL)
			exit(ADT_MALLOC);
		if (cdp.a_ref == 1) {
			readmem((long)cdp.a_path, 1, -1,(char *)cwdp, cdp.a_len,
				"audit current working directory string");
			fprintf(fp,"\t\tCurrent Working Directory: %s\n",cwdp); 
		} 
	}

	if (alwp.al_rdp) {
		readmem((long)alwp.al_rdp,1,-1,(char *)&rdp,sizeof rdp, 
			"audit current root directory structure");
		if ((crdp = ((char *)calloc(rdp.a_len+1,1))) ==  NULL)
			exit(ADT_MALLOC);
		readmem((long)rdp.a_path, 1, -1,(char *)crdp, rdp.a_len,
			"audit current root directory string");
		fprintf(fp,"\t\tCurrent Root Directory: %s\n",crdp); 
	}
	fprintf(fp,"\t\tBufp:   0x%-8x\n",alwp.al_bufp);
	fprintf(fp,"\t\tObufp:  0x%-8x\n",alwp.al_obufp);
	fprintf(fp,"\t\tFrecp:  0x%-8x\n",alwp.al_frec1p);
	fprintf(fp,"\t\tCmpcnt:   %8d\n",alwp.al_cmpcnt);
}
