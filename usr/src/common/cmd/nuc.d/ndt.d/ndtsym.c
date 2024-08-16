/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ndt.cmd:ndtsym.c	1.2"
#ident	"$Header: /SRCS/esmp/usr/src/nw/cmd/nuc.d/ndt.d/ndtsym.c,v 1.2 1994/01/31 21:52:14 duck Exp $"

/*
 *        Copyright Novell Inc. 1991
 *        (C) Unpublished Copyright of Novell, Inc. All Rights Reserved.
 *
 *        No part of this file may be duplicated, revised, translated, localized
 *        or modified in any manner or compiled, linked or uploaded or
 *        downloaded to or from any computer system without the prior written
 *        consent of Novell, Inc.
 *
 *
 *  Netware Unix Client 
 *        Author: Duck
 *       Created: Sun May  5 14:06:50 MDT 1991
 *
 *  MODULE:
 *
 *  ABSTRACT:
 *
 */
#define MAINSTORE	0xc0000000

#include	<sys/types.h>
#include	<a.out.h>
#include	<unistd.h>
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<fcntl.h>
#include	<syms.h>
#include	<elf.h>
#include	<libelf.h>
#include	<sys/nwctrace.h>
#include	<sys/traceuser.h>
#include	"ndt.h"

NameOff *find_symbol();
char	*strdup();

static void compute_high_low( SYMTAB_t *sym_p);

short			N_TEXT,N_DATA,N_BSS;		/* used in symbol search		*/

SYMTAB_t		symtab[2];

extern char		*namelist;
extern int		errno;
extern char		*read_trace_file;
extern int		debug_option;


		struct syment *findsym();


/* symbol table initialization function */


static void
compute_high_low( SYMTAB_t *sym_p)
{
	SYMENT	*sp;
	int		i;

	sym_p->low = 0xffffffff;
	sym_p->high = 0;

	for(sp=sym_p->symtab; sp < &sym_p->symtab[sym_p->sym_count]; sp++) {
		if( sp->n_value < sym_p->low)	sym_p->low = sp->n_value;
		if( sp->n_value > sym_p->high)	sym_p->high = sp->n_value;
	}
}


void
read_symbols()
{
	FILE			*np;
	struct filehdr	filehdr;
	SYMTAB_t		*sym_p = &symtab[0];
	SYMENT			*sp;
	struct scnhdr	scnptr ,
					boot_scnhdr ;
	int				i, N_BOOTD ;
	char			*str;
	long			*str2;

	if( read_trace_file) {
		/* XXX  read_symbols_from_file(); */
		return;
	}

	rdsymtab();								/* read symbol file		*/
}


static	NameOff no_s;

/*
 *	Find symbol and return a structure containing a module
 *	name string and an offset.
 */
NameOff *
find_symbol(addr)
unsigned long addr;
{
	SYMENT		*sp;
	SYMTAB_t	*sym_p = &symtab[0];

	sp = findsym( addr);

	no_s.base   = sp->n_value;
	no_s.offset = addr - sp->n_value;
	if( sp->n_zeroes)
		no_s.name = sp->n_nptr;
	else
		no_s.name = sp->n_offset + sym_p->stringtab;

	return( &no_s);
}



#ifdef NEVER
/*
 *	Read the symbol table and the string table
 *	from a saved trace file.
 */

read_symbols_from_file()
{

	int			fd, i;
	TR_FILE_HDR	hdr;
	SYMTAB_t	*sym_p;

	if( (fd = open( read_trace_file, O_RDONLY)) == -1) {
		perror("ndtsym: open read trace file");
		exit( 1);
	}

	if( read( fd, &hdr, sizeof( hdr)) == -1) {
		perror("ndtsym: read trace save file header");
		exit( 1);
	}

	if( strcmp( hdr.marker, "Trace") != 0) {
		printf("ndtsym: %s is not a valid trace file.\n", read_trace_file);
		exit( 1);
	}

	for( i=0; i<NSYMTAB; i++) {
		sym_p = &symtab[i];

		sym_p->symtab_size    = hdr.symbol_table_size[i];
		sym_p->stringtab_size = hdr.string_table_size[i];
		sym_p->sym_count      = hdr.sym_count[i];

		if((sym_p->symtab=(SYMENT *)malloc(hdr.symbol_table_size[i]))==NULL){
			perror("ndtsym: malloc of symbol table");
			exit( 1);
		}

		if( lseek( fd, hdr.symbol_table_lseek[i], SEEK_SET) == -1) {
			perror("ndtsym: symbol table seek");
			exit( 1);
		}

		if( read( fd, sym_p->symtab, hdr.symbol_table_size[i]) == -1) {
			perror("ndtsym: read trace save file: symbol table");
			exit( 1);
		}

		compute_high_low( sym_p);		/* determine symbol table range	*/

		if((sym_p->stringtab=malloc(hdr.string_table_size[i]))==(char *)NULL){
			perror("ndtsym: malloc of string table");
			exit( 1);
		}

		if( lseek( fd, hdr.string_table_lseek[i], SEEK_SET) == -1) {
			perror("ndtsym: string table seek");
			exit( 1);
		}

		if( read( fd, sym_p->stringtab, hdr.string_table_size[i]) == -1) {
			perror("ndtsym: read trace save file: string table");
			exit( 1);
		}
	}

	close( fd);
}
#endif NEVER


