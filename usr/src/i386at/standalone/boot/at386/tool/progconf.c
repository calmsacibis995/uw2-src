/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)stand:i386at/standalone/boot/at386/tool/progconf.c	1.2"
#ident	"$Header: $:"

#include <stdio.h>
#include <libelf.h>
#include <fcntl.h>
#include <boothdr/bootdef.h>
#include <sys/types.h>
#include <sys/param.h>
#include <string.h>

#define P_PROG_SECT	"#define PROG_SECT 0x%x \n"
#define P_NXT_LD_ADDR	"#define NXT_LD_ADDR 0x%x \n"


/* Round up page table address */

extern void elf_eoj();

main (argc, argv)
int argc;
char *argv[];
{
	FILE *ofp;
	Elf *elf;
	Elf32_Ehdr *ehdr;
	Elf32_Phdr *phdr;
	char *cmd;
	char *infile, *ofile;
	int fd;
	int kind;
	paddr_t pg_endaddr = 0;
	paddr_t ldaddr;
	unsigned int pg_bytesz = 0;
	unsigned int sectsz;
	unsigned short phnum;

	cmd = argv[0];
	infile = argv[1];
	ofile = argv[2];

	if (argc > 4) {
		fprintf(stderr, "usage: %s infile outfile [V]\n", cmd);
		exit (1);
	}

	if ((elf_version(EV_CURRENT)) == EV_NONE) {
		fprintf(stderr, "%s: ELF Access library out of date\n", cmd);
		exit (1);
	}

	if ((fd = open(infile, O_RDONLY)) == -1) {
		perror(infile);
		exit (1);
	}

	if ((elf = elf_begin(fd, ELF_C_READ, NULL)) == NULL) {
		fprintf(stderr, "%s: Can't Elf Begin %s (%s)\n",
			cmd, infile, elf_errmsg(-1));
		elf_eoj(NULL, fd, 1);
	}

	if ((ehdr = elf32_getehdr(elf)) == NULL) {
		fprintf(stderr, "%s: (%s) Can't get Elf Header (%s)\n",
			cmd, infile, elf_errmsg(-1));
		elf_eoj(elf, fd, 1);
	}

	if (((kind = elf_kind(elf)) != ELF_K_ELF) &&
			(kind != ELF_K_COFF)) {
		fprintf(stderr, "%s: %s not a valid binary file\n",
			cmd, infile);
		elf_eoj(elf, fd, 1);
	}

	if ((phdr = elf32_getphdr(elf)) == NULL) {
		fprintf(stderr, "%s: Can get Program Header for %s (%s)\n",
			cmd, infile, elf_errmsg(-1));
		elf_eoj(elf, fd, 1);
	}

	for (phnum = 0; phnum < ehdr->e_phnum; ++phnum) {

		if (phdr[phnum].p_type != PT_LOAD)
			continue;

		if (phdr[phnum].p_filesz)
			pg_bytesz += phdr[phnum].p_filesz;
	}
	--phnum;
	pg_endaddr = phdr[phnum].p_vaddr + phdr[phnum].p_memsz;

	ldaddr  = (paddr_t) ptob(btopr(pg_endaddr + (paddr_t)PROG_GAP));
	sectsz = ((pg_bytesz - 1) / 512 + 1);

	if ((ofp = fopen(ofile, "w")) == NULL) {
		perror(ofile);
		elf_eoj(elf, fd, 1);
	}

	(void)fprintf(ofp, P_PROG_SECT, sectsz);
	(void)fprintf(ofp, "\n");
	(void)fprintf(ofp, P_NXT_LD_ADDR,  ldaddr);
	(void)fprintf(ofp, "\n");
	(void)fclose(ofp);

	elf_eoj(elf, fd, 0);
}

void
elf_eoj(e, fd, x)
Elf *e;
int fd;
int x;
{
	if (e)
		(void)elf_end(e);
	if (fd != -1)
		(void)close(fd);
	exit (x);
}
