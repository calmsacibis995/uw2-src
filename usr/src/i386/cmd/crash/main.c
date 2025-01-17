/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:i386/cmd/crash/main.c	1.10"

/*
 * This file contains code for the crash functions:  ?, help, redirect,
 * and quit, as well as the command interpreter.
 */

extern long KPD;

#include <sys/param.h>
#include <a.out.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/fs/s5dir.h>
#include <sys/var.h>
#include <sys/user.h>
#include <setjmp.h>
#include <locale.h>
#include "crash.h"

int mem;			/* file descriptor for dump file */
char *namelist = "/stand/unix";
char *dumpfile = "/dev/mem";
char *lmodpath = (char *)NULL;

FILE *fp;				/* output file pointer */
FILE *rp;				/* redirect file pointer */
struct var vbuf;			/* var structure buffer */
int Virtmode = 1;			/* virtual or physical mode flag*/

struct syment *File_list,*Vnode,*Vfs,*V,*Panic,
	*Streams,*Kmeminfo,*Km_pools;	/* namelist symbol pointers */

short N_TEXT;		/* used in symtab.c */
short N_DATA;		/* used in symtab.c */
short N_BSS;		/* used in symtab.c */
int active;	/* Flag set if crash is examining an active system */
jmp_buf	jmp,syn;	/* labels for jump */
void exit();


/* function calls */
extern int getas(),getbufhdr(),getbuffer(),getcallout(),getdis(),
	getinode(),get_cdfs_inode(), getkfp(),getlcks(),getpage(),
	getmap(),getvfsarg(),getnm(),getod(),getpcb(),
	getproc(),getqrun(),getqueue(),getquit(),getptbl(),getprnode(),
	getsnode(),
	getsrmount(),getstack(),getstat(),getstream(),getstrstat(),
	gettrace(),getsymbol(),gettty(),getuser(),getvar(),getvnode(),getvtop(),
	getfuncs(),getbase(),gethelp(),getsearch(), getdeflwp(),
	getsearch(),getfile(),getdefproc(),getmode(),getredirect(),
	getsize(),getfindslot(),getfindaddr(),getvfssw(),getlinkblk(),
	getresrc(),getpty(),gethrt(),getclass(),getasync(),
	gettsdptbl(),getrtdptbl(),get_vxfs_inode(),
	getdispq(),gettslwp(),getrtproc(),getkmastat(),get_sfs_inode(),
	getfpriv(),getabuf(),getlidcache(),getsymval(),getplocal(),
	/*
	 * i386 specifics are following
	 */
	getldt(),getgdt(),getidt(),getpanic(),gettest(),runq(),getplock(),
	getengine();


/* function table */
/* entries with NA description fields should be removed in next release */
struct func functab[] = {
	"abuf","[-b|-c|-d|-x|-o][-w filename]",
		getabuf,"audit buffer data",	
	"as","[-e] [-f] [-wfilename] [proc[s]]",
		getas,"address space structures",
	"b"," ",getbuffer,"(buffer)",
	"base","[-wfilename] number[s]",
		getbase,"base conversions",
	"buf"," ",getbufhdr,"(bufhdr)",
	"buffer","[-wfilename] [-b|-c|-d|-x|-o|-i] (bufferslot |[-p] st_addr)",
		getbuffer,"buffer data",
	"bufhdr","[-f] [-wfilename] [[-p] tbl_entry[s]]",
		getbufhdr,"buffer headers",
	"c"," ",getcallout,"(callout)",
	"callout","[-wfilename]",
		getcallout,"callout table",
	"class","[-wfilename] [tbl_entry[s]]",
		getclass,"class table",
	"defproc","[-wfilename] [-c | slot]",
		getdefproc,"set default process slot",
	"deflwp", "[-wfilename] [-c | slot]",
		getdeflwp, "set default lwp slot",
	"dis","[-wfilename] [-a] -c | st_addr [count]",
		getdis,"disassembler",
	"dispq","[-wfilename] [-l|-g] [tbl_entry[s]]",
		getdispq, "dispq table",
	"ds","[-wfilename] virtual_address[es]",
		getsymbol,"data address namelist search",
	"eng","[-wfilename] eng_num",
		getengine,"change or display current engine",
	"f"," ",getfile,"(file)",
	"file","[-e] [-f] [-wfilename] [[-p] address[es]]",
		getfile,"file table",
	"filepriv","[-n] [-wfilename]", getfpriv,
		"kernel privilege table",
	"fprv"," ", getfpriv, "(filepriv)",
	"findaddr","[-wfilename] table slot",
		getfindaddr,"find address for given table and slot",
	"findslot","[-wfilename] virtual_address[es]",
		getfindslot,"find table and slot number for given address",
	"fs"," ",getvfssw,"(vfssw)",
	"help","[-wfilename] function[s]",
		gethelp,"help function",
	"hrt","[-wfilename]",
		gethrt,"high resolution timers",
	"i"," ",getinode,"(inode)",
	"inode","[-e] [-f] [-wfilename] [[-p] tbl_entry[s]]",
		getinode,"inode table",
	"kfp","[-wfilename] [-sprocess] [-r | value]",
		getkfp,"frame pointer for start of stack trace",
	"kmastat","[-wfilename]",
		getkmastat,"kernel memory allocator statistics",


	"plocal","[-wfilename]", getplocal, "processor local data",
	"lck","[-e] [-wfilename] [[-p] tbl_entry[s]]",
		getlcks,"record lock tables",
	"lidcache"," ",getlidcache,"lid cache",
	"linkblk","[-e] [-wfilename] [[-p] linkblk_addr[s]]",
		getlinkblk,"linkblk table",

	"m"," ",getvfsarg,"(vfs)",

	"map","[-wfilename] mapname[s]",
		getmap,"map structures",
	"mode","[-wfilename] [v | p]",
		getmode,"address mode",
	"mount"," ",getvfsarg,"(vfs)",
	"nm","[-wfilename] symbol[s]",
		getnm,"name search",
	"od","[-wfilename] [-c|-d|-x|-o|-a|-h] [-l|-t|-b] [-sprocess] [-p] st_addr [count]",
		getod,"dump symbol values",
	"p"," ",getproc,"(proc)",
	"ps"," ",getproc,"(proc)",
	"page","[-e] [-wfilename] [[-p] tbl_entry[s]]",
		getpage,"page structure",
	"pcb","[-wfilename] [[-u | -k] [process] | -i [-p] st_addr]",
		getpcb,"process control block",
	"pcinode", "[-e] [-wfilename] [[-p] tbl_entry[s]]",
		get_cdfs_inode,"(cdfs inode)",
	"prnode","[-e] [-wfilename] [[-p] tbl_entry[s]]",
		getprnode,"proc node",
	"proc","[-e] [-f[-n]] [-wfilename] [([-p] [-a] tbl_entry | #procid)... | -r]",
		getproc,"process table",
	"ptbl","[-e] [-wfilename] [-sprocess] (section segment|[-p] st_addr [count])",
		getptbl,"page tables",
	"pty","[-e] [-f] [-wfilename] [-s] [-h] [-l] [([-p] tbl_entry)]",
		getpty,"pty structure",
	"q"," ",getquit,"(quit)",
	"qrun","[-wfilename]",
		getqrun,"list of servicable stream queues",
	"queue","[-e] [-f] [-wfilename] [[-p] queue_addr[s]]",
		getqueue,"allocated stream queues",
	"quit"," ",
		getquit,"exit",
	"rd"," ",getod,"(od)",
	"redirect","[-wfilename] [-c | filename]",
		getredirect,"output redirection",
	"resource","[-wfilename]", getresrc,"resource list",
	"rtdptbl","[-wfilename] [tbl_entry[s]]",
		getrtdptbl, "real time dispatcher parameter table",
	"rtproc","[-e] [-wfilename] [tbl_entry[s]]",
		getrtproc,"real time process table",
	"s"," ",getstack,"(stack)",
	"search","[-wfilename] [-mmask] [-sprocess] pattern [-p] st_addr length",
		getsearch,"memory search",
	"si"," ",get_sfs_inode,"(sfs/ufs inode)",
	"sinode","[-e] [-f] [-wfilename] [[-p] tbl_entry[s]]",
		get_sfs_inode,"sfs inode table",
	"size","[-x] [-wfilename] structurename[s]",
		getsize,"symbol size",
	"snode","[-e] [-f] [-wfilename] [[-p] tbl_entry[s]]",
		getsnode,"special node",
	"srmount","[-wfilename] [-p] srmount_addr[s]",
		getsrmount,"server mount list",
	"stack","[-wfilename] [[-u | -k] [process] | -i [-p] st_addr]",
		getstack,"stack dump",
	"stat","[-wfilename]",
		getstat,"dump statistics",
	"stream","[-e] [-f] [-wfilename] [[-p] stream_addr[s]]",
		getstream,"allocated stream table slots",
	"strstat","[-wfilename]",
		getstrstat,"streams statistics",
	"symval","[-wfilename]",
		getsymval,"address to symbol translation",
	"sym","[-wfilename]",
		getsymval,"address to symbol translation",
	"t"," ",gettrace,"(trace)",
	"trace","[-wfilename] [[-r] [process] | -i [-p] st_addr]",
		gettrace,"kernel stack trace",
	"ts","[-wfilename] virtual_address[es]",
		getsymbol,"text address namelist search",
	"tsdptbl","[-wfilename] [tbl_entry[s]]",
		gettsdptbl, "time sharing dispatcher parameter table",
	"tslwp","[-e] [-wfilename] [tbl_entry[s]]",
		gettslwp,"time sharing hash table",
	"tsproc"," ",gettslwp,"(tslwp)",
	"tty","[-e] [-f] [-wfilename] [-l] [-ttype [[-p] tbl_entry[s]] | [-p] st_addr]",
		gettty,"tty structures (valid types: pp, iu)",
	"u"," ",getuser,"(user)",
	"user","[-f] [-wfilename] [process]",
		getuser,"uarea",
	"v"," ",getvar,"(var)",
	"var","[-wfilename]",
		getvar,"system variables",
	"vfs","[-e] [-wfilename] [[-p] address[es]]",
		getvfsarg,"mounted vfs list",
	"vfssw","[-wfilename] [[-p] tbl_entry[s]]",
		getvfssw,"virtual file system switch table",
	"vi"," ",get_vxfs_inode,"(vxfs inode)",
	"vinode","[-e] [-f] [-A | -a] [-l] [-wfilename] [[-p] tbl_entry[s]]",
		get_vxfs_inode,"vxfs inode table",
	"vnode","[-wfilename] [-p] vnode_addr[s]",
		getvnode,"vnode list",
	"vtop","[-wfilename] [-sprocess] st_addr[s]",
		getvtop,"virtual to physical address",

	/*
	 * i386 specifics
	 */
	"ldt","[-e] [-wfilename] [process [slot [count]]]",
		getldt,"print local descriptor table",
	"idt","[-e] [-wfilename] [slot [count]]",
		getidt,"print interrupt decriptor table",
	"gdt","[-e] [-wfilename] [slot [count]]",
		getgdt,"print global decriptor table",
	"panic","[-wfilename] [process]", getpanic, "print panic information",
	"test","[mode]", gettest , "enter debug mode",

	/*
	 * again generic ones
	 */
	"?","[-wfilename]",
		getfuncs,"print list of available commands",
	"!cmd"," ",NULL,"escape to shell",
	"hdr"," ",getbufhdr,"NA",
	"files"," ",getfile,"NA",
	"fp"," ",getkfp,"NA",
	"r9"," ",getkfp,"NA",
	"mnt"," ",getvfsarg,"NA",
	"dump"," ",getod,"NA",
	"ps"," ",getproc,"NA",
	"k"," ",getstack,"NA",
	"kernel"," ",getstack,"NA",
	"stk"," ",getstack,"NA",
	"ad"," ",gettty,"NA",
	"con"," ",gettty,"NA",
	"term"," ",gettty,"NA",
	"u_area"," ",getuser,"NA",
	"uarea"," ",getuser,"NA",
	"ublock"," ",getuser,"NA",
	"tunable"," ",getvar,"NA",
	"tunables"," ",getvar,"NA",
	"tune"," ",getvar,"NA",
	"calls"," ",getcallout,"NA",
	"call"," ",getcallout,"NA",
	"timeout"," ",getcallout,"NA",
	"time"," ",getcallout,"NA",
	"tout"," ",getcallout,"NA",
	NULL,NULL,NULL,NULL
};

char *args[NARGS];		/* argument array */
int argcnt;			/* argument count */
char outfile[100];		/* output file for redirection */
int tabsize;			/* size of function table */

/* main program with call to functions */
main(argc,argv)
int argc;
char **argv;
{
	struct func *a,*f;
	int c,i,found;
	int arglength;
	int   rwflag  = O_RDONLY;
	char *options = "WD:d:n:w:k:m:";
	char usage[] = "usage: crash [-d dumpfile] [-n namelist] [-m modpath]\n";

	(void)setlocale(LC_ALL, "");
	if(setjmp(jmp))
		exit(1);
	fp = stdout;
	strcpy(outfile,"stdout");

	for (tabsize = 0,f = functab; f->name; f++,tabsize++) 
		if (!strcmp(f->description,"NA"))  /* remove in next release */
			break;

	while((c = getopt(argc,argv,options)) !=EOF) {
		switch(c) {
			case 'W' : 	rwflag = O_RDWR;
					break;

			case 'D' : 	debugmode = atoi(optarg);
					setbuf(stdout,NULL);
					setbuf(stderr,NULL);
					break;

			case 'd' :	dumpfile = optarg;
				 	break;

			case 'n' : 	
					namelist = optarg; 
					break;

			case 'k' :
					sscanf(optarg,"%x",&KPD);
					break;
				
			case 'm' :	lmodpath = optarg;
					break;
			
			case 'w' : 	strncpy(outfile,optarg,ARGLEN);
					if (!(rp = fopen(outfile,"a")))
						fatal("unable to open %s\n",
							outfile);
					break;

			default  :	fatal(usage); 
					break;
		}
	}
	if (rp)
		fprintf(rp,"dumpfile = %s, namelist = %s, outfile = %s\n",
			dumpfile,namelist,outfile);

	fprintf(fp,"dumpfile = %s, namelist = %s, outfile = %s\n", dumpfile,
		namelist,outfile);

	init(rwflag);

	setjmp(jmp);

	for (;;) {
		getcmd();
		if(argcnt == 0)
			continue;
		if (rp) {
			fp = rp;
			fprintf(fp,"\n> ");
			for (i = 0;i<argcnt;i++)
				fprintf(fp,"%s ",args[i]);
			fprintf(fp,"\n");
		}
		found = 0;
		for (f = functab; f->name; f++) 
			if(!strcmp(f->name,args[0])) {
				found = 1;
				break;
			}
		if (!found) {
			arglength = strlen(args[0]);
			for(f = functab; f->name; f++) {
				if(!strcmp(f->description,"NA"))
					break;     /* remove in next release */
				if(!strncmp(f->name,args[0],arglength)) {
					found++;
					a = f;
				}
			}
			if(found) {
				if (found > 1)
					error("%s is an ambiguous function name\n",args[0]);
				else f = a;
			}	
		}
		if (found) {
			if (!strcmp(f->description,"NA")) /* remove in next release */
				pralias(f);
			if (setjmp(syn)) {
				while(getopt(argcnt,args,"") != EOF);
				if (*f->syntax == ' ') {
					for (a = functab;a->name;a++)
						if((a->call == f->call) &&
						(*a->syntax != ' '))
							error("%s: usage: %s %s\n",f->name,f->name,a->syntax);
				}
				else error("%s: usage: %s %s\n",f->name,f->name,f->syntax);
			}
			else (*(f->call))();
		}
		else
			prerrmes("unrecognized function name\n");

		fflush(fp);
		resetfp();
	}
}
