/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:common/cmd/crash/pty.c	1.2"
#ident	"$Header: pty.c 1.1 91/07/23 $"

/*
 * This file contains code for the crash function pty.
 */

#include "sys/param.h"
#include "a.out.h"
#include "stdio.h"
#include "sys/sysmacros.h"
#include "sys/types.h"
#include "sys/fs/s5dir.h"
#include "string.h"
#include "sys/stream.h"
#include "sys/termios.h"
#include "sys/strtty.h"
#include "crash.h"
#include "sys/ptms.h"
#include "sys/ptem.h"

short print_header;	

/* get arguments for tty function */
int
getpty()
{
	int slot = -1;
	int full = 0;
	int all = 0;
	int phys = 0;
	int count = 0;
	int line_disp = 0;
	int ptem_flag = 0;
	int pckt_flag = 0;
	int c;

	char *type = "";
	char *heading = "SLOT   MWQPTR   SWQPTR  PT_BUFP STATE\n";

	long addr = -1;
	long arg2 = -1;
	long arg1 = -1;


	optind = 1;
	while( ( c = getopt( argcnt, args, "efhplsw:t:")) != EOF) {
		switch ( c) {
			case 'e':
					all = 1;
					break;

			case 'f':
					full = 1;
					break;

			case 'p':
					phys = 1;
					break;

			case 't':
					type = optarg;
					break;

			case 'w':
					redirect();
					break;

			case 'l':
					line_disp = 1;
					break;

			case 'h':
					ptem_flag = 1;
					break;

			case 's':
					pckt_flag = 1;
					break;

			default:
					longjmp( syn, 0);
		}
	}
	if (( !strcmp( "",type)) && !args[optind]){
		addr = getaddr( "ptms");
		count = getcount( "pt");
		print_header = 0;
		fprintf( fp, "ptms_tty TABLE SIZE = %d\n", count);

		if ( !full)
			fprintf( fp, "%s", heading);

		for ( slot = 0; slot < count; slot++)
			prspty( all, full, slot, phys, addr, "", line_disp, ptem_flag, pckt_flag, heading);
	} else {
		if ( strcmp( type, "")) {
			addr = getaddr( type);
			count = getcount( type);

			fprintf( fp,"%s TABLE SIZE = %d\n", type, count);
		}
		if ( !full)
			fprintf( fp, "%s", heading);

		if ( args[optind]) {
			all = 1;
			do {
				getargs( count, &arg1, &arg2);
				if ( arg1 == -1) 
					continue;

				if ( arg2 != -1)
					for ( slot = arg1; slot <= arg2; slot++)
						prspty( all, full, slot, phys, addr, type, line_disp, ptem_flag, pckt_flag, heading);
				else {
					if ( arg1 >=0 && arg1 < count)
						slot = arg1;
					else
						addr = arg1;

					prspty( all, full, slot, phys, addr, type, line_disp, ptem_flag, pckt_flag, heading);
				}
				slot = arg1 = arg2 = -1;
				if ( strcmp( type, ""))
					addr = getaddr( type);
			} while ( args[++optind]);
		} else
			for ( slot = 0; slot < count; slot++)
				prspty( all, full, slot, phys, addr, "", line_disp, ptem_flag, pckt_flag, heading);
	}
}

/*
 * print streams pseudo tty table
 */
prspty( all, full, slot, phys, addr, type, line_disp, ptem_flag, pckt_flag, heading)
int all,full,slot,phys;
long addr;
char *type;
char *heading;
{

	struct pt_ttys	pt_tty;
	struct queue	q;
	struct syment	*symsrch();

	int count;

	long base;
	long offset;


	if ( slot == -1)
		readmem( (long)addr, phys, -1, (char *)&pt_tty, sizeof pt_tty,"ptms_tty structure");
	else
		readmem( (long)(addr + slot * sizeof pt_tty), 1, -1,
			(char *)&pt_tty,sizeof pt_tty,"ptms_tty structure");

	/*
	 * A free entry is one that does not have PTLOCK, PTMOPEN and
	 * PTSOPEN flags all set
	 */
	if ( !( pt_tty.pt_state & (PTMOPEN | PTSOPEN | PTLOCK)) && !all)
		return;


	if ( full || print_header) {
		print_header = 0;
		fprintf( fp, "%s", heading);
	}

	if ( ( slot == -1) && ( strcmp( type, ""))) {
		base = getaddr( type);
		count = getcount( type);
		slot = getslot( addr, base, sizeof pt_tty, phys, count);
	}

	if ( slot == -1)
		fprintf( fp, "  - ");
	else
		fprintf( fp, "%4d", slot);

	fprintf( fp, "%9x%9x%9x",	 pt_tty.ptm_wrq,
					 pt_tty.pts_wrq,
					 pt_tty.pt_bufp);

	fprintf(fp,"%s%s%s\n",
		pt_tty.pt_state & PTMOPEN ? " mopen" : "",
		pt_tty.pt_state & PTSOPEN ? " sopen" : "",
		pt_tty.pt_state & PTLOCK ? " lock" : "");

	if ( line_disp || ptem_flag) {
		offset = (long)(pt_tty.pts_wrq) - (long)(sizeof( struct queue));
		readmem( offset, 1, -1, (char *)&q, sizeof( struct queue), "");
		offset = prsptem( full, ptem_flag, q.q_next);
		if ( line_disp && offset && !prsldterm( full, offset))
			print_header = 1;
	}
	if ( pckt_flag) {
		offset = (long)(pt_tty.ptm_wrq) - (long)(sizeof( struct queue));
		readmem( offset, 1, -1, (char *)&q, sizeof( struct queue), "");
		prspckt( q.q_next);
	}
	return;
}
prsptem( all, print, addr)
int all;
int print;
long addr;
{
	char *heading = "\t  RQADDR MODNAME   MODID DACK_PTR   STATE\n";
	char mname[9];	/* Buffer for the module name */

	struct ptem ptem;
	struct queue q;
	struct qinit qinfo;
	struct module_info minfo;


	/*
	 * Wade through the link-lists to extract the line disicpline
	 * name and id
	 */
	readmem( addr, 1, -1, (char *)&q, sizeof( struct queue), "");
	/*
	 * q_next is zero at the stream head, i.e. no modules have
	 *  been pushed
	 */
	if ( !q.q_next)
		return( 0);
	readmem( (long)(q.q_qinfo), 1, -1, (char *)&qinfo, sizeof( struct qinit), "");
	readmem( (long)(qinfo.qi_minfo), 1, -1, (char *)&minfo, sizeof( struct module_info), "");
	readmem( (long)(minfo.mi_idname), 1, -1, mname, 8, "");
	mname[8] = '\0';

	readmem( (long)(q.q_ptr), 1, -1, (char *)&ptem, sizeof( struct ptem), "");

	if ( print) {

		fprintf( fp, "%s", heading);
		fprintf( fp, "\t%x %-9s%6d%9x         ", addr, mname, minfo.mi_idnum, ptem.ptem_dackp);
		fprintf( fp, "%s%s\n",
			 (ptem.ptem_state == OFLOW_CTL	? " oflow"    : ""),
			 (ptem.ptem_state == STRFLOW	? " strflow"    : ""));
	}

	if ( !all)
		return( (long)q.q_next);

	if ( print) {
		fprintf(fp,"\tcflag: %s%s%s%s%s%s%s%s%s%s%s%s%s%s",
			(ptem.ptem_cflags&CBAUD)==B0    ? " b0"    : "",
			(ptem.ptem_cflags&CBAUD)==B50   ? " b50"   : "",
			(ptem.ptem_cflags&CBAUD)==B75   ? " b75"   : "",
			(ptem.ptem_cflags&CBAUD)==B110  ? " b110"  : "",
			(ptem.ptem_cflags&CBAUD)==B134  ? " b134"  : "",
			(ptem.ptem_cflags&CBAUD)==B150  ? " b150"  : "",
			(ptem.ptem_cflags&CBAUD)==B200  ? " b200"  : "",
			(ptem.ptem_cflags&CBAUD)==B300  ? " b300"  : "",
			(ptem.ptem_cflags&CBAUD)==B600  ? " b600"  : "",
			(ptem.ptem_cflags&CBAUD)==B1200 ? " b1200" : "",
			(ptem.ptem_cflags&CBAUD)==B1800 ? " b1800" : "",
			(ptem.ptem_cflags&CBAUD)==B2400 ? " b2400" : "",
			(ptem.ptem_cflags&CBAUD)==B4800 ? " b4800" : "",
			(ptem.ptem_cflags&CBAUD)==B9600 ? " b9600" : "",
			(ptem.ptem_cflags&CBAUD)==B19200 ? " b19200" : "");
		fprintf(fp,"%s%s%s%s%s%s%s%s%s%s\n",
			(ptem.ptem_cflags&CSIZE)==CS5   ? " cs5"   : "",
			(ptem.ptem_cflags&CSIZE)==CS6   ? " cs6"   : "",
			(ptem.ptem_cflags&CSIZE)==CS7   ? " cs7"   : "",
			(ptem.ptem_cflags&CSIZE)==CS8   ? " cs8"   : "",
			(ptem.ptem_cflags&CSTOPB) ? " cstopb" : "",
			(ptem.ptem_cflags&CREAD)  ? " cread"  : "",
			(ptem.ptem_cflags&PARENB) ? " parenb" : "",
			(ptem.ptem_cflags&PARODD) ? " parodd" : "",
			(ptem.ptem_cflags&HUPCL)  ? " hupcl"  : "",
			(ptem.ptem_cflags&CLOCAL) ? " clocal" : "");

		fprintf( fp, "\tNumber of rows: %d\tNumber of columns: %d\n", ptem.ptem_wsz.ws_row, ptem.ptem_wsz.ws_col);
		fprintf( fp, "\tNumber of horizontal pixels: %d\tNumber of vertical pixels: %d\n", ptem.ptem_wsz.ws_xpixel, ptem.ptem_wsz.ws_ypixel);
	}

	return( (long)q.q_next);
}
prspckt( addr)
long addr;
{
	char *heading = "\t  RQADDR MODNAME   MODID\n";
	char mname[9];	/* Buffer for the module name */

	struct queue q;
	struct qinit qinfo;
	struct module_info minfo;


	/*
	 * Wade through the link-lists to extract the line disicpline
	 * name and id
	 */
	readmem( addr, 1, -1, (char *)&q, sizeof( struct queue), "");
	/*
	 * q_next is zero at the stream head, i.e. no modules have
	 *  been pushed
	 */
	if ( !q.q_next)
		return( 0);

	readmem( (long)(q.q_qinfo), 1, -1, (char *)&qinfo, sizeof( struct qinit), "");
	readmem( (long)(qinfo.qi_minfo), 1, -1, (char *)&minfo, sizeof( struct module_info), "");
	readmem( (long)(minfo.mi_idname), 1, -1, mname, 8, "");
	mname[8] = '\0';



	fprintf( fp, "%s", heading);
	fprintf( fp, "\t%x %-9s%6d\n", addr, mname, minfo.mi_idnum);

	return( 1);
}
