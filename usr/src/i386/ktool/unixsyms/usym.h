/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _USYM_H
#define _USYM_H
#ident	"@(#)ktool:i386/ktool/unixsyms/usym.h	1.2"
#ident	"$Header: $"


extern	char		*prog;
extern	char 		*symsection;
extern	int		debugonlyflg;
extern	off_t icmd_len, ecmd_len;
int addflg ;
extern	char **namelist;
extern char	*optarg;
extern int	optind;

extern	int ifd, efd;
extern	int fd, numlim, lim;

#ifndef NATIVE

/* structure to keep track of sections in input file */
struct section {
	Elf_Scn *sec_scn;
	Elf32_Shdr *sec_shdr;
};

/* Special symbol names */
#define SN_KERNMOD	"mod_obj_kern"	/* kernel modobj pointer */
#define SN_SYMSIZE	"mod_obj_size"
#define SN_KDBCOMMANDS	"kdbcommands"	/* location for kdb commands strings*/
#define SYMSECTION	".unixsyms"	/* section name for symbol data */

#ifdef __STDC__
extern Elf32_Addr addsym(Elf *, Elf32_Ehdr *,char *, Elf_Data *, Elf_Data *, Elf_Data *, Elf_Data *,
	Elf_Data *, Elf32_Addr *,  unsigned int *);
#else
extern Elf32_Addr addsym();
#endif

/* Special symbol names */
#define SN_KERNMOD	"mod_obj_kern"	/* kernel modobj pointer */
#define SN_SYMSIZE	"mod_obj_size"
#define SN_KDBCOMMANDS	"kdbcommands"	/* location for kdb commands strings*/
#define SYMSECTION	".unixsyms"	/* section name for symbol data */

/* initialize Elf_Data structure to minimal values */
/* The version field is initialized inline */
#define INITDATA(data)	{ data.d_size = 0; data.d_buf = NULL; \
				data.d_align = 1; data.d_type = ELF_T_BYTE; }

/* offsets for special symbols */
Elf32_Addr loc_kernmod;
unsigned long sec_kernmod;
Elf32_Addr loc_kdbcommands;
unsigned long sec_kdbcommands;
Elf32_Addr loc_symsize;
unsigned long sec_symsize;

#endif
#endif /* _USYM_H */
