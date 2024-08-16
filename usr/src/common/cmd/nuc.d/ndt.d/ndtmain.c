/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ndt.cmd:ndtmain.c	1.2"
#ident	"$Header: /SRCS/esmp/usr/src/nw/cmd/nuc.d/ndt.d/ndtmain.c,v 1.2 1994/01/31 21:52:09 duck Exp $"

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
#include	<sys/param.h>
#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/fcntl.h>
#include	<sys/stat.h>

#include	<sys/ioctl.h>
#include	<sys/time.h>

#include	<sys/plocal.h>
#include	<sys/var.h>
#include	<sys/user.h>

#include	<stdlib.h>

#include	<string.h>
#include	<syms.h>

#include	<sys/nwctrace.h>

#include	<sys/traceuser.h>

#include	"ndt.h"


/* Command line argument variables	*/

static	char		command=0;
#define	CMD_SET			1
#define	CMD_GET 		2
#define CMD_LOOP_THRU	3
#define CMD_RESET		4
#define CMD_WRITE		5
#define CMD_PERFORMANCE	6
#define CMD_SET_STRLOG	7
#define CMD_BOTH		8
#define CMD_SET_SIZE	9

static	unsigned int cmdmask[NVLT_MASK_ARRAY_SIZE];

		int			print_option=0,
					time_option =0,
					debug_option=0,
					verbose=0,
					usingDumpFile,
					newEntries=0;

		/*
		 *	these apply to the current trace table.
		 */
		trace_t		*firstTracePtr,				/* first meaningful entry	*/
					*lastTracePtr;				/* last trace entry			*/
		int			nTraceEntries;				/* number of trace entries	*/

		int			nmlst_tstamp, /* for crash */
					debugmode=0,
					active=1,
					Virtmode=1,
					mem,
					kmem_fd;
		int			Procslot;	/* new crash	*/
		struct plocal	l;
		struct syment	dummyent;
		FILE			*fp, *rp;
		struct var		vbuf;
		struct user		*ubp;

		double		etime;	/* elapsed time	in a routine or on the stopwatch or on the wire*/

		char		*namelist="/stand/unix";
		char		*dump_file=NULL;
		char		*memImageFile;
		char		*write_trace_file=NULL;
		char		*read_trace_file=NULL;
		char		*ndtStrings;
		int			ndtStringLen;

		int			nLWPs;
		lwpTable_t	*lwpTable;

		int			nLocks;
		lockTable_t	*lockTable;

		int			nEngines;
		enum timeStyle timeStyle;


extern	MASK_TAB	mask_tab[];


static	void		usage();


main( argc, argv)
int argc;
char **argv;
{
	extern char *optarg;
	extern int	optind;
	int			c;
	int			m;

	while( (c=getopt( argc, argv, "prsgtDvKlbn:w:f:d:P:S:")) != -1) {
		switch( c) {
			case 'p':
				command = CMD_LOOP_THRU;
				print_option++;
				break;
			case 'r':
				command = CMD_RESET;
				break;
			case 's':
				command = CMD_SET;
				break;
			case 'l':
				command = CMD_SET_STRLOG;
				break;
			case 'b':
				command = CMD_BOTH;
				break;
			case 'g':
				command = CMD_GET;
				break;
			case 't':
				command = CMD_LOOP_THRU;
				time_option++;
				break;
			case 'D':
				debug_option++;
				break;
			case 'n':
				namelist = optarg;
				break;
			case 'w':
				write_trace_file = optarg;
				command = CMD_WRITE;
				break;
			case 'f':
				active = 0;
				read_trace_file = optarg;
				break;
			case 'v':
				verbose++;
				break;
			case 'd':
				dump_file = optarg;
				active = 0;
				break;
			case 'S':
				command = CMD_SET_SIZE;
				newEntries = atoi( optarg);
				break;
			case '?':
				printf("hit the ?\n");
				fflush( stdout);
				break;
		}
	}


	if( !command) {
		usage();
		exit( 1);
	}

	if( argc == optind) { 					/* no further arguments			*/
		if( (command == CMD_SET)  || (command == CMD_SET_STRLOG) || (command == CMD_BOTH)) {
			for( m=0; m < NVLT_MASK_ARRAY_SIZE; m++)
				cmdmask[m]=0;				/* turn off everything			*/
		}

		/* default "print" and "time" to do all	mask bits*/
		if( command == CMD_LOOP_THRU) 		
			for( m=0; m < NVLT_MASK_ARRAY_SIZE; m++)
				cmdmask[m]=0xffff0000;					/* turn on everything			*/

	} else {											/* process mask flags			*/
		int i;

		for( m=0; m < NVLT_MASK_ARRAY_SIZE; m++)
			cmdmask[m]=0;								/* start with clean slate		*/
			
		for( i=optind; i<argc; i++ ) {
			MASK_TAB	*mt_p;

			/*
			 *	Find mask bit corresponding to mask name
			 *	Handle special "all" case first.
			 */

			if( strcmp( argv[i], "all") == 0) {
				for( m=0; m < NVLT_MASK_ARRAY_SIZE; m++)
					cmdmask[m]=0xffff0000;				/* turn on everything			*/
				continue;
			}

			/*
			 *	Loop thru table of known mask names.  Iff we find a match,
			 *	turn on the appropriate bit.
			 */
			for( mt_p=mask_tab; mt_p->name; mt_p++) { 
				if( strcmp( argv[i], mt_p->name) == 0) {
					cmdmask[TR_INDEX( mt_p->mask)] |= TR_MASK_BITS(mt_p->mask);
					break;
				}
			}

			if( mt_p->name == NULL) {					/* Fell off end == no find	*/
				printf( "unknown mask bit: \"%s\"\n", argv[i]);
				exit( 1);
			}
				
		}												/* end of argv loop				*/
	}													/* end of cmd line args			*/

	if( debug_option) {
		int				i;
		unsigned int	reset_mask[NVLT_MASK_ARRAY_SIZE];

		printf( "  set:");
		show_mask( cmdmask);

		for( i=0; i<NVLT_MASK_ARRAY_SIZE; i++)
			reset_mask[i] = cmdmask[i] ^ NVLT_Mask_Mask;

		printf( "reset:");
		show_mask( reset_mask);
	}



	/*
	 *	Open the crash can.
	 */
	if( (fp = fopen( "/dev/null", "a")) == NULL ) {
		perror( "open /dev/nnull");
		exit( 1);
	}

	/*
	 *	Open kmem here;  we'll need it in a couple
	 *	of places real soon.
	 */

	if( (kmem_fd=open( "/dev/kmem", O_RDONLY)) == -1) {
		perror( "open /dev/kmem");
		exit( 1);
	}

	if( dump_file == NULL) {							/* no dump file to read */
		memImageFile = "/dev/mem";						/* get symbols from here*/
		usingDumpFile=0;
	} else {
		memImageFile = dump_file;						/* use command line specification	*/
		usingDumpFile=1;
	}

	if( (mem=open(memImageFile, O_RDONLY)) == NULL) {
		perror("open dump file");
		exit( 1);
	}



	switch( command) {
		case CMD_SET:
			cmd_set( cmdmask);
			break;

		case CMD_SET_STRLOG:
			cmd_set_strlog( cmdmask);
			break;

		case CMD_BOTH:
			cmd_set( cmdmask);
			cmd_set_strlog( cmdmask);
			break;

		case CMD_GET:
			cmd_get();
			break;

		case CMD_LOOP_THRU:
			cmd_loop( cmdmask);
			break;

		case CMD_RESET:
			cmd_reset();
			break;

		case CMD_WRITE:
			cmd_write();
			break;

		case CMD_SET_SIZE:
			cmd_set_size( newEntries);
			break;
	}

}


static void
usage()
{
	printf("Usage:\n");
	printf("ndt -srgptdvK [-n <nlist_file>] [-f <trace_file>] [-d <dump_file>] mask ...\n");
	printf("ndt -w <trace_file>\n");
	printf("ndt -S <number of trace entries>\n");
	printf("\n");
	printf("    -s mask ...      Sets the trace mask in the driver.\n");
	printf("                     Only events with these mask bits will be traced.\n");
	printf("    -l mask ...      Sets the strlog mask in the driver.\n");
	printf("                     Only strlogs with these mask bits will be traced.\n");
	printf("    -b mask ...      Sets the both the trace and strlog masks in the driver.\n");
	printf("    -r               Reset trace table.\n");
	printf("    -g               Gets and displays current driver mask settings.\n");
	printf("    -p [mask ...]    Prints current trace table. If mask bits are specified,\n");
	printf("                     only events with these mask bits will be printed.\n");
	printf("    -t [mask ...]    Prints timing analysis. Mask bits may be specified to\n");
	printf("                     limit output.\n");
	printf("    -D               Debug option.\n");
	printf("    -v               Verbose output.\n");
	printf("    -n <nlist_file>  Specifies the kernel you're running, if it's not /stand/unix.\n");
	printf("    -f <trace_file>  Specifies the name of a saved trace file (written with -w).\n");
	printf("    -d <dump_file>   Specifies the name of a saved system dump (panic) file.\n");
	printf("    -w <trace_file>  Saves the current trace table and symbol table to \n");
	printf("                     <trace_file> for future analysis.\n");
	printf("    -S <number>      Set new trace table size to <number> entries.\n");
}


