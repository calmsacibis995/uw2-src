/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)fur:common/cmd/fur/fur.c	1.6"
#ident	"$Header:"

#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#ifdef __STDC__
#include <stdarg.h>
#include <stdlib.h>
#else
#include <varargs.h>
#include <malloc.h>
#endif
#include <libelf.h>
#include "fur.h"

static Elf32_Sym	**Notypes;
static ulong		Nnotypes;
static Elf32_Sym	**Syms;
static ulong		Nsyms;
static ulong		*Names;
static int			text_ptr = 0; /* used as index into text_newdata */
static Elf32_Shdr	*text_shdr;	/* section header of the target sect */
static Elf_Data		*str_data;
static Elf_Data 	*text_data;	/* input text section */


#define NO_SUCH_ADDR 0xffffffff

char *prog;				/* program name */
struct section *esections;		/* list of section in target file */
int debugflg =0;

#ifdef __STDC__
void usage(char *);
#else
void usage();
typedef enum { B_FALSE, B_TRUE } boolean_t;
#endif

static void *Ret;

#define ALIGN(a, b) ((b == 0) ? (a) : ((((a) +(b) -1) / (b)) * (b)))

#define MALLOC(X) ((Ret = (void *) malloc(X)) ? Ret : out_of_memory())
#define REALLOC(S, X) ((Ret = (void *) realloc(S, X)) ? Ret : out_of_memory())
#define CALLOC(S, X) ((Ret = (void *) calloc(S, X)) ? Ret : out_of_memory())

#define NAME(SYM) (((char *) str_data->d_buf) + (SYM)->st_name)
#define CURADDR(SYM) (((struct text_info *) (SYM)->st_value)->ti_curaddr)
#define NEWADDR(SYM) (((struct text_info *) (SYM)->st_value)->ti_newaddr)
#define USESIZE(SYM) (((struct text_info *) (SYM)->st_value)->ti_usesize)
#define FILENAME(SYM) (((struct text_info *) (SYM)->st_value)->ti_filename)
#define DATAPTR(SYM) (((struct text_info *) (SYM)->st_value)->ti_data)


static void *
out_of_memory()
{
	fprintf(stderr, "Program ran out of memory\n");
	exit(1);
	return(0);
}

static int
comp_orig_addr(const void *v1, const void *v2)
{
	return(CURADDR(*((Elf32_Sym **) v1)) - CURADDR(*((Elf32_Sym **) v2)));
}
static int
comp_new_addr(const void *v1, const void *v2)
{
	return(NEWADDR(*((Elf32_Sym **) v1)) - NEWADDR(*((Elf32_Sym **) v2)));
}

static void
setup_addr(int entno, int warn)
{
	int master;

	/* First, see if this has already been assigned an addr */
	if (NEWADDR(Syms[entno]) != NO_SUCH_ADDR) {
		if (warn)
			fprintf(stderr, "Warning: address for '%s' already computed\n\tThis means that '%s' appears twice in the list or a WEAK/STRONG\n\tpartner is already in the list\n", NAME(Syms[entno]), NAME(Syms[entno]));
		return;
	}

	if (debugflg)
		fprintf(stderr, "Computing position for %s\n", NAME(Syms[entno]));
	/* Find master entry */
	while ((entno < Nsyms) && (USESIZE(Syms[entno]) == 0))
		entno++;

	if (entno == Nsyms)
		error("Internal Error\n");

	master = entno;
	text_ptr = ALIGN((long) text_ptr, text_shdr->sh_addralign);
	NEWADDR(Syms[entno]) = (Elf32_Addr) text_ptr;

	/* Set up slaves */
	while ((entno > 0) && (USESIZE(Syms[--entno]) == 0))
		NEWADDR(Syms[entno]) = (Elf32_Addr) text_ptr;

	text_ptr += USESIZE(Syms[master]);
}

static void
compute_sizes()
{
	int i;
	
	for (i = 0; i < Nsyms - 1; i++)
		USESIZE(Syms[i]) = CURADDR(Syms[i+1]) - CURADDR(Syms[i]);

	USESIZE(Syms[i]) = text_data->d_size - CURADDR(Syms[i]);
}

static int
pnames()
{
	int i;

	fprintf(stderr, "Current ordering:\n");
	for (i = 0; i < Nsyms; i++)
		fprintf(stderr, "%s\n", NAME(Syms[Names[i]]));
	return(1);
}

static int
comp_names(const void *v1, const void *v2)
{
	return(strcmp(NAME(Syms[*((ulong *) v1)]), NAME(Syms[*((ulong *) v2)])));
}

static void
sort_by_name()
{
	int i;

	Names = (ulong *) malloc(Nsyms * sizeof(ulong));
	for (i = 0; i < Nsyms; i++)
		Names[i] = i;
	qsort(Names, Nsyms, sizeof(ulong), comp_names);
}

static int
binsearch(char *name)
{
	int high = Nsyms, middle, low = 0;
	int ret;

	do {
		middle = (high + low) / 2;
		if ((ret = strcmp(name, NAME(Syms[Names[middle]]))) == 0)
			return(middle);
		else if (ret > 0)
			low = middle + 1;
		else
			high = middle - 1;
	} while (high >= low);
	return(-1);
}

int
#ifdef __STDC__
main(int argc, char *argv[])
#else
main(argc, argv)
int argc;
char *argv[];
#endif
{
	/* variables dealing with list of functions given by user */
	int 		lfnum;			/* number of functions */
	int 		lfsize = 0;			/* sizeof list */
	char 		*lfbuf;			/* buffer holding list */
	char 		**lfnames = NULL;	/* array of pointers to names */

	/* temporary variables */
	unsigned int 	i;
	char 		*name;
	Elf32_Sym	*esym;
	Elf_Scn		*scn;
	char		*curfile;	/* current file in symbol table */

	/* file information for target file */
	int 		fd;
	Elf		*elf_file;
	Elf32_Ehdr	*p_ehdr;	/* elf file header */

	/* info on symbol table section */
	Elf_Data	*sym_data = NULL;
	Elf32_Sym	*endsym;	/* pointer to end of symbol table */

	/* info on string table for symbol table */
	int		str_ndx;

	/* info on target section */
	int 		text_ndx = -1;	/* index of target section*/
	char 		*text_name = ".text";	
	char 		*debug_name = ".debug";	
					/* name of target section*/
	char 		*text_newdata;	/* rearranged section */
	int		textsym_ndx; /* index in symtab of sect symbol */

	/* getopt processing */
	int		c;
	extern char	*optarg;
	extern int	optind;


	prog = argv[0];

	while ((c = getopt(argc, argv, "?l:s:d")) != EOF) {
		switch (c) {
		case 's':
			text_name = optarg;
			break;
		case 'd':
			debugflg = 1;
			break;
		case 'l':
			if ((fd = open(optarg, O_RDONLY)) < 0) 
				error("Cannot open function list file %s\n", optarg);
			else {
				if ((lfsize = lseek(fd, 0L, 2)) == -1L)
					error("seek error on %s\n", optarg);
				lseek(fd, 0L, 0);
				lfbuf = CALLOC(lfsize+1, 1);
				if (read(fd, lfbuf, lfsize) != lfsize)
					error("read error on %s\n", optarg);
				
				i = 0;
				lfnum = 0;
				while (i < lfsize) {
					if (lfbuf[i] == '\n')
						lfnum++;
					i++;
				}
				lfnames = (char **)MALLOC(lfnum * sizeof(char *));
				i = 1;
				*(lfnames) = strtok(lfbuf, "\n");
				while ((name = strtok(NULL, "\n")) != NULL) {
					lfnames[i] = name;
					i++;
				}
				close(fd);
			}
			break;
		default:
			usage("illegal option");
		}
	}

	if (lfsize == 0) {
		usage("no function list given");
	}



	if ((fd = open(argv[optind], O_RDWR)) < 0)
		error("cannot open %s\n", argv[optind]);

	if (elf_version(EV_CURRENT) == EV_NONE)
		error("ELF library is out of date\n");

	if ((elf_file = elf_begin(fd, ELF_C_RDWR, (Elf *)0)) == 0)
		error("ELF error in elf_begin: %s\n", elf_errmsg(elf_errno()));

	/*
	 *	get ELF header
	 */
	if ((p_ehdr = elf32_getehdr(elf_file)) == 0)
		error("problem with ELF header: %s\n", elf_errmsg(elf_errno()));

	/*
	 *	check that it is a relocatable file
	 */
	 if (p_ehdr->e_type != ET_REL)
		error("%s is not a relocatable file\n", argv[optind]);

	/*
	 *	load section table
	 */
	esections = CALLOC(sizeof(struct section), p_ehdr->e_shnum);


	i = 1;	/* skip the first entry so indexes match with file */
	scn = 0;

	while ((scn =  elf_nextscn(elf_file, scn)) != 0) {
		esections[i].sec_scn =  scn;
		esections[i].sec_shdr = elf32_getshdr(scn);
		if (esections[i].sec_shdr->sh_type == SHT_SYMTAB){

			if (sym_data != NULL)
				error("multiple symbol table sections not allowed\n");
			esections[i].sec_data = sym_data = 
			   myelf_getdata(scn, 0, "symbol table");
			endsym = (Elf32_Sym *) 
				 ((char *) sym_data->d_buf + sym_data->d_size);

			/* get string data for symbol table */
			str_ndx = esections[i].sec_shdr->sh_link;
		}

		/* is this the target section */
		if ((name = elf_strptr(elf_file, p_ehdr->e_shstrndx, esections[i].sec_shdr->sh_name)) == NULL)
			error("cannot get name for section header %d\n", i);
		if (strcmp(name, text_name) == 0) {
			/* if so save, some information */
			if (text_ndx != -1)
				error("multiple %s sections\n", name);
			text_ndx = i;
			esections[i].sec_data = text_data = 
			   myelf_getdata(scn, 0, "section to be rearranged\n");
			text_shdr = esections[i].sec_shdr;
		}
		i++;
	}
	if (esections[str_ndx].sec_shdr->sh_type != SHT_STRTAB)
		error("symbol table does not point to string table.\n");
	esections[str_ndx].sec_data = str_data =
		myelf_getdata(esections[str_ndx].sec_scn, 0, "string table");


	/* Make pass through all functions, looking for duplicates and
	** figuring out the distances between functions.
	*/
	for (esym = ((Elf32_Sym *) sym_data->d_buf) + 1; esym < endsym; esym++) {
		if (debugflg)
			fprintf(stderr, "Processing %s\n", NAME(esym));

		if (ELF32_ST_TYPE(esym->st_info) == STT_FILE)
			curfile = NAME(esym);

		if (esym->st_shndx != text_ndx)
			continue;

		if (ELF32_ST_TYPE(esym->st_info) == STT_SECTION) {
			textsym_ndx = esym - (Elf32_Sym *) sym_data->d_buf;
			continue;
		}

		if (ELF32_ST_TYPE(esym->st_info) == STT_NOTYPE) {
			if (!(Nnotypes % 10))
				Notypes = (Elf32_Sym **) realloc(Notypes, (Nnotypes + 10) * sizeof(Elf32_Sym *));
			Notypes[Nnotypes++] = esym;
			continue;
		}

		if (ELF32_ST_TYPE(esym->st_info) != STT_FUNC)
			error("illegal type for symbol %s, %d\n",
				NAME(esym),
				ELF32_ST_TYPE(esym->st_info));

		if (esym->st_size == 0)
			error("function of unknown size %s\n", NAME(esym));

		/* create and initialize text_info structure */
		esym->st_value = (Elf32_Addr) gettextinfo(esym, text_data->d_buf, curfile);

		if (!(Nsyms % 100))
			Syms = (Elf32_Sym **) realloc(Syms, (Nsyms + 100) * sizeof(Elf32_Sym *));
		Syms[Nsyms++] = esym;
	}

	qsort(Syms, Nsyms, sizeof(Elf32_Sym *), comp_orig_addr);
	compute_sizes();
	sort_by_name();

	/* determine new addresses for given functions */
	for (i = 0; i < lfnum; i++) {
		int entno;
		char *name;
		int found;

		if (name = strchr(lfnames[i], '@')) {
			*name = '\0';
			name++;
		}
		else
			name = lfnames[i];
		if ((entno = binsearch(name)) < 0) {
			fprintf(stderr, "WARNING: function %s not found in symbol table\n", name);
			continue;
		}

		/* Position at first symbol with the given name */
		for ( ; entno > 0; entno--)
			if (strcmp(NAME(Syms[Names[entno-1]]), name))
				break;

		found = 0;
		do {
			/*
			* IF the symbol is local and the given filename matches the
			* symbol's filename OR the symbol is global and no name was
			* given, THEN this is a match
			*/
			if ((FILENAME(Syms[Names[entno]]) && (name != lfnames[i]) && (strcmp(lfnames[i], FILENAME(Syms[Names[entno]])) == 0)) ||
					(!FILENAME(Syms[Names[entno]]) && (name == lfnames[i]))) {
				setup_addr(Names[entno], 1);
				found++;
			}
			entno++;
		} while ((entno < Nsyms) && (strcmp(NAME(Syms[Names[entno]]), name) == 0));
		if (!found) {
			if (name == lfnames[i])
				fprintf(stderr, "WARNING: global function %s not found in symbol table\n", name);
			else
				fprintf(stderr, "WARNING: function %s not found in symbol table for file %s\n", name, lfnames[i]);
		}
	}

	/* find the rest of the functions in the target section */
	for (i = 0; i < Nsyms; i++)
		setup_addr(i, 0);

	/* update all relocation sections for rearranged section */
	for(i = 1; i < p_ehdr->e_shnum; i++) {
		Elf_Data *td;
		int rtype = esections[i].sec_shdr->sh_type;

		if (rtype != SHT_REL && rtype != SHT_RELA) 
			continue;

		if ((name = elf_strptr(elf_file, p_ehdr->e_shstrndx,
				   esections[i].sec_shdr->sh_name)) == NULL)
			error("Cannot find section name");
		/*if (strstr(name, debug_name))*/
			/*error("Cannot process an object compiled for debugging\n");*/

		esections[i].sec_data = td = 
		   myelf_getdata(esections[i].sec_scn, 0, "relocation section");

		if (esections[i].sec_shdr->sh_info != text_ndx) {
			/* check for relocations against text section
			   symbol in relocations for other sections */
			chktextrels(td, sym_data, esections[i].sec_shdr->sh_info, 
					text_ndx, textsym_ndx, rtype);
			continue;
		}

		/* update relocations for section being rearranged */
		updaterels(td, sym_data, text_ndx, rtype);
	}

	/* update NOTYPE entries */
	for (i = 0; i < Nnotypes; i++) {
		if (!(esym = findsymbyoff(Notypes[i]->st_value)))
			error("Internal Error\n");
		Notypes[i]->st_value = NEWADDR(esym) + (Notypes[i]->st_value - CURADDR(esym));
	}


	/* now actually rearrange the section */
	qsort(Syms, Nsyms, sizeof(Elf32_Sym *), comp_new_addr);

	text_newdata = CALLOC(text_ptr, 1);

	text_ptr = 0;

	for (i = 0; i < Nsyms; i++) {
		int tptr;

		/* align pointer and fill in with desired value */
		tptr = text_ptr;
		text_ptr = ALIGN(text_ptr, text_shdr->sh_addralign);
		memset(text_newdata + tptr, (unchar) 0x90, text_ptr - tptr);

		memcpy(text_newdata + text_ptr, DATAPTR(Syms[i]), USESIZE(Syms[i]));
		text_ptr += USESIZE(Syms[i]);

	}

	/* update the Elf_Data structure for the rearranged section*/
	text_data->d_buf=text_newdata;
	text_data->d_size = text_ptr;

	/* fix the symbol table back up */
	for (esym = (Elf32_Sym *)sym_data->d_buf + 1; esym < endsym; esym++) {

		if ((esym->st_shndx != text_ndx) || (ELF32_ST_TYPE(esym->st_info) == STT_SECTION) || (ELF32_ST_TYPE(esym->st_info) == STT_NOTYPE))
			continue;

		if (debugflg)
			printf("name %s oldaddr %x newaddr %x\n",
				NAME(esym),
				CURADDR(esym),
				NEWADDR(esym));
		esym->st_value = NEWADDR(esym);
	}

	elf_flagphdr(elf_file, ELF_C_SET, ELF_F_DIRTY);

	elf_update(elf_file, ELF_C_WRITE);

	elf_end(elf_file);

	close(fd);
}



static Elf32_Sym *
#ifdef __STDC__
findsymbyoff(Elf32_Addr off)
#else
findsymbyoff(off)
Elf32_Addr off;
#endif
{
	int high = Nsyms, middle, low = 0;
	int ret;

	do {
		middle = (high + low) / 2;
		if (off < CURADDR(Syms[middle]))
			high = middle - 1;
		else if (off > CURADDR(Syms[middle]) + USESIZE(Syms[middle]))
			low = middle + 1;
		else
			return(Syms[middle]);
	} while (high >= low);
	return(NULL);
}

long
#ifdef __STDC__
symchg(Elf32_Addr off, int text_ndx, Elf32_Sym *firstsym, Elf32_Sym *end)
#else
symchg(off, text_ndx, firstsym, end)
Elf32_Addr off;
int text_ndx;
Elf32_Sym *firstsym, *end;
#endif
{
	Elf32_Sym *esym;

	if ((esym = findsymbyoff(off)) == NULL)
		error("cannot find symbol at offset %x in target section\n", off);

	return(NEWADDR(esym) - CURADDR(esym));
}

		

Elf_Data *
#ifdef __STDC__
myelf_getdata(Elf_Scn *scn, Elf_Data *data, const char *errmsg)
#else
myelf_getdata(scn, data, errmsg)
Elf_Scn *scn;
Elf_Data *data;
const char *errmsg;
#endif
{
	Elf_Data *td;
	if ((td = elf_getdata(scn, data)) == NULL)
		error("cannot get data for %s\n", errmsg);
	return(td);
}

struct text_info *
#ifdef __STDC__
gettextinfo(Elf32_Sym *esym, char *text_dptr, char *curfile)
#else
gettextinfo(esym, text_dptr)
Elf32_Sym *esym;
char *text_dptr;
#endif
{
	struct text_info *titemp;
	titemp = (struct text_info *) calloc(1, sizeof(struct text_info));

	titemp->ti_curaddr = esym->st_value;
	titemp->ti_newaddr = NO_SUCH_ADDR;
	if (ELF32_ST_BIND(esym->st_info) == STB_LOCAL)
		titemp->ti_filename = curfile;
	else
		titemp->ti_filename = NULL;

	/* set pointer to body of function in data */
	titemp->ti_data = text_dptr + esym->st_value;
	return(titemp);
}

void
#ifdef __STDC__
usage(char *msg)
#else
usage(msg)
char *msg;
#endif
{
	fprintf(stderr, "%s: %s\n", prog, msg);
	fprintf(stderr, "Usage: %s -l function_list [ -s section_name ] reloc_file\n", prog);
	exit(-1);
}

void
#ifdef __STDC__
error(const char *fmt, ...)
#else
error(va_alist)
va_dcl
#endif
{
	va_list ap;
#ifdef __STDC__
	va_start(ap, fmt);
#else
	char *fmt;
	va_start(ap);
	fmt = va_arg(ap, char *);
#endif
	fprintf(stderr, "%s: ", prog);
	vfprintf(stderr, fmt, ap);
	exit(-1);
}

