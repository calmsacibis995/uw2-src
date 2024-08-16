/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:i386/cmd/crash/util.c	1.1.1.13"

/*
 * This file contains code for utilities used by more than one crash function.
 */

#include "sys/param.h"
#include "sys/vmparam.h"
#include "a.out.h"
#include "signal.h"
#include "stdio.h"
#include "setjmp.h"
#include "ctype.h"
#include "sys/types.h"
#include "sys/var.h"
#include "sys/proc.h"
#include "sys/bootinfo.h"
#include "sys/plocal.h"
#include "sys/pid.h"
#include "crash.h"
#include "priv.h"
#include "sys/kcore.h"
#include "sys/resource.h"
#include <unistd.h>
#include "memory.h"

#define min(a, b)       ((a) < (b) ? (a) : (b))

extern jmp_buf jmp;
extern struct syment *V;
struct syment *U;
extern long cr_vtop();
extern int opipe;
extern FILE *rp;
void exit();
void free();
extern int Nengine;
int Cur_proc;
int Cur_lwp;
extern int Cur_eng;

STATIC cr_mchunk_t *cr_mem_org = NULL;

/* close pipe and reset file pointers */
int
resetfp()
{
	extern void pipesig();

	if (opipe == 1) {
		pclose(fp);
		signal(SIGPIPE,pipesig);
		opipe = 0;
		fp = stdout;
	}
	else {
		if ((fp != stdout) && (fp != rp) && (fp != NULL)) {
			fclose(fp);
			fp = stdout;
		}
	}
}

/* signal handling */
void 
sigint()
{
	extern int *stk_bptr;

#if defined(__STDC__)
	signal(SIGINT, (void (*)(int))sigint);
#else
	signal(SIGINT, sigint);
#endif
	if (stk_bptr) {
		free((char *)stk_bptr);
		stk_bptr = NULL;
	}
	fflush(fp);
	resetfp();
	fprintf(fp,"\n");
	longjmp(jmp, 0);
}

/* used in init.c to exit program */
/*VARARGS1*/
int
fatal(string,arg1,arg2,arg3)
char *string;
int arg1,arg2,arg3;
{
	fprintf(stderr,"crash: ");
	fprintf(stderr,string,arg1,arg2,arg3);
	exit(1);
}

/* string to hexadecimal long conversion */
long
hextol(s)
char *s;
{
	int	i,j;

	for (j = 0; s[j] != '\0'; j++)
		if ((s[j] < '0' || s[j] > '9') && (s[j] < 'a' || s[j] > 'f')
			&& (s[j] < 'A' || s[j] > 'F'))
			break ;
	if (s[j] != '\0' || sscanf(s, "%x", &i) != 1) {
		prerrmes("%c is not a digit or letter a - f\n",s[j]);
		return(-1);
	}
	return(i);
}


/* string to decimal long conversion */
long
stol(s)
char *s;
{
	int	i,j;
		
	for (j = 0; s[j] != '\0'; j++)
		if ((s[j] < '0' || s[j] > '9'))
			break ;
	if (s[j] != '\0' || sscanf(s, "%d", &i) != 1) {
		prerrmes("%c is not a digit 0 - 9\n",s[j]);
		return(-1);
	}
	return(i);
}


/* string to octal long conversion */
long
octol(s)
char *s;
{
	int	i,j;
		
	for (j = 0; s[j] != '\0'; j++)
		if ((s[j] < '0' || s[j] > '7')) 
			break ;
	if (s[j] != '\0' || sscanf(s, "%o", &i) != 1) {
		prerrmes("%c is not a digit 0 - 7\n",s[j]);
		return(-1);
	}
	return(i);
}


/* string to binary long conversion */
long
btol(s)
char *s;
{
	int	i,j;
		
	i = 0;
	for(j = 0; s[j] != '\0'; j++)
		switch(s[j]) {
			case '0' :	i = i << 1;
					break;
			case '1' :	i = (i << 1) + 1;
					break;
			default  :	prerrmes("%c is not a 0 or 1\n",s[j]);
					return(-1);
		}
	return(i);
}

/* string to number conversion */
long
strcon(string,format)
char *string;
char format;
{
	char *s;

	s = string;
	if(*s == '0') {
		if(strlen(s) == 1)
			return(0); 
		switch(*++s) {
			case 'X' :
			case 'x' :	format = 'h';
					s++;
					break;
			case 'B' :
			case 'b' :	format = 'b';
					s++;
					break;
			case 'D' :
			case 'd' :	format = 'd';
					s++;
					break;
			default  :	format = 'o';
		}
	}
	if (!format)
		format = 'd';
	switch(format) {
		case 'h' :	return(hextol(s));
		case 'd' :	return(stol(s));
		case 'o' :	return(octol(s));
		case 'b' :	return(btol(s));
		default  :	return(-1);
	}
}


/* lseek and read */
int
readmem(addr, mode, proc, buffer, size, name)
unsigned long addr;
int mode,proc;
char *buffer;
unsigned size;
char *name;
{
	unsigned cnt;
	unsigned long paddr;
	int err;

	if (debugmode > 5)
		fprintf(stderr,"readmem addr= %x mode=%x proc=%x size =%x\
			 name =%s\n", addr,mode,proc,size,name);
	if (!KADDR(addr) && (proc == -1) && mode)
		error("readmem error: user address for kernel process\n");

        while (size > 0) {
		/* compute bytes till end of page */
		cnt = MMU_PAGESIZE - (addr & PAGEOFFSET); 

                if (cnt > size)
                        cnt = size;

		if (!mode || !Virtmode) 
			paddr = addr;
		else
			paddr = cr_vtop(addr,proc);
		if (paddr == -1)
			error("readmem: %x invalid address\n", addr);

		if (active) {
			if (lseek(mem,paddr,0) == -1) {
				fprintf(stderr, 
					"seek error on address %x\n", addr);
				return -1;
			}
			if (read(mem, buffer, cnt) != cnt) {
				fprintf(stderr,
					"read error on address %x\n", addr); 
				return -1;
			}
		} else {  /* reading from a dumpfile */
			if (cr_read(paddr, buffer, cnt) == -1)
				return -1;
		}
                size -= cnt;
                addr += cnt;
                buffer += cnt;
		if (debugmode > 5) {
			int i;
			unsigned char *c = (unsigned char *)buffer - cnt;
                        if (cnt > 256) 
				cnt=256;
                        for (i = 0; i < cnt; c++, i++) {
				if (i && !(i&0xf))
					fprintf(stderr,"\n");
                                fprintf(stderr," %02x",*c);
                        }
                        fprintf(stderr,"\n");
                }
        }
        return 0;
}

/* error handling */
/*VARARGS1*/
int
error(string,arg1,arg2,arg3)
char *string;
int arg1,arg2,arg3;
{

	if (rp) 
		fprintf(stdout,string,arg1,arg2,arg3);
	fprintf(fp,string,arg1,arg2,arg3);
	fflush(fp);
	resetfp();
	longjmp(jmp,0);
}


/* print error message */
/*VARARGS1*/
int
prerrmes(string,arg1,arg2,arg3)
char *string;
int arg1,arg2,arg3;
{

	if((rp && (rp != stdout)) || (fp != stdout)) 
		fprintf(stdout,string,arg1,arg2,arg3);
	fprintf(fp,string,arg1,arg2,arg3);
	fflush(fp);
}


/* simple arithmetic expression evaluation ( +  - & | * /) */
long
eval(string)
char *string;
{
	int j,i;
	char rand1[ARGLEN];
	char rand2[ARGLEN];
	char *op;
	long addr1,addr2;
	struct syment *sp;
	extern char *strpbrk();

	if(string[strlen(string)-1] != ')') {
		prerrmes("(%s is not a well-formed expression\n",string);
		return(-1);
	}
	if(!(op = strpbrk(string,"+-&|*/"))) {
		prerrmes("(%s is not an expression\n",string);
		return(-1);
	}
	for(j=0,i=0; string[j] != *op; j++,i++) {
		if(string[j] == ' ')
			--i;
		else rand1[i] = string[j];
	}
	rand1[i] = '\0';
	j++;
	for (i = 0; string[j] != ')'; j++,i++) {
		if(string[j] == ' ')
			--i;
		else rand2[i] = string[j];
	}
	rand2[i] = '\0';
	if (!strlen(rand2) || strpbrk(rand2,"+-&|*/")) {
		prerrmes("(%s is not a well-formed expression\n",string);
		return(-1);
	}
	if (sp = symsrch(rand1))
		addr1 = sp->n_value;
	else if((addr1 = strcon(rand1,NULL)) == -1)
		return(-1);
	if (sp = symsrch(rand2))
		addr2 = sp->n_value;
	else if((addr2 = strcon(rand2,NULL)) == -1)
		return(-1);
	switch(*op) {
		case '+' : return(addr1 + addr2);
		case '-' : return(addr1 - addr2);
		case '&' : return(addr1 & addr2);
		case '|' : return(addr1 | addr2);
		case '*' : return(addr1 * addr2);
		case '/' : if(addr2 == 0) {
				prerrmes("cannot divide by 0\n");
				return(-1);
			   }
			   return(addr1 / addr2);
	}
	return(-1);
}

static struct procslot {
	proc_t *p;
	pid_t   pid;
} *slottab;
static maketab = 1;
static getu = 1;

static struct engslot {
	engine_t * e;
	processorid_t eng_id;
} * engtab;

/* make a table that maps address of engine structure to 
 * processor id number
 */
void
make_engmap()
{
	engine_t * begin;
	struct syment * sym_eng, *sym_neng;
	int i;


	if(!(sym_neng = symsrch("Nengine")))
		fatal("Nengine not found in symbol table\n");
	
	readmem( (long)sym_neng->n_value, 1, -1,(char *)&Nengine, 
		sizeof (int ), "Nengine");


	engtab = (struct engslot*)malloc(Nengine *sizeof(struct engslot));
	if(engtab == (struct engslot *)NULL)
		fatal("can't allocate engine slot table\n");

	if(!(sym_eng = symsrch("engine")))
		fatal("engine not found in symbol table\n");


	readmem( (long)sym_eng->n_value, 1, -1,(char *)&begin, 
		sizeof (engine_t *), "engine");

	for(i=0; i<Nengine; i++){
		engtab[i].e = begin + i;
		engtab[i].eng_id=i;
	}
	
}

int
eng_to_id(engine_t * e)
{
	int i;

	for(i=0; i<Nengine;i++){
		if( engtab[i].e == e )
			return(engtab[i].eng_id);
	}

	return(-1);
}

engine_t * 
id_to_eng(int id)
{
	int i;

	if( id < 0 || id > Nengine) return ( engine_t * ) -1;

	for(i=0; i<Nengine;i++){
		if(engtab[i].eng_id == id)
			return( engtab[i].e );
	}
	return((engine_t*)-1);
}

void
makeslottab()
{
	proc_t *prp, pr;
	struct pid pid;
	struct syment *practive;
	register i;

	maketab = 0;
	if (!(practive = symsrch("practive")))
		fatal("practive not found in symbol table\n");
	/*
	 * Make it reuseable
	 */
	if(slottab != (struct procslot *)NULL)
		free((char *)slottab);

	slottab = (struct procslot*)malloc(vbuf.v_proc*sizeof(struct procslot));
	if(slottab == (struct procslot *)NULL)
		fatal("can't allocate slot table\n");

	for (i = 0; i < vbuf.v_proc; i++)
		slottab[i].p = 0;

	readmem( (long)practive->n_value, 1, -1,(char *)&prp, 
		sizeof (proc_t *), "practive");

	for (; prp != NULL; prp = pr.p_next) {

                readmem(prp, 1, -1,(char *)&pr, sizeof (struct proc),
                        "proc table");

                readmem(pr.p_pidp, 1, -1,(char *)&pid, sizeof (struct pid),
                        "pid table");


                i = pr.p_slot;

                if (i < 0 || i >= vbuf.v_proc)
                        fatal("process slot out of bounds\n");

                slottab[i].p = prp;
                slottab[i].pid = pid.pid_id;
	}
}

pid_t
slot_to_pid(slot)
int slot;
{
	if (slot < 0 || slot >= vbuf.v_proc)
		return NULL;
	if (maketab)
		makeslottab();
	return slottab[slot].pid;
}

/* convert the lwp ordinal "slot" into
 * a pointer to the requested lwp
 */

lwp_t* 
slot_to_lwp(slot,proc)
int slot;
struct proc *proc;
{
	int i = 1;
	lwp_t lwp;
	lwp_t *lwpp;

	if (slot < 0)
		return NULL;

	lwpp = proc->p_lwpp;

	if (slot == 0) 
		return(lwpp);

	readmem(lwpp,1,-1,&lwp,sizeof(lwp_t),"lwp struct");
	lwpp = lwp.l_next;

	while ((i < slot) && lwp.l_next) {
		readmem(lwpp,1,-1,&lwp,sizeof(lwp_t),"lwp struct");
		lwpp = lwp.l_next;
		if (lwpp == NULL)
			break;
		i++;
	}
	if ((slot != i) || lwpp == NULL)
		return(NULL);
	else
		return(lwpp);
}

/* convert an lwp pointer to it's ordinal on that proc's list of lwps */
lwp_to_slot(lwpp,proc)
lwp_t* lwpp;
struct proc* proc;
{
	int slot=0;
	struct lwp lwp;

	if(proc->p_lwpp == lwpp ) return(slot);

	readmem(lwpp,1,-1,&lwp,sizeof(lwp_t),"lwp struct");
	slot++;
	
	while(lwpp != lwp.l_next) {
		readmem(lwpp,1,-1,&lwp,sizeof(lwp_t),"lwp struct");
		lwpp = lwp.l_next;
		if( lwp.l_next == NULL) break;
		slot++;
	}

	if( lwpp != lwp.l_next )
		error("cant find current lwp for the current process\n");

	return(slot);
}

proc_t * slot_to_proc(slot)
int slot;
{
	if (slot < 0 || slot >= vbuf.v_proc)
		return NULL;
	if (maketab)
		makeslottab();
	return slottab[slot].p;
}

/* convert proc address to proc slot */
int
proc_to_slot(addr)
long addr;
{
	int i;

	if (addr == NULL)
		return -1;

	if (maketab)
		makeslottab();

	for (i = 0; i < vbuf.v_proc; i++)
		if (slottab[i].p == (proc_t *)addr)
			return i;

	return -1;

}

/* get current context info: engine, process and lwp */

get_context()
{
	struct syment *L;
	engine_t engine;
	engine_t * eng_p;
	engine_t * id_to_eng();
	struct proc pr;
	extern struct user  *ubp;
	extern struct rlimits  *rlimitbp;

	Cur_lwp  = 0;
	Cur_proc = (-1);

	if(!(L = symsrch("l")))
                fatal("plocal structure (l.) not found in symbol table\n");
	
	readmem((long)L->n_value,1,-1,(char *)&l,
		sizeof (struct plocal) ,"boot plocal structure");

	if (debugmode > 1)
		fprintf(stderr,"get_context: l.userp is %x\n", l.userp);

	if (!KADDR(l.userp)) 
		error("get_context : l.userp is invalid - value is %x\n", 
			l.userp);

	Cur_eng = l.eng_num;
	if (debugmode > 5)
		fprintf(stderr,"get_context: read plocal structure and\
			 current eng is %d and cpu speed is %d\n",
			 Cur_eng, l.cpu_speed);

	if( (eng_p = id_to_eng(Cur_eng)) == (engine_t*)(-1)) 
		error("translating engine number");

	readmem((long)eng_p, 1, -1, (char *)&engine,
		sizeof (struct engine) ,"engine structure");

	readmem((long)l.userp,1,-1,(char *)ubp,
		sizeof (struct user) ,"current virtual u  structure");

	readmem((long)ubp->u_rlimits,1,-1,(char *)rlimitbp,
		sizeof(struct rlimits),"rlimits structure");

	if (debugmode > 5)
		fprintf(stderr,"get_context: ubp is %x\n", ubp);

	if (ubp && ubp->u_procp) {
		if (debugmode > 5)
			fprintf(stderr,"get_context: ubp->u_procp is %x\n",
				ubp->u_procp);
		readmem(ubp->u_procp, 1, -1,(char *)&pr, sizeof (struct proc),
				"proc structure");
	} else	{
		fprintf(stderr, "current engine %d on idle stack ?\n",
				Cur_eng);
		fprintf(stderr, "setting default proc, lwp to 0, 0\n");
		getublock(0, 0);

		readmem(ubp->u_procp, 1, -1, (char *)&pr, 
			sizeof(struct proc), "proc structure");
	}

	if (maketab)
		makeslottab();

	if (ubp->u_procp) {
		Cur_proc = (proc_to_slot(ubp->u_procp));
		Cur_lwp = lwp_to_slot(pr.p_lwpp,&pr);
	}
}

pr_context()
{
	if (Cur_proc ==  (-1 ) ){
		fprintf(fp,"Engine: %u Running the idle context\n",Cur_eng);
	} else {
		fprintf(fp,"Engine: %u Procslot: %u Lwpslot: %u\n",
					Cur_eng, Cur_proc,Cur_lwp);
	}
}
		
/* get current process slot number */

int
getcurproc()
{
	long curproc;
	struct user  ub;

	readmem((long)l.userp, 1, -1, &ub, sizeof (struct user),
		"current virtual u  structure");

	if (maketab)
		makeslottab();

	if (ub.u_procp != 0 )
		Cur_proc = proc_to_slot(ub.u_procp);
	else
		Cur_proc = 0;

	return Cur_proc;
}

/* check if the given proc slot is on proc */
int
onproc(proc)
int proc;
{
	struct user  ub;

	readmem((long)l.userp, 1, -1, &ub, sizeof (struct user),
		"current virtual u  structure");

	if (ub.u_procp != 0) {
		if (proc_to_slot(ub.u_procp) == proc)
			return 1;
	}
	return 0;
}

/* determine valid process table entry */
int
procntry(slot,prbuf)
int slot;
struct  proc *prbuf;
{
	long addr;

	if(slot == -1) 
		slot = getcurproc();

	if ((addr = (long)slot_to_proc(slot)) == NULL)
		error(" %d is not a valid process\n",slot);
	readmem((long)addr,1, -1,(char *)prbuf,sizeof (proc_t),"proc table");
}

/* argument processing from **args */
long
getargs(max,arg1,arg2)
int max;
long *arg1,*arg2;
{	
	struct syment *sp;
	long slot;

	/* range */
	if(strpbrk(args[optind],"-")) {
		range(max,arg1,arg2);
		return;
	}
	/* expression */
	if(*args[optind] == '(') {
		*arg1 = (eval(++args[optind]));
		return;
	}
	/* symbol */
	if((sp = symsrch(args[optind])) != NULL) {
		*arg1 = (sp->n_value);
		return;
	}

	/* slot number or address */
	if((slot = strcon(args[optind],'d')) == -1) {
		*arg1 = -1;
		return;
	}
	*arg1 = slot;
} 

/* get slot number in table from address */
int
getslot(addr,base,size,phys,max)
long addr,base,max;
int size,phys;
{
	long pbase;
	int slot;
	
	if(phys || !Virtmode) {
		pbase = cr_vtop(base,-1);
		if (pbase == -1)
			fprintf(stderr,"getslot: %x is an invalid address\n",base);
		slot = ((addr&~MAINSTORE) - pbase) / size;
	}
	else slot = (addr - base) / size;
	if((slot >= 0) && (slot < max))
		return(slot);
	else return(-1);
}


/* file redirection */
int
redirect()
{
	int i;
	FILE *ofp;

	ofp = fp;
	if(opipe == 1) {
		fprintf(stdout,"file redirection (-w) option ignored\n");
		return;
	}
	if(fp = fopen(optarg,"a")) {
		fprintf(fp,"\n> ");
		for(i=0;i<argcnt;i++)
			fprintf(fp,"%s ",args[i]);
		fprintf(fp,"\n");
	}
	else {
		fp = ofp;
		error("unable to open %s\n",optarg);
	}
}


/*
 * putch() recognizes escape sequences as well as characters and prints the
 * character or equivalent action of the sequence.
 */
int
putch(c)
unsigned int  c;
{
	if(isgraph(c))
		fprintf(fp," %c ",c);
	else
		fprintf(fp," \\%#o ",c);
/*
	c &= 0377;
	if(c < 040 || c > 0176) {
		fprintf(fp,"\\");
		switch(c) {
		case '\0': c = '0'; break;
		case '\t': c = 't'; break;
		case '\n': c = 'n'; break;
		case '\r': c = 'r'; break;
		case '\b': c = 'b'; break;
		default:   c = '?'; break;
		}
	}
	else fprintf(fp," ");
	fprintf(fp,"%c ",c);
*/
}

/* sets process to input argument */
int
setproc()
{
	int slot;

	if((slot = strcon(optarg,'d')) == -1)
		error("\n");
	if((slot > vbuf.v_proc) || (slot < 0))
		error("%d out of range\n",slot);
	return(slot);
}

/* check to see if string is a symbol or a hexadecimal number */
int
isasymbol(string)
char *string;
{
	int i;

	for(i = (int)strlen(string); i > 0; i--)
		if(!isxdigit(*string++))
			return(1);
	return(0);
}


/* convert a string into a range of slot numbers */
int
range(max,begin,end)
int max;
long *begin,*end;
{
	int i,j,len,pos;
	char string[ARGLEN];
	char temp1[ARGLEN];
	char temp2[ARGLEN];

	strcpy(string,args[optind]);
	len = (int)strlen(string);
	if((*string == '-') || (string[len-1] == '-')){
		fprintf(fp,"%s is an invalid range\n",string);
		*begin = -1;
		return;
	}
	pos = strcspn(string,"-");
	for(i = 0; i < pos; i++)
		temp1[i] = string[i];
	temp1[i] = '\0';
	for(j = 0, i = pos+1; i < len; j++,i++)
		temp2[j] = string[i];
	temp2[j] = '\0';
	if((*begin = (int)stol(temp1)) == -1)
		return;
	if((*end = (int)stol(temp2)) == -1) {
		*begin = -1;
		return;
	}
	if(*begin > *end) {
		fprintf(fp,"%d-%d is an invalid range\n",*begin,*end);
		*begin = -1;
		return;
	}
	if(*end >= max) 
		*end = max - 1;
}


/*
 * print the privilege vector symbolically.
*/
void
prt_symbprvs(strp, vect)
char	*strp;
pvec_t	vect;
{
	register int	had_privs = 0;
	register int	prnt = 0;
	register int	k = 0;

	fprintf(fp,"%s", strp);
	if (all_privs_on(vect)) {
		fprintf(fp, "allprivs");
		++had_privs;
	}
	else {
		for (k = 0; k < NPRIVS; k++) {
			if ((pm_privbit(k) & vect)) {
				if ((prnt = prt_symprvs(k, prnt)))
					++had_privs;
			}
		}
	}
	if (!had_privs)
		fprintf(fp, "<none>");
	fprintf(fp,"\n");
}


/*
 * This routine actually does the symbolic printing.
*/
int
prt_symprvs(priv, printed)
int	priv;
int	printed;
{
	char	pbuf[BUFSIZ];
	char	*pbp = &pbuf[0];

	if (printed) {
		printed = 0;
		fprintf(fp,",");
	}
	if (privname(pbp, priv) != -1) {
		fprintf(fp,"%s", pbp);
		printed = 1;
	}
	return(printed);
}

/*
 * This routine returns 1 if all privileges are on in the privilege
 * vector passed.  Otherwise, it returns 0.
*/
int
all_privs_on(prv_vector)
pvec_t	prv_vector;
{
	register int	k;

	for (k = 0; k < NPRIVS; k++) {
		if (!(pm_privbit(k) & prv_vector))
			return(0);
	}
	return(k);
}

/*
 * If reading from dumpfile, construct memory layout from the file.
 */
void
findmemregs()
{
	extern int mem; /* file desc. for dumpfile */
	kcore_t header;
	off_t chunk_off, image_off;
	mreg_chunk_t mchunk;
	cr_mchunk_t *cr_mchunk, *new_chunk;
	vaddr_t addr;
	boolean_t eof = B_FALSE;
	int i;

	if (active || cr_mem_org != NULL)
		return;

	lseek(mem, 0, SEEK_SET);
	/*
	 * get the header to verify we have the dumpfile.
	 */
	if (read(mem, (caddr_t)&header, sizeof(header)) < sizeof(header))
		error("header not found in the dump file");

	if (header.k_magic != KCORMAG)
		error("wrong magic number in dump file\n");

	/*
	 * find out the number of memory chunks in the dumpfile.
	 */
	chunk_off =  (sizeof(header) + header.k_align) & ~(header.k_align - 1);
	image_off = chunk_off + sizeof(mchunk);
	if (image_off % header.k_align != 0)
		image_off = (image_off + header.k_align) & 
				~(header.k_align -1);

	cr_mchunk = cr_mem_org = (cr_mchunk_t *)calloc(1, sizeof(cr_mchunk_t));

	if (cr_mem_org == 0)
		error("calloc failed\n"); 

	addr = 0;

	for (;;) {
		if (lseek(mem, chunk_off, SEEK_SET) == -1)
			error("cannot seek chunk header from dumpfile\n"); 

		if (read(mem, (caddr_t)&mchunk, sizeof(mchunk)) < 
				sizeof(mchunk))
			error("cannot read chunk header from dumpfile\n");

		if (mchunk.mc_magic != MRMAGIC)
			error("expected chunk header not found\n");

		if (eof) {
			if (MREG_LENGTH(mchunk.mc_mreg[0]) != 0)
				error("incomplete dump\n");
			else
				break;
		}
		if (MREG_LENGTH(mchunk.mc_mreg[0]) == 0)
			break;
		/*
		 * The very first chunk cannot be a end marker!
		 */
		if (cr_mem_org == cr_mchunk &&
			MREG_LENGTH(mchunk.mc_mreg[0]) == 0)
			error("inconsistent memory layout\n");

		cr_mchunk->addr = addr;

		for (i = 0; i < NMREG; i++) {
			if (MREG_LENGTH(mchunk.mc_mreg[i]) == 0)
				break;
			cr_mchunk->cr_mregdesc[i].reg_addr = addr;
			addr += MREG_LENGTH(mchunk.mc_mreg[i]);
			cr_mchunk->cr_mregdesc[i].mreg = 
				mchunk.mc_mreg[i];
			if (MREG_TYPE(mchunk.mc_mreg[i]) == MREG_IMAGE) {
				cr_mchunk->cr_mregdesc[i].image_off = 
						image_off;
				image_off += MREG_LENGTH(mchunk.mc_mreg[i]);
			}
		}
		chunk_off = image_off;
		if (image_off % header.k_align != 0)
			chunk_off = (chunk_off + header.k_align) & 
						~(header.k_align - 1);
		/*
		 * More chunks to follow?
		 */
		if (i == NMREG)	{
			image_off = chunk_off + sizeof(mchunk);
			if (image_off % header.k_align != 0)
				image_off = (image_off + header.k_align) & 
						~(header.k_align -1);
			new_chunk = (cr_mchunk_t *)calloc(1,
							  sizeof(cr_mchunk_t));
			if (new_chunk == 0)
				error("calloc failed\n");
			cr_mchunk->next = new_chunk;
			cr_mchunk = new_chunk;
		} else
			eof = B_TRUE;
	}
	if (debugmode > 5) {
		for (i = 0; i < 5; i++) {
			printf("region = %d, type = %d, length = %lx ", 
				i, MREG_TYPE(cr_mchunk->cr_mregdesc[i].mreg),
				MREG_LENGTH(cr_mchunk->cr_mregdesc[i].mreg));

			printf("reg_addr = %lx i_off = %lx\n",
				cr_mchunk->cr_mregdesc[i].reg_addr,
				cr_mchunk->cr_mregdesc[i].image_off);
		}
	}
}

int
cr_read(paddr, buffer, cnt)
unsigned long paddr;
char *buffer;
unsigned long cnt;
{
	cr_mchunk_t *cr_mchunk;
	char *bp = buffer;
	int i, err;
	vaddr_t addr;
	size_t len;
	off_t startoff;

	lseek(mem, 0, SEEK_SET);

	for (cr_mchunk = cr_mem_org; cr_mchunk != NULL; 
					cr_mchunk = cr_mchunk->next) {
		/*
		 * Is the address in this chunk?
		 */
		if (cr_mchunk->next != NULL) {
			if (paddr >= cr_mchunk->next->addr)
				continue;
		}
		/*
		 * Find the right region in this chunk.
		 */

		for (i = 0; i < NMREG; i++) {
			/*
			 * if last region, then this is it.
			 */
			if (paddr >= (cr_mchunk->cr_mregdesc[i].reg_addr +
				MREG_LENGTH(cr_mchunk->cr_mregdesc[i].mreg)))
				continue;

			addr = cr_mchunk->cr_mregdesc[i].reg_addr;
			len = MREG_LENGTH(cr_mchunk->cr_mregdesc[i].mreg);
			len = addr + len - paddr;

			if (MREG_TYPE(cr_mchunk->cr_mregdesc[i].mreg) ==
				MREG_ZERO) {
				memset(bp, 0, min(cnt, len));
			} else if (MREG_TYPE(cr_mchunk->cr_mregdesc[i].mreg) ==
				 MREG_HOLE) {
					fprintf(stderr, 
					     "cr_read: non-existent addr.\n");
					return -1;
			} else {  /* has to be MREG_IMAGE */ 
				startoff = cr_mchunk->cr_mregdesc[i].image_off;
				startoff += paddr - addr;

				err = lseek(mem, startoff, SEEK_SET);

				if (err == -1) {
				  	fprintf(stderr, 
						"cr_read: seek failed\n");
					return -1;
				}
				if (read(mem, bp, min(cnt, len)) < 
					min(cnt, len)) {
					return -1;
				}
			}
			/*
			 * Have we satisfied the entire request?
			 */
			if (len >= cnt)
				return 0;
			else {
			  	bp += len;
				cnt -= len;
				paddr += len;
			}
		}
	}
	fprintf(stderr, "paddr not found in the layout\n");
	return -1;
}

/* get address for symbol which may reside in loadable kernel module */
void
getdsym(loc, name, fatal)
struct syment **loc;
char *name;
boolean_t fatal;
{
	extern struct syment dummyent;
	if (active || !*loc)
		if ((*loc = symsrch(name)) == NULL) {
			if (fatal)
				error("%s not found in symbol table\n",name);
			else
				*loc = &dummyent;
		}
}
