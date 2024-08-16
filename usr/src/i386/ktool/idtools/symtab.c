/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ktool:i386/ktool/idtools/symtab.c	1.3"
#ident	"$Header: $"

/*
 * This module examines the symbol table of a Driver.o to find out
 * which entry points are present, and make symbol remapping
 * according to requests in interface definitions.  This is used by idconfig.
 */

#include "inst.h"
#include "defines.h"
/*
 * In a cross-environment, make sure these headers are for the host system
 */
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
/*
 * The remaining headers are for the target system.
 */
#include <libelf.h>

extern struct entry_def *entry_defs;
extern struct intfc *interfaces;
extern int debug;

static FILE *elf_fp;		/* stdio stream for ELF file */
static Elf *elf_file;		/* ELF file */
static size_t str_ndx;		/* string index */
static Elf_Scn *strscn;		/* ELF section header for string table */
static Elf32_Sym *symbols;	/* pointer to ELF symbols */
static size_t nsyms;		/* number of symbols */
static int file_modified;	/* ELF info has been modified */
static Elf32_Sym *symp;		/* pointer to current ELF symbol */


int
load_symtab(filename, read_only)
char *filename;	/* Name of Driver.o file */
{
	Elf32_Shdr      *section;	/* elf section header	*/
	Elf_Data	*data;		/* info on section tab	*/
	Elf_Scn		*scn;		/* elf section header	*/

	if (debug)
		fprintf(stderr, "\tLOAD_SYMTAB: %s (%s)\n", filename,
				(read_only ? "r" : "r+"));

	if ((elf_fp = fopen(filename, (read_only ? "r" : "r+"))) == NULL) {
		if (debug)
			fprintf(stderr, "load_symtab: cannot open %s\n",
					filename);
		return(-1);
	}

	if (elf_version(EV_CURRENT) == EV_NONE) {
		if (debug)
			fprintf(stderr, "load_symtab: ELF lib out of date\n");
		fclose(elf_fp); 
		return(-1);
	}

	if ((elf_file = elf_begin(fileno(elf_fp), read_only ? ELF_C_READ :
				  ELF_C_RDWR, (Elf *)0)) == 0) {
		if (debug)
			fprintf(stderr,
				"load_symtab: ELF error in elf_begin: %s, %s\n",
				filename, elf_errmsg(elf_errno()));
		fclose(elf_fp);
		return(-1);
	}

	/*
	 * load section table
	 */
	scn = 0;
	nsyms = 0;
	while ((scn = elf_nextscn(elf_file, scn)) != 0) {
		section = elf32_getshdr(scn);
		if (section->sh_type == SHT_SYMTAB) {
			data = 0;
			if ((data = elf_getdata(scn, data)) == 0) {
				if (debug)
					fprintf(stderr,
					"load_symtab: bad symbol table %s.\n",
					filename);
				elf_end(elf_file);
				fclose(elf_fp);
				return(-1);
			}

			str_ndx = section->sh_link;

			/* skip non-global symbols */
			symbols = (Elf32_Sym *)data->d_buf + section->sh_info;
			nsyms = data->d_size / sizeof(Elf32_Sym) -
				section->sh_info + 1;

			break;
		}
	}

	strscn = elf_getscn(elf_file, str_ndx);
	section = elf32_getshdr(strscn);
	if (section->sh_type == SHT_STRTAB) {
		data = 0;
		if ((data = elf_getdata(scn, data)) == 0 || data->d_size == 0) {
			if (debug)
				fprintf(stderr,
				"load_symtab: bad string section data in %s.\n",
				filename);
			elf_end(elf_file);
			fclose(elf_fp);
			return(-1);
		}
	} else {
		if (debug)
			fprintf(stderr,
	"load_symtab: symbol table does not point to valid string table in %s.\n",
				filename);
		elf_end(elf_file);
		fclose(elf_fp);
		return(-1);
	}

	file_modified = 0;

	return(0);
}

void
close_symtab()
{
	if (file_modified) {
		elf_update(elf_file, ELF_C_WRITE);
		if (debug)
			fprintf(stderr, "Symbol table modified.\n");
	}

	elf_end(elf_file);
	fclose(elf_fp);
}

/*
 * Scan all symbols, calling a specified function for each one.
 * Skip symbols that are not of the specified type.
 */
int
scan_symbols(func, arg, type)
int (*func)();
void *arg;
int type;
{
	int i, stat;
	char *symname;

	/*
	 * for each symbol in input file...
	 */
	for (symp = symbols, i = 1; i < nsyms; i++, symp++) {
		switch (type) {
		case SS_UNDEF:
			if (symp->st_shndx != SHN_UNDEF)
				continue;
			if (ELF32_ST_BIND(symp->st_info) != STB_GLOBAL)
				continue;
			break;
		case SS_GLOBAL:
			if (symp->st_shndx == SHN_UNDEF)
				continue;
			if (ELF32_ST_BIND(symp->st_info) != STB_GLOBAL)
				continue;
			break;
		default:
			return EINVAL;
		}
		symname = elf_strptr(elf_file, str_ndx, (size_t)symp->st_name);
		stat = (*func)(symname, arg);
		if (stat != 0)
			return stat;
	}

	return 0;
}

/*
 * Change the name of the current symbol being processed by scan_symbols().
 */
void
rename_symbol(newname)
char *newname;
{
	Elf_Data *data;
	Elf32_Off new_off;

	new_off = elf32_getshdr(strscn)->sh_size;
	data = elf_newdata(strscn);
	data->d_buf = newname;
	data->d_type = ELF_T_BYTE;
	data->d_size = strlen(newname) + 1;
	data->d_align = 1;
	symp->st_name = new_off;
	elf_update(elf_file, ELF_C_NULL);

	file_modified = 1;
}


/*
 * Get a pointer to the value of a variable in the ELF file, looked up by name.
 */

static void *valptr;

static int
find_valptr(symname, arg)
char *symname;
void *arg;
{
	Elf_Scn *secp;
	Elf32_Shdr *shdrp;
	Elf_Data *datap;

	if (strcmp(symname, (char *)arg) != 0)
		return 0;

	if (debug)
		fprintf(stderr, "\tfound %s\n", symname);

	secp = elf_getscn(elf_file, symp->st_shndx);
	shdrp = elf32_getshdr(secp);
	datap = elf_getdata(secp, NULL);
	valptr = (char *)datap->d_buf + (symp->st_value - shdrp->sh_addr);
	return 0;
}

void *
get_valptr(symname)
char *symname;
{
	if (debug)
		fprintf(stderr, "get_valptr(%s)\n", symname);
	(void) scan_symbols(find_valptr, symname, SS_GLOBAL);
	return valptr;
}
