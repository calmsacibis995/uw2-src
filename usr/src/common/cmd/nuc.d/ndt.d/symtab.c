/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ndt.cmd:symtab.c	1.2"
#ident	"$Header: /SRCS/esmp/usr/src/nw/cmd/nuc.d/ndt.d/symtab.c,v 1.2 1994/01/31 21:52:16 duck Exp $"

/*	Copyright (c) 1993 Novell, Inc.  All Rights Reserved.      	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF NOVELL, INC.	*/
/*	The copyright notice above does not evidence any actual or 	*/
/*	intended publication of such source code.                  	*/

/* XXX  #ident	"@(#)crash:common/cmd/crash/symtab.c	1.1" */
/* XXX  #ident	"$Header: /SRCS/esmp/usr/src/common/cmd/crash/symtab.c,v 1.1.1.2 1993/11/08 17:48:29" */

/*
 * This file contains code for the crash functions: nm, ds, and ts, as well
 * as the initialization routine rdsymtab.
 */

#include "a.out.h"
#include "stdio.h"
#include "string.h"
#include "crash.h"
#include "malloc.h"
#include "sys/ksym.h"
#include "libelf.h"
#include	<sys/nwctrace.h>
#include	<sys/traceuser.h>

#include "ndt.h"

extern	SYMTAB_t		symtab[];
extern	int	nmlst_tstamp ;		/* namelist timestamp */
extern char *namelist;
extern short N_TEXT,N_DATA,N_BSS;	/* used in symbol search */
struct syment *stbl;			/* symbol table */
int symcnt;				/* symbol count */
char *strtbl;				/* pointer to string table */

int iscoff = 1;				/* presume namelist is COFF */



/* symbol table initialization function */

int
rdsymtab()
{
	FILE *np;
	struct filehdr filehdr;
	struct syment	*sp,
			*ts_symb ;
	struct scnhdr	scnptr ,
			boot_scnhdr ;
	int	i ,
		N_BOOTD ;
	char *str;
	long *str2;
	long strtblsize;

	/* see if we need to read the symbol table or will we get it on the fly
		from getksym */
	if(active)
		return(0);
	/*
	 * Open the namelist and associate a stream with it. Read the file into a buffer.
	 * Determine if the file is in the correct format via a magic number check.
	 * An invalid format results in a return to main(). Otherwise, dynamically 
	 * allocate enough space for the namelist. 
	 */

		
	if(!(np = fopen(namelist, "r")))
		fatal("cannot open namelist file\n");
	if(fread((char *)&filehdr, FILHSZ, 1, np) != 1)
		fatal("read error in namelist file\n");
	if(filehdr.f_magic != FBOMAGIC) {
		rewind(np);
		if ((rdelfsym(np) != 0))
			fatal("namelist not in a.out format\n");
	}
	if (iscoff) {
	/*
	 * Read the section headers to find the section numbers
	 * for .text, .data, and .bss.  First seek past the file header 
	 * and optional header, then loop through the section headers
	 * searching for the names .text, .data, and .bss.
	 */
	N_TEXT=0;
	N_DATA=0;
	N_BSS=0;
	N_BOOTD=0 ;
	if(fseek(np, (long)(FILHSZ + filehdr.f_opthdr), 0) != 0
	  && fread((char *)&filehdr, FILHSZ, 1, np) != 1)
		fatal("read error in section headers\n");

	for(i=1; i <= (int)filehdr.f_nscns; i++)
	{
		if(fread(&scnptr, SCNHSZ, 1, np) != 1)
			fatal("read error in section headers\n");

		if(strcmp(scnptr.s_name,_TEXT) == 0)
			N_TEXT = i ;
		else if(strcmp(scnptr.s_name,_DATA) == 0)
			N_DATA = i ;
		else if(strcmp(scnptr.s_name,_BSS) == 0)
			N_BSS = i ;
		else if(strcmp(scnptr.s_name,"boot") == 0)
		{
			/* save data section for later processing */
			N_BOOTD = 1 ;
			boot_scnhdr = scnptr ;
		}

	}
	if(N_TEXT == 0 || N_DATA == 0 || N_BSS == 0) 
		fatal(".text, .data, or .bss was not found in section headers\n");

	/*
	 * Now find the string table (if one exists) and
	 * read it in.
	 */
	if(fseek(np,filehdr.f_symptr + filehdr.f_nsyms * SYMESZ,0) != 0)
		fatal("error in seeking to string table\n");
	
	if(fread((char *)&strtblsize,sizeof(int),1,np) != 1)
		fatal("read error for string table size\n");
	
	if(strtblsize)
	{
		if(!(strtbl = (char *)malloc((unsigned)strtblsize)))
			fatal("cannot allocate space for string table\n");

		str2 = (long *)strtbl;
		*str2 = strtblsize;

		for(i = 0,str = (char *)((int)strtbl + (int)sizeof(long)); i < strtblsize - sizeof(long); i++, str++)
			if(fread(str, sizeof(char), 1, np) != 1)
				fatal("read error in string table\n");
	}
	else
		str = 0;

	if(!(stbl=(struct syment *)malloc((unsigned)(filehdr.f_nsyms*SYMESZ))))
		fatal("cannot allocate space for namelist\n");

	/*
	 * Find the beginning of the namelist and read in the contents of the list.
	 *
	 * Additionally, locate all auxiliary entries in the namelist and ignore.
	 */

	fseek(np, filehdr.f_symptr, 0);
	symcnt = 0;
	for(i=0, sp=stbl; i < filehdr.f_nsyms; i++, sp++) {
		symcnt++;
		if(fread(sp, SYMESZ, 1, np) != 1)
			fatal("read error in namelist file\n");
		if(sp->n_zeroes == 0) 
			sp->n_offset = (long) (sp->n_offset+strtbl);
		if(sp->n_numaux) {
			fseek(np,(long)AUXESZ*sp->n_numaux,1);
			i += sp->n_numaux;
		}
	}
	/* save timestamp from data space of namelist file */
		
	if(!(ts_symb = symsrch("crash_sync")) || !N_BOOTD)
		nmlst_tstamp = 0 ;
	else
	{
		if(fseek(np,(long)(boot_scnhdr.s_scnptr + (ts_symb -> n_value - boot_scnhdr.s_paddr)),0) != 0)
			fatal("could not seek to namelist timestamp\n") ;
		if(fread((char *)&nmlst_tstamp,sizeof(int),1,np) != 1)
			fatal("could not read namelist timestamp\n") ;
	}
	} /* iscoff */
	fclose(np);
}


/* find symbol */
struct syment *
findsym(addr)
unsigned long addr;
{
	struct syment *sp;
	struct syment *save;
	unsigned long value;
	char name[MAXSYMNMLEN];
	unsigned long offset;
	char *tname;

	if(active) {
		if(getksym(name,&addr,&offset) != 0)
			return(NULL);
		sp = findsp(name);
		if(sp->n_offset == 0) {
			sp->n_offset = (long) calloc(strlen(name) + 1,1);
			strcpy((char *) sp->n_offset,name);
		}
		offset =0;
		(void) getksym(name,&sp->n_value,&offset);
		sp->n_type = offset;
		sp->n_sclass =  C_EXT;
		return(sp);
	}

	value = MAINSTORE;
	save = NULL;

	for(sp = stbl; sp < &stbl[symcnt]; sp++) {
		if(((sp->n_sclass == C_EXT) || (sp->n_sclass == C_STAT)) && 
			((unsigned long)sp->n_value <= addr)
		  && ((unsigned long)sp->n_value > value)) {
			value = (unsigned long)sp->n_value;
			save = sp;
		}
	}
	if(save && save->n_zeroes) {
		tname = calloc(SYMNMLEN+1,1);
		strncpy(tname,save->n_name,SYMNMLEN);
		save->n_zeroes = 0;
		save->n_offset = (long) tname;
	}
	return(save);
}

#ifdef XXX
/* get arguments for ds and ts functions */
int
getsymbol()
{
	int c;

	optind = 1;
	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(args[optind]) {
		do {prsymbol(args[optind++], 0);
		}while(args[optind]);
	}
	else longjmp(syn,0);
}

/* print result of ds and ts functions */
int
prsymbol(string, addr)
char *string;
long addr;
{
	struct syment *sp = NULL;

	if (!addr) {
		if((addr = strcon(string,'h')) == -1)
			error("\n");
	}

	if(!(sp = findsym((unsigned long)addr))) {
		if (string)
			prerrmes("%s does not match\n",string);
		else
			prerrmes("%x does not match\n",addr);
		return;
	}

	fprintf(fp,"%s",(char *) sp->n_offset);		

	fprintf(fp," + %x\n",addr - (long)sp->n_value);
}
#endif XXX


/* search symbol table */
struct syment *
symsrch(s)
char *s;
{
	struct syment *sp;
	struct syment *found;
	char *name;
	unsigned long info;
	unsigned long value = 0;
	char *tname;

	if(active) {
		if(getksym(s,&value,&info) != 0)
			return(NULL);
		sp = findsp(s);
		sp->n_value = value;
		if(sp->n_zeroes) {
			tname = calloc(SYMNMLEN+1,1);
			strncpy(tname,sp->n_name,SYMNMLEN);
			sp->n_zeroes = 0;
			sp->n_offset = (long) tname;
		}
		sp->n_type = info;
		sp->n_sclass =  C_EXT;
		return(sp);
	}
	found = 0;


	for(sp = stbl; sp < &stbl[symcnt]; sp++) {
		if(((sp->n_sclass == C_EXT) || (sp->n_sclass == C_STAT)) &&
		   ((unsigned long)sp->n_value >= MAINSTORE)) {
			if(sp->n_zeroes) {
				name = calloc(SYMNMLEN+1,1);
				strncpy(name,sp->n_name,SYMNMLEN);
				sp->n_zeroes = 0;
				sp->n_offset = (long) name;
			} else
				name = (char *) sp->n_offset;
			if(!strcmp(name,s)) {
				found = sp;
				break;
			}
		}
	}
	return(found);
}

#ifdef XXX
/* get arguments for nm function */
int
getnm()
{
	int c;

	optind = 1;
	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(args[optind]) 
		do { prnm(args[optind++]);
		}while(args[optind]);
	else longjmp(syn,0);
}


/* print result of nm function */
int
prnm(string)
char *string;
{
	char *cp;
	struct syment *sp;

	if(!(sp = symsrch(string))) {
		prerrmes("%s does not match in symbol table\n",string);
		return;
	}
	fprintf(fp,"%s   %08.8lx  ",string,sp->n_value);

	if(active) {
		if(sp->n_type == STT_FUNC)
			cp = "text";
		else if(sp->n_type == STT_OBJECT)
			cp = "data";
		else
			cp = "type unknown";
	} else if (iscoff) {
		if      (sp -> n_scnum == N_TEXT)
			cp = " text";
		else if (sp -> n_scnum == N_DATA)
			cp = " data";
		else if (sp -> n_scnum == N_BSS)
			cp = " bss";
		else if (sp -> n_scnum == N_UNDEF)
			cp = " undefined";
		else if (sp -> n_scnum == N_ABS)
			cp = " absolute";
		else
			cp = " type unknown";

	} else {	/* Is ELF */

		if	(sp->n_scnum == N_ABS)
			cp = " absolute";
		/*
		else if (sp->n_value < 0x02004001)
			cp = " gate segment";
		else if (sp->n_value < 0x40000001)
			cp = " start segment";
		else if (sp->n_value < 0x40160001)
			cp = " text segment";
		else if (sp->n_value < 0x401a0001)
			cp = " data segment";
		else if (sp->n_value < 0x40300000)
			cp = " bss segment";
		*/
		else
			cp = " type unknown";
	}

	fprintf(fp,"%s (%s symbol)\n", cp,
		(sp->n_sclass == C_EXT ? "global" : "static/local"));
}
#endif XXX

/*
**	Read symbol table of ELF namelist
*/

rdelfsym(fp)
FILE *fp;
{
	register int i;
	register Elf32_Sym *sy;
	register struct syment *sp;
	struct syment *ts_symb;
	Elf *elfd;
	Elf_Scn	*scn;
	Elf32_Shdr *eshdr;
	Elf32_Sym *symtab;
	Elf_Data *data;
	Elf_Data *strdata;
	int fd;
	int nsyms;

        if (elf_version (EV_CURRENT) == EV_NONE) {
		fatal("ELF Access Library out of date\n");
	}

	fd = fileno(fp);

	if ((lseek(fd, 0L, 0)) == -1L) {
		fatal("Unable to rewind namelist file\n");
	}

        if ((elfd = elf_begin (fd, ELF_C_READ, NULL)) == NULL) {
		fatal("Unable to elf begin\n");
	}

	if ((elf_kind(elfd)) != ELF_K_ELF) {
		elf_end(elfd);
		return (-1);
	}

	scn = NULL;
	while ((scn = elf_nextscn(elfd, scn)) != NULL) {

		if ((eshdr = elf32_getshdr(scn)) == NULL) {
			elf_end(elfd);
			fatal("cannot read section header\n");
		}

		if (eshdr->sh_type == SHT_SYMTAB) {
			break;		/* Can only do 1 symbol table */
		}
	}

		/* Should have scn and eshdr for symtab */

	data = NULL;
	if (((data = elf_getdata(scn, data)) == NULL) ||
		(data->d_size == 0) || (!data->d_buf)) {
			elf_end(elfd);
			fatal("can not read symbol table\n");
	}

	symtab = (Elf32_Sym *)data->d_buf;

	nsyms = data->d_size / sizeof(Elf32_Sym);

	/*
	**	get string table
	*/

	if ((scn = elf_getscn(elfd, eshdr->sh_link)) == NULL) {
		elf_end(elfd);
		fatal("ELF strtab read error\n");
	}

	strdata = NULL;
	if (((strdata = elf_getdata(scn, strdata)) == NULL) ||
		(strdata->d_size == 0) || (!strdata->d_buf)) {
			elf_end(elfd);
			fatal("string table read failure\n");
	}

	if ((strtbl = malloc(strdata->d_size)) == NULL)
		fatal("cannot allocate space for string table\n");

	(void)memcpy(strtbl, strdata->d_buf, strdata->d_size);

	if((stbl=(struct syment *)malloc((unsigned)(nsyms*sizeof(SYMENT)))) == NULL)
		fatal("cannot allocate space for namelist\n");

	/*
	**	convert ELF symbol table info to COFF
	**	since rest of pgm uses COFF
	*/

	symcnt = 0;
	sp = stbl;
	sy = symtab;
	for (i = 0; i < nsyms; i++, sy++) {

		if ((ELF32_ST_TYPE(sy->st_info)) == STT_FILE)
			continue;

		if ((ELF32_ST_TYPE(sy->st_info)) == STT_SECTION)
			continue;

		sp->n_zeroes = 0L;
		sp->n_offset = (long) (sy->st_name + strtbl);
		sp->n_value = sy->st_value;
		sp->n_scnum = sy->st_shndx;
		sp->n_type = ELF32_ST_TYPE(sy->st_info);
		sp->n_sclass =  ELF32_ST_BIND(sy->st_info);
		sp->n_numaux = 0;

		if (sp->n_scnum == SHN_ABS)
			sp->n_scnum = N_ABS;

		if (sp->n_sclass == STB_GLOBAL)
			sp->n_sclass = C_EXT;
		else
			sp->n_sclass = C_STAT;

		sp++;
		symcnt++;
	}

	/* Get time stamp */

	if(!(ts_symb = symsrch("crash_sync")))
                nmlst_tstamp = 0 ;
        else {

		if ((scn = elf_getscn(elfd, ts_symb->n_scnum)) == NULL) {
			elf_end(elfd);
			fatal("ELF timestamp scn read error\n");
		}

		if ((eshdr = elf32_getshdr(scn)) == NULL) {
			elf_end(elfd);
			fatal("cannot read timestamp section header\n");
		}

		if ((lseek(fd,
			(long)(ts_symb->n_value - eshdr->sh_addr + eshdr->sh_offset),
				0)) == -1L)
                        fatal("could not seek to namelist timestamp\n") ;

                if ((read(fd, (char *)&nmlst_tstamp, sizeof(nmlst_tstamp))) !=
				sizeof(nmlst_tstamp))
                        fatal("could not read namelist timestamp\n");
        }

	iscoff = 0;

	elf_end(elfd);

	return(0);
}

static struct symlist {
	struct syment *sl_ent;
	struct symlist *sl_next;
} *slhead = NULL;

struct syment *
findsp(name)
char *name;
{
	struct symlist *tsl;
	char buf[SYMNMLEN+1];

	tsl = slhead;
	while(tsl) {
		if(strcmp(name,(char *) tsl->sl_ent->n_offset) == 0)
			return(tsl->sl_ent);
		tsl = tsl->sl_next;
	}
	tsl = (struct symlist *) malloc(sizeof(struct symlist));
	tsl->sl_ent = (struct syment *) calloc(sizeof(struct syment), 1);
	tsl->sl_next = slhead;
	slhead = tsl;
	return(tsl->sl_ent);
}

#ifdef XXX
int
getsymval()
{
	int c;
	int proc = Procslot;
	long value=(-1);
	char * db_sym_off();
	struct syment* sp;

	optind = 1;
	while((c = getopt(argcnt,args,"w:s:r")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}

	if(args[optind]) {
		if(*args[optind] == '(') {
			if((value = eval(++args[optind])) == -1)
				error("\n");
			else
				fprintf(fp,"%s: %s\n",args[optind],
							db_sym_off(value));
		} else if(sp = symsrch(args[optind])) {
				fprintf(fp,"%s: %x\n",args[optind], 
						sp->n_value);
		} else if(isasymbol(args[optind]))   {
				error("%s not found in symbol table\n",
								args[optind]);
		} else if((value = strcon(args[optind],'h')) == -1) {
				error("\n");
		} else
			fprintf(fp,"%s: %s\n",args[optind],db_sym_off(value));
	}
}
#endif XXX

char *
findsyminfo(vaddr_t value, vaddr_t *loc_p, int *valid_p)
{

	static struct syment *se;
	extern struct syment* findsym();
	
	se = findsym(value);

	if (se) {
		
		*valid_p = 1;
		*loc_p = (vaddr_t) se->n_value;
		return((char*) se->n_offset);

	} else {
		*valid_p = *loc_p = 0;
		return("ZERO");
	}
}

char *
db_sym_off(vaddr_t addr, unsigned cutoff)
{
	char *p;
	vaddr_t sym_addr;
	int valid;
	static char line[80];

	p = findsyminfo(addr, &sym_addr, &valid);
	if (!valid){
  		sprintf(line,"%8.8x", addr);
		return(line);
	}

	if (addr != sym_addr) {
		if( (addr - sym_addr) > cutoff )
			sprintf(line,"%8.8x", addr);
		else
			sprintf(line,"%s+%x",p,addr-sym_addr);

	} else
		sprintf(line,"%s",p);

	return line;
}


vaddr_t
findsymval(vaddr_t value)
{
	vaddr_t	loc;
	int	valid;

	(void) findsyminfo(value, &loc, &valid);
	return valid? loc : (ulong)0;
}
