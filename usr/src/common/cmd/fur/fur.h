/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)fur:common/cmd/fur/fur.h	1.3"
#ifndef _FUR_H
#define _FUR_H

#ifdef __STDC__
void error(const char *, ...);
Elf_Data * myelf_getdata(Elf_Scn *, Elf_Data *, const char *);
Elf32_Sym *findsymbyoff(Elf32_Addr);
long symchg(Elf32_Addr off, int text_ndx, Elf32_Sym *firstsym, Elf32_Sym *end);
Elf32_Sym *findsym(char *, Elf32_Sym *, Elf32_Sym *, int, char *);
struct text_info *gettextinfo(Elf32_Sym *, char *, char *);
void updaterels(Elf_Data *, Elf_Data *, int, int);
void chktextrels(Elf_Data *, Elf_Data *, int, int, int, int);
#else
void error();
Elf32_Sym *findsym();
Elf32_Sym *findsymbyoff();
long symchg();
struct text_info *gettextinfo();
void usage();
Elf_Data *myelf_getdata();
void updaterels();
void chktextrels();
#endif

/* keeps track of functions as the are moved */
struct text_info {
	Elf32_Addr ti_curaddr;		/* symtab input address */
	Elf32_Addr ti_newaddr;		/* symtab output address */
	Elf32_Addr ti_usesize;		/* size to use for function*/
	char *ti_filename;
	char *ti_data;			/* input buffer ptr to function body */
};

/* structure to keep track of sections in file */
struct section {
	Elf_Scn *sec_scn;		/* scn pointer */
	Elf32_Shdr *sec_shdr;		/* section header */
	Elf_Data *sec_data;		/* data associated with section */
};

extern struct section *esections;
#endif
