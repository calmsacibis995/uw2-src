/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mp.cmds:common/cmd/psrinfo/psrinfo.c	1.7"
/*
 * Command: psrinfo
 *
 *	psrinfo displays info about the specifics
 *	of the host multiprocessor system 
 *
 *	Usage  psrinfo [-v ] [ processor_id  [... ] ]
 *
 *	       psrinfo [-s ]  processor_id
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <priv.h> 
#include <sys/types.h>
#include <sys/processor.h>
#include <sys/prosrfs.h>
#include <sys/sysconfig.h>
#include <locale.h>
#include <pfmt.h>
#include <time.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <string.h>

static int flg_verbose=0;
static int flg_nengine=0;
static int flg_silent=0;
static int processor=(-1);	

static void usage_exit(void);
static void do_psrinfo(int, int, int);
static char * time_str(time_t*);
static char * eng_time(int);
static void failure(int);
static void mount_pfs(void);
static void p_line_ness(int );

extern int sysconfig(int);

int 
main(int argc, char *argv[])
{
	int c;
	char * ptr;
					/* setlocale with "" means use
					 * the LC_* environment variable
					 */
	(void) setlocale(LC_ALL, "");	
	(void) setcat("uxmp");		/* set default message catalog */
	(void) setlabel("UX:psrinfo");	/* set deault message label */


	while ((c = getopt(argc, argv, "vsn")) != EOF) {
		switch (c) {

		case 's':
		flg_silent=1;
		if ( flg_verbose ) usage_exit();
		break;

		case 'v':
		flg_verbose=1;
		if ( flg_silent ) usage_exit();
		break;

		case 'n':
		flg_nengine=1;
		break;

		case '?':
		usage_exit();

		}
	}

	if( flg_silent && flg_nengine ) 		/* only one possible */
		usage_exit();

	if ( flg_silent && argv[optind] == NULL )	/* need engine # */
		usage_exit();

	if ( flg_silent && argv[optind+1] != NULL ) 	/* want only 1 # */
		usage_exit();

	if ( flg_nengine && argv[optind] != NULL ) 	/* need no other args */
		usage_exit();

	if( flg_nengine ) {

		c = sysconfig(_CONFIG_NENGINE); /* proc's configured */
	
		if (c <  0){
	     		(void)pfmt(stderr, MM_ERROR,
	         		":1:cannot determine number of processors.\n");
		
	     		exit(1);
		}

		printf("%d\n",c);
		exit(0);
	}

	/*
	 * 	give info for each processor_id on the command line
	 */

	if ( argv[optind] ) {

		while (argv[optind]) {
			processor = strtol(argv[optind], &ptr, 10);
	
			if (ptr == argv[optind])  /* didn't string convert */
				usage_exit();
	
			do_psrinfo(processor, flg_silent, flg_verbose);
			optind++;
		}
	} else 	{

		/*
	 	 * give info about all configured processors
	 	 */
	
		c = sysconfig(_CONFIG_NENGINE); /* proc's configured */
	
		if (c <  0){
	     		(void)pfmt(stderr, MM_ERROR,
	         		":1:cannot determine number of processors.\n");
		
	     		exit(1);
		}
	

		mount_pfs();	/* try to mount the PFS */

		/* do it for all the processors */
	
		for(processor=0; processor < c; processor++)
			do_psrinfo(processor, flg_silent, flg_verbose);
	}

	exit(0);
	/*NOTREACHED*/
}

/*
 * print information about processor proc
 */

void 
do_psrinfo(int proc, int silent, int verbose)
{

	int state;
	processor_info_t pinfo;
	time_t t;


	state = p_online(proc,P_QUERY);
	if ( state < 0  ) {
		if ( silent ) {
			state = P_OFFLINE;
		} else {
			failure(proc);
			return;
		}
	}

	if ( silent ) {		/* jest print 0/1 */

		switch( state ){
		
			case P_ONLINE:
			(void)printf("1\n");	/* "1" is international */
			break;

			case P_OFFLINE:
			(void)printf("0\n");	/* "0" is international */
			break;

			default:
			(void)pfmt(stderr,MM_ERROR,":31:error processor %d\n",proc);
		}
		return;
	}

	if ( verbose  == 0 ) {		/* "half cooked" verbose mode */
		p_line_ness(proc);	/* up/down time for processor proc */
		return;
	}

	/* 
 	 *	FULL VERBOSE MODE 
	 *
	 *	Print out all available info  about processor proc
	 *
	 */

	(void)time(&t);

	(void)pfmt(stdout,MM_NOSTD,
		":32:Status of processor %d as of %s\n",
		 (int)proc,(char*) time_str(&t));
	
	if ( state == P_ONLINE )
		(void)pfmt(stdout,MM_NOSTD,
		":33:  Processor has been on-line since %s.\n",
		eng_time(proc));
	else
		(void)pfmt(stdout,MM_NOSTD,
		":34:  Processor has been off-line since %s.\n",
		eng_time(proc));

	if (_processor_info(proc, &pinfo) < 0 ) {
		failure(proc);		/* PFS is not mounted */
		return;			/* nothing more to do */
	}
	
	(void)pfmt(stdout,MM_NOSTD,
		":48:  The %s processor ",pinfo.pi_processor_type);

	if ( pinfo.pi_fputypes[0] == 0 )
		(void)pfmt(stdout,MM_NOSTD, 
		":36:has no floating point processor.\n");
	else
		(void)pfmt(stdout,MM_NOSTD, 
		":37:has a %s floating point processor.\n",
		pinfo.pi_fputypes);

	(void)pfmt(stdout,MM_NOSTD, ":38:  The following conditions exist:\n");
		
	if ( pinfo.pi_state & PISTATE_BOUND )
		(void)pfmt(stdout,MM_NOSTD,
		":39:\tDevice drivers are bound to this processor.\n");
	else 
		(void)pfmt(stdout,MM_NOSTD,
		":40:\tDevice drivers are not bound to this processor.\n");

	(void)printf("\n");
}

/*
 * print usage message and exit with error
 *
 *
 */

void
usage_exit(void)
{
	char *Usage0 = ":41:Usage:  psrinfo -v [ processor_id  [... ] ] \n";
	char *Usage1 = ":42:Usage:  psrinfo -s  processor_id \n";
	char *Usage2 = ":43:Usage:  psrinfo -n \n";

	(void)pfmt(stderr, MM_ERROR, ":7:Incorrect Usage\n");
	(void)pfmt(stderr, MM_ACTION, Usage0);
	(void)pfmt(stderr, MM_ACTION, Usage1);
	(void)pfmt(stderr, MM_ACTION, Usage2);

	exit(1);
}

/*
 *
 * return current time as ascii string of form "MM/DD/YY HH:MM:SS"
 */

char * 
time_str(time_t* t)
{
	static char t_buf[30];

	(void)cftime(t_buf, "%D %T", t);
	return(t_buf);
}

/*	
 *	print status and duration of (online/offline) status
 * 	for processor p
 */

void
p_line_ness(int p)
{
	int status;

	status = p_online(p,P_QUERY);

	if (status  < 0)  {

		failure(p);

	} else {

		if ( status == P_ONLINE) {
			(void)pfmt(stdout,MM_NOSTD,
			":44:%d on-line since %s\n",p,eng_time(p));
		} else  {
			(void)pfmt(stdout,MM_NOSTD,
			":45:%d off-line since %s\n",p,eng_time(p));
		}
	}
}


void
failure(int p)
{
        switch(errno) {

        case EINVAL:
        (void)pfmt(stderr, MM_ERROR, ":10:processor %d non-existent\n", p);
        break;

        case EIO:
        (void)pfmt(stderr, MM_ERROR, ":12:cannot reach processor %d\n", p);
        break;

	case ENOENT:
	(void)pfmt(stderr, MM_INFO, 
		":46:mount %s for more detailed info.\n",PFS_DIR);
	break;

	default:
        (void)pfmt(stderr, MM_ERROR, ":47:unknown error processor %d\n", p);
        break;
	
        }
}

/*
 *	The processor file system saves the state change information
 *	(last time online/offline state changed) in the vnode
 *	for that processor entry in the /sys/processor/nnn  directory
 *	
 *	ls -l reports last state change information 
 *	we get the information via stat(2)
 */

char * 
eng_time(int engine)
{
	char buf[64];
	char sbuf[12];
	struct stat statbuf;

        (void)strcpy(buf,PFS_DIR);                      /* the base directory */
        (void)strcat(buf,"/");
        (void)sprintf(sbuf,PFS_FORMAT,engine);/* convert to filename */
        (void)strcat(buf,sbuf);                 /* buf has full path name */

	if ((stat(buf,&statbuf)) != 0 ){
		return("?/?/? ?:?:?");
	}
	return(time_str((time_t*)&(statbuf.st_mtime)));
}

/*
 *	If the "standard" mount point exists
 *	and the processor file system is unmounted:
 *
 *	Silently try to mount it in its standard mount point.
 */

void
mount_pfs(void)
{
	struct stat statbuf;

	/*
         * find out if the processor file system is mounted
         * by doing a stat on the control file.
         */

        if ((stat(PFS_CTL_FILE,&statbuf)) == 0 ){
                return;		/* already mounted */
        }

	/*
	 * stat the mount point
	 * if it's there we will try to mount
	 */

        if ((stat(PFS_DIR,&statbuf)) != 0){
                return;		
	}

	/*
	 * mount point is there..It's jest not mounted
	 * give it a try
	 */

	 (void)mount(PFSTYPE,PFS_DIR,MS_DATA,PFSTYPE,(char*)NULL,NULL);
}
