/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ndt.cmd:mtrace.c	1.2"
#ident	"@(#)mtrace.c	2.1 "
#ident  "$Header: /SRCS/esmp/usr/src/nw/cmd/nuc.d/ndt.d/mtrace.c,v 1.2 1994/01/31 21:51:59 duck Exp $"
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
 *       Created: Sun May  5 14:06:47 MDT 1991
 *
 *  MODULE:
 *
 *  ABSTRACT:
 *
 */
#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/fcntl.h>

#include	<sys/nwctypes.h>
#include	<sys/nwcmem.h>
#include	<sys/trace/nwctrace.h>
#include	<syms.h>

#include	<sys/traceuser.h>
#include	"ndt.h"

int         errno;
static int	long_option=0;
int         debug_option=0;

char        *read_trace_file=NULL;
char        *namelist="/stand/unix";
char		*dump_file=NULL;
char		*memImageFile;
char		*modpath=NULL;

int			debugmode=0,
			usingDumpFile,
			active=0,
			Virtmode=1,
			mem,
			nmlst_tstamp;


void			 symgetval( char *symbol, char *value, int length);
NameOff			*find_symbol();
struct syment	*symsrch();
static void		usage();



main( argc, argv)
int argc;
char **argv;
{
	extern char			*optarg;
	MEM_BLOCK_T			*bp, block;
	int					i, c, count=0, size=0;
	NameOff				*no_p;
	memRegionSummary_t	regionSummary={0};

	while( (c=getopt( argc, argv, "Dln:d:h")) != -1) {
		switch( c) {
			case 'n':
				namelist = optarg;
				break;
			case 'd':
				dump_file = optarg;
				break;
			case 'D':
				debug_option++;
				break;
			case 'l':
				long_option++;
				break;
			case '?':
			case 'h':
				usage();
				exit( 1);
		}
	}

	if( dump_file == NULL) {				/* no dump file to read */
		memImageFile = "/dev/mem";			/* get symbols from here*/
		usingDumpFile=0;
	} else {
		memImageFile = dump_file;			/* get symbols from here*/
		usingDumpFile=1;
	}

	if( (mem=open( memImageFile, O_RDONLY)) == NULL) {
		perror("open dump file");
		exit( 1);
	}
	
	read_symbols();

    symgetval( "nwtlAllocatedList", (char *)&bp, sizeof( bp));

	while( bp) {
		readmem( bp, 1, -1, &block, sizeof( block), "block");

		no_p = find_symbol( block.caller);
		if( long_option)
			printf("% 8x % 8x % 5d bytes   region %02d   %s+%#x\n",
				block.addr,
				block.addr + block.size - 1, 
				block.size,
				block.region,
				no_p->name, no_p->offset
				);

		regionSummary[(int)block.region].count++;
		regionSummary[(int)block.region].size += block.size;

		bp = block.Next;
	}

	printf("\nSummary:\n");
	for( i=0; i<NREGIONS; i++) {
		if( regionSummary[i].count) {
			printf("r%02d  % 5d requests  % 8d bytes\n",
				i, regionSummary[i].count, regionSummary[i].size);
			count += regionSummary[i].count;
			size  += regionSummary[i].size;
		}
	}
	
	printf("% 9d requests % 9d bytes total.\n\n", count, size);
}



void
symgetval( char *symbol, char *value, int length)
{
    struct syment *sysgetval_info = NULL;

    if((sysgetval_info = symsrch( symbol)) == NULL) {
        fprintf(stderr,"Can't find %s\n", symbol);
        exit( 1);
    }
    readmem( sysgetval_info->n_value, 1, -1, value, length, symbol);
}





static void
usage()
{
	printf("Usage:\n");
	printf("mtrace [-l] [-D] [-n <nlist_file>]  [-d <dump_file>]\n");
	printf("  Shows summary of allocated memory blocks, by region.\n");
	printf("    -l               Show details for each allocated memory block.\n");
	printf("    -D               Debug option.\n");
	printf("    -n <nlist_file>  Specifies the kernel you're running, default is /stand/unix.\n");
	printf("    -d <dump_file>   Specifies the name of a saved system dump (panic) file.\n");
}
