/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

#ident	"@(#)sfs.cmds:common/cmd/fs.d/sfs/volcopy/volcopy.c	1.1.8.9"
#ident "$Header: volcopy.c 1.3 91/06/28 $"


/***************************************************************************
 * Command: volcopy
 * Inheritable Privileges: P_SETFLEVEL,P_SYSOPS P_MACREAD,P_MACWRITE,P_DACREAD,
 *				P_DACWRITE,P_DEV
 *       Fixed Privileges: None
 * Notes:
 *
 ***************************************************************************/

#include <sys/param.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/vnode.h>
#include <sys/acl.h>
#include <sys/fs/sfs_fsdir.h>
#include <sys/fs/sfs_inode.h>
#include <sys/fs/sfs_fs.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <varargs.h>
#include <sys/errno.h>
#include <sys/utsname.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <archives.h>
#include <libgenIO.h>
#include <mac.h>
#include <priv.h>
#include "volcopy.h"

extern
int	errno;

/*
 * main I/O information structure, contains information for the
 * source and destination files.
 */

struct file_info {
	char	*f_dev_p,	/* name of device */
		*f_vol_p;	/* volume name */
	int	f_bsize,	/* size to buffer I/O to */
		f_des,		/* file descriptor */
		f_dev;		/* device type (generic I/O library) */
} In, Out;

#define IFTAPE(s) (equal(s,"/dev/rmt",8) || equal(s,"rmt", 3))

int	Sem_id[BUFCNT],	/* semaphore ids for controlling shared memory */
	Shm_id[BUFCNT],	/* shared memory identifier */
	*Cnts[BUFCNT];	/* an array of byte counts for shared memory */

char	Empty[BLKSIZ],	/* empty memory used to clear sections of memory */
	*Buf[BUFCNT],	/* buffer pointers (possibly to shared memory) */
	*sbrk();

struct sembuf	Sem_buf,	/* semaphore operation buffer */
		Rstsem_buf;	/* semaphore reset operation buffer */

char   sblock[SBSIZE];	/* disk buffer for sfs super block */

typedef union {
	char            dummy[SBSIZE];
	struct fs       sblk;

} sb_un;

sb_un	isup, osup;

#define Isup isup.sblk
#define Osup osup.sblk

struct fs *Sptr = (struct fs *) sblock;	/* super-block pointer */

char    *Ifname, *Ifpack, *Ofname, *Ofpack;
struct volcopy_label	V_labl;

/* data structure for security information.  security header is written */
/* to tape following volcopy label.  On tape-to-disk copy, information  */
/* from tape's security header is compared to system information,       */
/* warning is issued if any does not match. */
/* labelit for sfs also declares and uses this structure */

#define SFILLEN 236

struct sec_hdr {  
        long    sh_fs_magic;       /* file system type magic number */
#if (defined (_POSIX_SOURCE) || defined(_XOPEN_SOURCE)) && !defined(_KERNEL) && !defined(_STYPES)
	char 	sh_host[_SYS_NMLN]; /* name of source system */
#else
	char 	sh_host[SYS_NMLN]; /* name of source system */
#endif
	time_t 	sh_db_cdate;       /* ltdb creation date */
	time_t  sh_db_mdate;       /* ltdb last modification date */
	off_t	sh_db_size;        /* ltdb size */
	char	sh_fill[SFILLEN];
} in_sec_hdr, sys_sec_hdr;

char	From_vol[VVOLLEN + 1],
	To_vol[VVOLLEN + 1],
	*Fsys_p;

int	Blk_cnt = 1,	/* Do I/O in (Blk_cnt * BLKSIZ) byte blocks */
	Blocks = 0,	/* Number of blocks transferred */
	Bpi = 0,
	Bufcnt,
	Bufflg = 0,
	Bufsz = BLKSIZ,
	Disk_cnt = 1,	/* Disk I/O (Disk_cnt * Blk_cnt * BLKSIZ) byte blocks */
	Drive_typ = 0,	/* Flag for special tape drive types */
	Eomflg = 0,
	Ipc = 1,
	Itape,
	Otape,
	Pid = -1,
	R_blks = 0,	/* Number of blocks per tape reel */
	R_cur = 1,	/* Current tape reel being processed */
	R_len = 0,	/* Length in feet of tape reels */
	R_num = 0,	/* Number of tape reels to be processed */
	Yesflg = 0,
        macpkg = 1;     /* flag - Mandatory Access Controls installed? */

long	Fs,
	Fstype,
	Tvec;

FILE	*Devtty;

static void     chk_sec_hdr(),
		dev_range_chk(),
		fill_sec_hdr(),
                getinfs(), 
		getoutfs();


static char *   getfslabel();
static char *   getvolabel();


/*
 * Procedure:     main
 *
 * Restrictions:
 *                fopen: none
 *                isatty: none
 *                open(2): none
 *                g_init: none
 *                g_read: none
 *                printf: none
 *                ctime: P_MACREAD
 *                g_write: none
*/
/*
 * Notes:
 *
 * filesystem copy with propagation of volume ID and filesystem name:
 *
 * volcopy [-options]  filesystem /dev/from From_vol /dev/to To_vol
 *
 * options are:
 * 	-feet - length of tape
 * 	-bpi  - recording density
 * 	-reel - reel number (if not starting from beginning)
 * 	-buf  - use double buffered i/o (if dens >= 1600 bpi)
 *	-block - Set the transfer block size to NUM physical blocks (512
 *	bytes on 3B2 and 3B15).  Note that an arbitrary block size might
 *	or might not work on a given system.  Also, the block size
 *	read from the header of an input tape silently overrides this.
 *	-r - Read NUM transfer blocks from the disk at once and write it
 *	to the output device one block at a time.  Intended only to
 *	boost the 3B15 EDFC disk to tape performance.  Disabled on 3B2.
 * 	-a    - ask "y or n" instead of "DEL if wrong"
 * 	-s    - inverse of -a, from/to devices are printed followed by `?'.
 * 		User has 10 seconds to DEL if mistaken!
 * 	-y    - assume "yes" response to all questions
 * 
 * Examples:
 *
 * volcopy root /dev/rdsk/0s2 pk5 /dev/rdsk/1s2 pk12
 * 
 * volcopy u3 /dev/rdsk/1s5 pk1 /dev/rmt/0m tp123
 * 
 * volcopy u5 /dev/rmt/0m -  /dev/rdsk/1s5 -
 */

main(argc, argv)
int argc;
char **argv;
{
	register char c;
	register int lfdes, altflg = 0, result, verify;
	register long dist;
	char *align();
	void sigalrm(), sigint();
	struct stat stbuf;
	int cnt;
        level_t tmp_lid;
	char *date;
	(void)signal(SIGINT, sigint);
	(void)signal(SIGALRM, sigalrm);
	In.f_bsize = Out.f_bsize = BLKSIZ;

/* Check if mac is installed.  The macpkg variable is checked by */
/* dev_range_chk(),fill_sec_hdr(), and chk_sec_hdr() */

        if ((lvlproc(MAC_GET,&tmp_lid) != 0) && (errno==ENOPKG))
           macpkg=0;

	while (argc > 1 && argv[1][0] == '-') {
		if (EQ(argv[1], "-a", 2)) {
			altflg |= MINUSA;
		} else if (EQ(argv[1], "-e", 2)) {
			Eomflg = 1;
		} else if (EQ(argv[1], "-s", 2)) {
			altflg |= MINUSS;
		} else if (EQ(argv[1], "-y", 2)) {
			Yesflg++;
		} else if (EQ(argv[1], "-buf", 4)) {
			Bufflg++;
		} else if (EQ(argv[1], "-bpi", 4)) {
			if ((c = argv[1][4]) >= '0' && c <= '9')
				Bpi = atoi(&argv[1][4]);
			else {
				++argv;
				--argc;
				Bpi = atoi(&argv[1][0]);
			}
		} else if (EQ(argv[1], "-feet", 5)) {
			if ((c = argv[1][5]) >= '0' && c <= '9')
				R_len = atoi(&argv[1][5]);
			else {
				++argv;
				--argc;
				R_len = atoi(&argv[1][0]);
			}
		} else if (EQ(argv[1], "-reel", 5)) {
			if ((c = argv[1][5]) >= '0' && c <= '9')
				R_cur = atoi(&argv[1][5]);
			else {
				++argv;
				--argc;
				R_cur = atoi(&argv[1][0]);
			}
		} else if (EQ(argv[1], "-r", 2)) { /* 3b15 only */
			if ((c = argv[1][2]) >= '0' && c <= '9')
				Disk_cnt = atoi(&argv[1][2]);
			else {
				++argv;
				--argc;
				Disk_cnt = atoi(&argv[1][0]);
			}
			if (Disk_cnt == 0)
				perr(1, "volcopy: Need a non-zero value for the -r option\n");
		} else if (EQ(argv[1], "-block", 6)) { /* 3b15 only */
			if ((c = argv[1][6]) >= '0' && c <= '9')
				Blk_cnt = atoi(&argv[1][6]);
			else {
				++argv;
				--argc;
				Blk_cnt = atoi(&argv[1][0]);
			}
			if (Blk_cnt == 0)
				perr(1, "volcopy: Need a non-zero value for the -block option\n");
		} else
			perr(1, "<%s> invalid option\n", argv[1]);
		++argv;
		--argc;
	} /* argv[1][0] == '-' */

	Devtty = fopen("/dev/tty", "r");
	if ((Devtty == NULL) && !isatty(0))
		Devtty = stdin;
	time(&Tvec);

	if (Eomflg && R_len)
		perr(9, "volcopy: -e and -feet are mutually exclusive\n");
	if ((altflg & MINUSA) && (altflg & MINUSS))
		perr(9, "volcopy: -a and -s are mutually exclusive\n");
	if (argc != 6) /* if mandatory fields not present */
		perr(9, "sfs usage: volcopy [-F sfs] [generic options] fsname /devfrom volfrom /devto volto\n");
	if (!(altflg & MINUSA)) /* -a was not specified, use default (-s) */
		altflg |= MINUSS;

	In.f_dev_p = argv[DEV_IN];
	Out.f_dev_p = argv[DEV_OUT];
	strncpy(To_vol, argv[VOL_OUT], VVOLLEN);
	To_vol[VVOLLEN] = '\0';
	Out.f_vol_p = &To_vol[0];
	strncpy(From_vol, argv[VOL_IN], VVOLLEN);
	From_vol[VVOLLEN] = '\0';
	In.f_vol_p = &From_vol[0];
	Fsys_p = argv[FIL_SYS];

	if ((In.f_des = open(In.f_dev_p, O_RDONLY)) < 1)
		perr(10, "%s: cannot open\n", In.f_dev_p);
	if ((Out.f_des = open(Out.f_dev_p, O_RDONLY)) < 1)
		perr(10, "%s: cannot open\n", Out.f_dev_p);

	if (fstat(In.f_des, &stbuf) < 0 || (stbuf.st_mode & S_IFMT) != S_IFCHR)
		perr(10, "From device not character-special\n");
	if (fstat(Out.f_des, &stbuf) < 0 || (stbuf.st_mode & S_IFMT) != S_IFCHR)
		perr(10, "To device not character-special\n");

	if ((Itape = tapeck(&In, INPUT)) == 1)
		R_blks = V_labl.v_reelblks;
	Otape = tapeck(&Out, OUTPUT);
	if (Otape && Itape)
		perr(10, "Use dd(1) command to copy tapes\n");

        /* compare level ranges of input and output devices */
        dev_range_chk(In.f_des,Out.f_des);

	(void)mem_setup();
	if (Bufflg && !Ipc)
		perr(1, "The -buf option requires ipc\n");
	if (!Itape && !Otape)
		R_cur = 1;
	if (R_cur == 1 || !Itape) {
                /* for tape, move past security header to reach superblock*/
                if (Itape)
                    if (g_read(In.f_dev, In.f_des, &in_sec_hdr, sizeof(in_sec_hdr)) != sizeof(in_sec_hdr))
			perr(10, "Read of sec_hdr failed\n");
		/* read in superblock */
		verify = 0;
		(void) getinfs(In.f_dev, In.f_des, Sptr);

		if (Sptr->fs_magic != SFS_MAGIC)
			perr(10, "File System type unknown--get help\n");

		(void)memcpy(&Isup, Sptr, Sptr->fs_sbsize);
		Ifname = getfslabel(&Isup);
		Ifpack = getvolabel(&Isup);
		Fs = Sptr->fs_size * Sptr->fs_nspf;
	}  /* R_cur == 1 || !Itape */

	/* read in superblock */
	verify = !Otape || (altflg & MINUSS);
	(void) getoutfs(Out.f_dev, Out.f_des, Sptr, verify);

	if (Sptr->fs_magic == SFS_MAGIC) {
		(void)memcpy(&Osup, Sptr, Sptr->fs_sbsize);
		Ofname = getfslabel(&Osup);
		Ofpack = getvolabel(&Osup);
	}
	else  {
		int i;

		/* out vol does not contain a sfs file system */
		/* stuff let over from Isup */
		(void)memcpy(&Osup, &Isup, Isup.fs_sbsize);
		Ofname = getfslabel(&Osup);
		Ofpack = getvolabel(&Osup);
		/* wipe out the fs name and pack name for warning purposes */
		for (i=0;i<6;i++) Ofname[i]=' ';
		for (i=0;i<6;i++) Ofpack[i]=' ';
	}

	if (Itape) {
		if (R_cur != 1) {
                	(void)printf("\nvolcopy: IF REEL 1 HAS NOT BEEN RESTORED,");
			(void)printf(" STOP NOW AND START OVER ***\07\n");
			if (!ask(" Continue? ")) {
				cleanup();
				exit(31+9);
			}
			strncpy(Ifname, Fsys_p, 6);
			strncpy(Ifpack, In.f_vol_p, 6);
		}
		if (V_labl.v_reel != R_cur || V_labl.v_reels != R_num)
			prompt(1, "Tape disagrees: Reel %d of %d : looking for %d of %d\n",
				V_labl.v_reel, V_labl.v_reels, R_cur, R_num);
	} else if (Otape) {
		strncpy(V_labl.v_volume, Out.f_vol_p, 6);
		strncpy(Ofpack, Out.f_vol_p, 6);
		strncpy(Ofname, Fsys_p, 6);
		if (!Eomflg) {
			R_num = Fs / R_blks + ((Fs % R_blks) ? 1 : 0);
			(void)printf("You will need %d reels.\n", R_num);
			(void)printf("(\tThe same size and density");
			(void)printf(" is expected for all reels)\n");
		}
	}
	if (NOT_EQ(Fsys_p, Ifname, 6)) {
		verify = !Otape || (altflg & MINUSS);
		prompt(verify, "arg. (%.6s) doesn't agree with from fs. (%.6s)\n",
			Fsys_p, Ifname);
	}
	if (NOT_EQ(In.f_vol_p, "-", 6) && NOT_EQ(In.f_vol_p, Ifpack, 6)) {
		verify = !Otape || (altflg & MINUSS);
		prompt(verify, "arg. (%.6s) doesn't agree with from vol.(%.6s)\n",
			In.f_vol_p, Ifpack);
	}

	if (*In.f_vol_p == '-')
		In.f_vol_p = Ifpack;
	if (*Out.f_vol_p == '-')
		Out.f_vol_p = Ofpack;

	if (R_cur == 1 && (Osup.fs_time + _2_DAYS) > Isup.fs_time) {
		verify = altflg & MINUSS;

		procprivl(CLRPRV,MACREAD_W,0);
		date = ctime(&Osup.fs_time);
		procprivl(SETPRV,MACREAD_W,0);

		prompt(verify, "%s less than 48 hours older than %s\nTo filesystem dated:  %s", Out.f_dev_p, In.f_dev_p, date);
	}
	if (NOT_EQ(Out.f_vol_p, Ofpack, 6)) {
		prompt(1, "arg.(%.6s) doesn't agree with to vol.(%.6s)\n",
			Out.f_vol_p, Ofpack);
		strncpy(Ofpack, Out.f_vol_p, 6);
	}
	if (Isup.fs_size > Osup.fs_size && !Otape)
		prompt(1, "from fs larger than to fs\n");
	if (!Otape && NOT_EQ(Ifname, Ofname, 6)) {
		verify = altflg & MINUSS;
		prompt(verify, "warning! from fs(%.6s) differs from to fs(%.6s)\n",
			Ifname, Ofname);
	}

	(void)printf("From: %s, to: %s? ", In.f_dev_p, Out.f_dev_p);
	if (!(altflg & MINUSA)) {
		(void)printf("(DEL if wrong)\n");
		sleep(10);
	} else if (!ask("(y or n) "))
		perr(10, "\nvolcopy: STOP\n");
	close(In.f_des);
	close(Out.f_des);
	sync();
	In.f_des = open(In.f_dev_p, O_RDONLY);
	Out.f_des = open(Out.f_dev_p, O_WRONLY);
	errno = 0;
	if (g_init(&In.f_dev, &In.f_des) < 0 || g_init(&Out.f_dev, &Out.f_des) < 0)
		perr(1, "volcopy: Error %d during initialization\n", errno);


/* Fill security header if tape-to-disk or disk-to-tape copy */

        if(Itape||Otape)
           fill_sec_hdr(&sys_sec_hdr);


	if (Itape) {
		errno = 0;
		if (g_read(In.f_dev, In.f_des, &V_labl, sizeof(V_labl)) < sizeof(V_labl))
			perr(10, "Error while reading label\n");
		if (g_read(In.f_dev, In.f_des, &in_sec_hdr, sizeof(in_sec_hdr)) < sizeof(in_sec_hdr))
			perr(10, "Error while reading security header\n");
                chk_sec_hdr(&in_sec_hdr,&sys_sec_hdr);
	} else if (Otape) {
		V_labl.v_reels = R_num;
		V_labl.v_reel = R_cur;
		V_labl.v_time = Tvec;
		V_labl.v_reelblks = R_blks;
		V_labl.v_blksize = BLKSIZ * Blk_cnt;
		V_labl.v_nblocks = Blk_cnt;
		V_labl.v_offset = 0L;
		V_labl.v_type = T_TYPE;
		errno = 0;
		if (g_write(Out.f_dev, Out.f_des, &V_labl, sizeof(V_labl)) < sizeof(V_labl))
			perr(10, "Error while writing label\n");
                if (g_write(Out.f_dev, Out.f_des, &sys_sec_hdr, sizeof(sys_sec_hdr)) != sizeof(sys_sec_hdr))
                        perr(10, "Error while writing security header\n");

	}
	if (R_cur > 1) {
		if (!Eomflg) {
			Fs = (R_cur - 1) * actual_blocks();
			lfdes = Otape ? In.f_des : Out.f_des;
			dist = (long)(Fs * BLKSIZ);
		} else { /* Eomflg */
			if (Otape)
				perr(1, "Cannot use -reel with -e when copying to tape\n");
			lfdes = Out.f_des;
			dist = (long)(V_labl.v_offset * BLKSIZ);
			Fs = V_labl.v_offset;
		}
		if (lseek(lfdes, dist, 0) < 0)
			perr(1, "Cannot lseek()\n");
		Sptr = Otape ? &Isup : &Osup;
		if (Sptr -> fs_magic !=  SFS_MAGIC)
			perr(10, "File System type unknown--get help!\n");
		Fs = (Sptr->fs_size * Sptr->fs_nspf) - Fs;
	}
	if (Itape || Otape)
		rprt();

	if (Ipc) {
		parent_copy();
		(void)cleanup();
	} else
		copy();
	(void)printf("  END: %ld blocks.\n", Blocks);

	fslog();
	if (Blocks)
		exit(0);
	exit(31+1);		/* failed.. 0 blocks */
}


/*
 * Procedure:     sigalrm
 *
 * Restrictions:  none
 *
 * Notes:
 *	catch alarm signals.
 */

void
sigalrm(sig)
int sig;
{

	(void)signal(SIGALRM, sigalrm);
}


/*
 * Procedure:     sigsys
 *
 * Restrictions:  none
 *
 * Notes:
 *
 *	catch illegal system calls to determine if IPC is available.
 */

void
sigsys(sig)
int sig;
{
	Ipc = 0;
}


/*
 * Procedure:     sigint
 *
 * Restrictions:  none
 * Notes:
 *	catch interrupts and prompt user to quit.
 */

void
sigint(sig)
int sig;
{
	register int tmpflg; 

	tmpflg = Yesflg; /* override yesflag for duration of interrupt */
	Yesflg = 0;		
	if (ask("Want to quit?    ")) {
		if (Pid > 0)
			kill(Pid, 9);
		(void)cleanup(); /* ipc */
		exit(31+2);
	}
	(void)signal(SIGINT, sigint);
	Yesflg = tmpflg; /* reset Yesflg */
}


/*
 * Procedure:     actual_blocks
 *
 * Restrictions:  none	
 *
 * Notes:
 *
 * actual_blocks: Calculate the actual number of blocks written to
 * the tape (will differ from V_labl.v_reelblks if v_reelblks is not
 * an even multiple of the blocking factor Blk_cnt).
 */

int
actual_blocks()
{

	if (R_blks % Blk_cnt)
		return(((R_blks / Blk_cnt) + 1) * Blk_cnt);
	else
		return(R_blks);
}


/*
 * Procedure:     mem_setup
 *
 * Restrictions:
 *                shmat(2): none
 *                semctl(2): none
 *                shmctl(2):  none
 * Notes:
 *
 * Determine memory needs and check for IPC.  If IPC is available,
 * use shared memory and semaphores to increase performance.  If no IPC,
 * get normal memory and only use one process.
 */

int
mem_setup()
{
	register int cnt, num, size;
	char *align();

	union semun {
		int val;
		struct semid_ds *buf;
		ushort *array;
	} sem_arg;

	if (Blk_cnt == 1) {
		switch (Drive_typ) {
		case A_DRIVE:
			Blk_cnt = 32;
			break;
		case C_DRIVE:
			Blk_cnt = 10;
			break;
		case K_DRIVE:
			Blk_cnt = 4;
			break;
		case T_DRIVE:
			if (Bpi == 6250)
				Blk_cnt = 50;
			else
				Blk_cnt = 10;
			break;
		default:
			if (Otape || Itape) {
				if (Bpi == 6250)
					Blk_cnt = 50;
				else
					Blk_cnt = 10;
			}
			break;
		} /* Drive_typ */
	} /* Blk_cnt == 1 */
	if (Blk_cnt > 1) /* user overrode g_init */
		In.f_bsize = Out.f_bsize = Blk_cnt * BLKSIZ;
	In.f_bsize = (!Itape) ? Disk_cnt * In.f_bsize : In.f_bsize;
	Out.f_bsize = (!Otape) ? Disk_cnt * Out.f_bsize : Out.f_bsize;
	Bufsz = find_lcm(In.f_bsize, Out.f_bsize);
	num = _128K / (Bufsz + sizeof(int));
	Bufsz *= num;
	size = Bufsz + sizeof(int);
	/* test to see if ipc is available, the shmat should fail with EINVAL */
	(void)signal(SIGSYS, sigsys);
	errno = 0;
	if (Ipc) {
		if ((int)shmat(0, (char *)NULL, 0) < 0 && errno != EINVAL)
			Ipc = 0; /* something went wrong */
	}
	if (Ipc) { /* ipc is available */
		Bufcnt = 2;
		sem_arg.val = 0;
		for (cnt = 0; cnt < BUFCNT; cnt++) {
			errno = 0;
			if ((Sem_id[cnt] = semget(IPC_PRIVATE, 1, 0)) < 0) 
				perr(1, "Error allocating semaphores: %d", errno);
			if (semctl(Sem_id[cnt], 0, SETVAL, sem_arg) < 0)
				perr(1, "Error setting semaphores: %d", errno);
			if ((Shm_id[cnt] = shmget(IPC_PRIVATE, size, 0)) < 0)
				perr(1, "Error allocating shared memory: %d", errno);
			if ((Buf[cnt] = (char *) shmat(Shm_id[cnt], 0, 0)) < (char *)0)
				perr(1, "Error attaching shared memory: %d", errno);
			if (shmctl(Shm_id[cnt], SHM_LOCK, 0) < 0)
				perr(0, "Error locking in shared memory: %d", errno);
			Cnts[cnt] = (int *)(Buf[cnt] + Bufsz);
		}
	} else { /* ipc is not available */
		Bufcnt = 1;
		if ((Buf[0] = align(size)) == (char *)NULL)
			perr(1, "Out of memory\n");
		Cnts[0] = (int *)(Buf[0] + Bufsz);
		*Cnts[0] = 0;
	}
}


/*
 * Procedure:     prompt
 *
 * Restrictions:
 *                vfprintf: none
 *                fprintf: none
 * Notes:
 *
 *	Prompt the user for verification.
 */

int
prompt(va_alist)
va_dcl
{
	register char *fmt_p;
	register int verify;
	va_list v_Args;

	va_start(v_Args);
	verify = va_arg(v_Args, int);
	if ((fmt_p = va_arg(v_Args, char *)) != (char *)NULL)
		(void)vfprintf(stdout, fmt_p, v_Args);
	if (verify) {
		(void)fprintf(stdout, "Type 'y' to override:    ");
		if (!ask("")) {
			cleanup();
			exit(31+9);
		}
	}
}


/*
 * Procedure:     ask
 *
 * Restrictions:
 *                printf: none
 *                fgets: none
 *
 * Notes:
 *
 *	Ask the user a question and get the answer.
 */

ask(s)
char *s;
{
	char ans[12];

	(void)printf(s);
	if (Yesflg) {
		(void)printf("YES\n");
		return(1);
	}
	ans[0] = '\0';
	fgets(ans, 10, Devtty);
	for (;;) {
		switch (ans[0]) {
			case 'a':
			case 'A':
				if (Pid > 0) /* parent with a child */
					kill(Pid, 9);
				cleanup();
				exit(31+1);
			case 'y':
			case 'Y':
				return(1);
			case 'n':
			case 'N':
				return(0);
			default:
				(void)printf("\n(y or n)? ");
				fgets(ans, 10, Devtty);
		}
	}
}


/*
 * Procedure:     align
 *
 * Restrictions:  none
 *
 * Notes:
 *
 * 	Align a malloc'd memory section on a page boundry.
 */

char *
align(size)
register int size;
{
	register int pad;
	char *sbrk();

	if ((pad = ((int)sbrk(0) & 0x7FF)) > 0) {
		pad = 0x800 - pad;
		if (sbrk(pad) == (char *)NULL)
			return((char *)NULL);
	}
	return(sbrk((unsigned)size));
}


/*
 * Procedure:     child_copy
 *
 * Restrictions:
 *                semop(2): none
 *                g_write: none
 *
 *
 * notes:  Using IPC, this child process reads from shared memory
 * and writes to the destination file system.
 */

int
child_copy()
{
	register int rv, cur_buf, left, have, tpcnt;
	register char *c_p;

	(void)signal(SIGINT, SIG_IGN);
	Sem_buf.sem_op = -1;
	(void)close(In.f_des);
	if (Otape && !Eomflg)
		tpcnt = actual_blocks() * BLKSIZ;
	cur_buf = 0;
	for (;;) {
		if (semop(Sem_id[cur_buf], &Sem_buf, 1) < 0)
			perr(1, "semaphore operation error %d\n", errno);
		left = *Cnts[cur_buf];
		if (!left)
			break;
		c_p = Buf[cur_buf];
		rv = 0;
		while (left) {
			have = (left < Out.f_bsize) ? left : Out.f_bsize;
			if (!Eomflg && Otape) {
				if (!tpcnt) {
					(void)chgreel(&Out, OUTPUT);
					tpcnt = actual_blocks() * BLKSIZ;
				}
				have = (tpcnt < have) ? tpcnt : have;
			}
			errno = 0;
			if ((rv = g_write(Out.f_dev, Out.f_des, c_p, have)) < 0) {
				if (Eomflg && errno == ENOSPC) {
					(void)chgreel(&Out, OUTPUT);
					continue;
				} else
					perr(1, "I/O error %d on write\n", errno);
			}
			left -= rv;
			c_p += rv;
			V_labl.v_offset += rv;
			if (!Eomflg && Otape)
				tpcnt -= rv;
		}
		if (semop(Sem_id[cur_buf], &Sem_buf, 1) < 0)
			perr(2, "semaphore operation error %d\n", errno);
		cur_buf = (cur_buf + 1) % BUFCNT;
	}
	exit(0);
}


/*
 * Procedure:     parent_copy
 *
 * Restrictions:
 *                fork(2):  none
 *                semop(2):  none
 *                g_read: none
 *
 * Notes:
 *
 * Using shared memory, the parent process reads from the
 * source file system and writes to shared memory.
 */

int
parent_copy()
{
	register int rv, left, have, tpcnt, cur_buf;
	register char *c_p;
	int eom = 0, xfer_cnt = Fs * BLKSIZ;

	Sem_buf.sem_num = 0;
	Sem_buf.sem_flg = 0;
	(void)fflush(stderr);
	(void)fflush(stdout);
	if ((Pid = fork()) == 0)
		child_copy(); /* child does not return */
	(void)close(Out.f_des);
	Rstsem_buf.sem_num = 0;
	Rstsem_buf.sem_flg = 0;
	Rstsem_buf.sem_op = 2;
	Sem_buf.sem_op = 0;
	cur_buf = 0;
	if (Itape && !Eomflg)
		tpcnt = actual_blocks() * BLKSIZ;
	while (xfer_cnt) {
		if (semop(Sem_id[cur_buf], &Sem_buf, 1) < 0)
			perr(1, "Semaphore operation error %d\n", errno);
		c_p = Buf[cur_buf];
		left = Bufsz;
		rv = 0;
		while (left >= In.f_bsize && xfer_cnt) {
			have = (xfer_cnt < In.f_bsize) ? xfer_cnt : In.f_bsize;
			if (!Eomflg && Itape) {
				if (!tpcnt) {
					*Cnts[cur_buf] = Bufsz - left;
					(void)flush_bufs(cur_buf);
					(void)chgreel(&In, INPUT);
					tpcnt = actual_blocks() * BLKSIZ;
					cur_buf = (cur_buf == 0) ? 1 : 0;
					eom = 1;
					break;
				}
				have = (tpcnt < have) ? tpcnt : have;
			}
			errno = 0;
			if ((rv = g_read(In.f_dev, In.f_des, c_p, have)) < 0) {
				if (Eomflg && errno == ENOSPC) {
					*Cnts[cur_buf] = Bufsz - left;
					(void)flush_bufs(cur_buf);
					(void)chgreel(&In, INPUT);
					cur_buf = (cur_buf == 0) ? 1 : 0;
					eom = 1;
					break;
				} else 
					perr(1, "I/O error %d on read\n", errno);
			}
			left -= rv;
			c_p += rv;
			xfer_cnt -= rv;
			if (!Eomflg && Itape)
				tpcnt -= rv;
		}
		if (eom > 0) {
			eom = 0;
			if (Eomflg)
				xfer_cnt -= rv;
			else if (Itape)
				tpcnt -= rv;
			continue;
		}
		*Cnts[cur_buf] = Bufsz - left;
		Blocks += *Cnts[cur_buf];
		if (semop(Sem_id[cur_buf], &Rstsem_buf, 1) < 0)
			perr(2, "Semaphore operation error %d\n", errno);
		cur_buf = (cur_buf == 0) ? 1 : 0;
	}
	if (semop(Sem_id[cur_buf], &Sem_buf, 1) < 0)
		perr(3, "Semaphore operation error %d\n", errno);
	*Cnts[cur_buf] = 0;
	if (semop(Sem_id[cur_buf], &Rstsem_buf, 1) < 0)
		perr(4, "Semaphore operation error %d\n", errno);
	wait((int *)NULL);
	Blocks /= BLKSIZ;
}


/*
 * Procedure:     copy
 *
 * Restrictions:
 *                g_read: none
 *                g_write: none
 *
 * Notes:
 *
 * copy:  Copy without shared memory.  The process reads from the source
 * filesystem and writes to the destination filesystem.
 */

int
copy()
{
	register int rv, left, have, tpcnt = 1, xfer_cnt = Fs * BLKSIZ;
	register char *c_p;

	if ((Itape || Otape) && !Eomflg)
		tpcnt = actual_blocks() * BLKSIZ;
	while (xfer_cnt) {
		c_p = (char *)(Buf[0] + *Cnts[0]);
		left = Bufsz - *Cnts[0];
		rv = 0;
		while (left >= In.f_bsize && xfer_cnt) {
			have = (xfer_cnt < In.f_bsize) ? xfer_cnt : In.f_bsize;
			if (!Eomflg && Itape) {
				if (!tpcnt) {
					*Cnts[0] = Bufsz - left;
					(void)chgreel(&In, INPUT);
					tpcnt = actual_blocks() * BLKSIZ;
					break;
				}
				have = (tpcnt < have) ? tpcnt : have;
			}
			errno = 0;
			if ((rv = g_read(In.f_dev, In.f_des, c_p, have)) < 0) {
				if (Eomflg && errno == ENOSPC) {
					(void)chgreel(&In, INPUT);
					break;
				} else 
					perr(1, "I/O error %d on read\n", errno);
			}
			left -= rv;
			c_p += rv;
			xfer_cnt -= rv;
			if (!Eomflg && Itape)
				tpcnt -= rv;
		} /* left >= In.f_bsize && xfer_cnt */
		*Cnts[0] = Bufsz - left;
		Blocks += *Cnts[0];
		c_p = Buf[0];
		left = *Cnts[0];
		rv = 0;
		while (left >= Out.f_bsize || (left > 0 && !xfer_cnt)) {
			have = (left < Out.f_bsize) ? left : Out.f_bsize;
			if (!Eomflg && Otape) {
				if (!tpcnt) {
					(void)chgreel(&Out, OUTPUT);
					tpcnt = actual_blocks() * BLKSIZ;
				}
				have = (tpcnt < have) ? tpcnt : have;
			}
			errno = 0;
			if ((rv = g_write(Out.f_dev, Out.f_des, c_p, have)) < 0) {
				if (Eomflg && errno == ENOSPC) {
					(void)chgreel(&Out, OUTPUT);
					continue;
				} else
					perr(1, "I/O error on %d write\n", errno);
			}
			left -= rv;
			c_p += rv;
			V_labl.v_offset += rv;
			if (!Eomflg && Otape)
				tpcnt -= rv;
		} /* left >= Out.f_bsize */
		if (left) {
			(void)memcpy(Buf[0], c_p, left);
			Blocks -= left;
		}
		*Cnts[0] = left;
	} /* xfer_cnt */
	Blocks /= BLKSIZ;
}


/*
 * Procedure:     flush_bufs
 *
 * Restrictions:
 *                 semop(2): none
 *
 * Notes:
 *
 * Permit child to read the remaining data from the
 * buffer before prompting user for end-of-media.
 */

int
flush_bufs(buffer)
register int buffer;
{

	Blocks += *Cnts[buffer];
	if (semop(Sem_id[buffer], &Rstsem_buf, 1) < 0)
		perr(5, "Semaphore operation error %d\n", errno);
	if (semop(Sem_id[buffer], &Sem_buf, 1) < 0)
		perr(6, "Semaphore operation error %d\n", errno);
}


/*
 * Procedure:     cleanup
 *
 * Restrictions:
 *                semctl(2): none
 *                shmctl(2): none
 *
 * Notes:
 *
 *	Clean up shared memory and semaphore resources.
 */

int
cleanup()
{
	register int cnt;

	if (Ipc) {
		for (cnt = 0; cnt < BUFCNT; cnt++) {
			(void)semctl(Sem_id[cnt], IPC_RMID, 0);
			(void)shmctl(Shm_id[cnt], 0, IPC_RMID);
		}
	}
}


/*
 * Procedure:     find_lcm
 *
 * Restrictions:  none
 *
 * Notes:
 *
 * Find the lowest common multiple of two numbers.  This is used
 * to determine the buffer size that should be malloc(3)'d such that the
 * input and output data blocks can both fit evenly into the buffer.
 */

int
find_lcm(sz1, sz2)
register int sz1, sz2;
{
	register int inc, lcm, small;

	if (sz1 < sz2) {
		lcm = inc = sz2;
		small = sz1;
	} else { /* sz1 >= sz2 */
		lcm = inc = sz1;
		small = sz2;
	}
	while (lcm % small != 0)
		lcm += inc;
	return(lcm);
}


/*
 * Procedure:     blks_per_ft
 *
 * Restrictions: none
 *
 * Notes:
 *
 * blks_per_ft:  Determine the number of blocks per foot of tape.
 * Inter-block gap (dgap) is 0.3 in.
 */

int
blks_per_ft(disc)
register double disc;
{
	register double dcnt = Blk_cnt, dBpi = Bpi, dsiz = BLKSIZ, dgap = 0.3;

	return((int)(dcnt / (((dcnt * dsiz / dBpi) + dgap) / 12.0) * disc));
}


/*
 * Procedure:     tapeck
 *
 * Restrictions:
 *                g_init:  none
 *                g_read:  none
 * Notes:
 *
 * Arbitrary block size.  Determine the number of physical blocks per
 * foot of tape, including the inter-block gap, and the possibility of a short 
 * tape.  Assume the usable portion of a tape is 85% of its length for small 
 * block sizes and 88% for large block sizes.
 */

tapeck(f_p, dir)
register struct file_info *f_p;
register int dir;
{
	register int again = 1, verify, old_style, new_style;
	char resp[16];

	errno = 0;
	if ((f_p->f_bsize = g_init(&f_p->f_dev, &f_p->f_des)) < 0)
		perr(1, "volcopy: Error %d during initialization\n", errno);
	if (!IFTAPE(f_p->f_dev_p))
		return(0);
	if (dir == OUTPUT)
		(void)printf("Please note that cartridge tapes are treated like reel to reel tapes\n");
	V_labl.v_magic[0] = '\0';	/* scribble on old data */
	alarm(5);
	if (g_read(f_p->f_dev, f_p->f_des, &V_labl, sizeof(V_labl)) <= 0) {
		if (dir == INPUT)
			perror("input tape");
		else
			perror("output tape");
	}
	alarm(0);
	if (V_labl.v_reel == (char)NULL && dir == INPUT)
		perr(9, "Input tape is empty\n");
	else {
		old_style = !strncmp(V_labl.v_magic, "Volcopy", 7);
		new_style = !strncmp(V_labl.v_magic, "VOLCOPY", 7);
		if (!old_style && !new_style) {
			verify = (dir == INPUT) ? 0 : 1;
			prompt(verify, "Not a labeled tape\n");
			if (dir == INPUT)
				perr(10, "Input tape not made by volcopy\n");
			mklabel();
			strncpy(V_labl.v_volume, f_p->f_vol_p, 6);
			Osup.fs_time = 0;
		} else if (new_style) {
			Eomflg = (dir == INPUT) ? 1 : Eomflg;
			if (!Eomflg)
				strncpy(V_labl.v_magic, "Volcopy", 7);
		}
	}
	if (*f_p->f_vol_p == '-') 
		strncpy(f_p->f_vol_p, V_labl.v_volume, 6);
	else if (NOT_EQ(V_labl.v_volume, f_p->f_vol_p, 6)) {
		prompt(1, "Header volume(%.6s) does not match (%s)\n",
			V_labl.v_volume, f_p->f_vol_p);
		strncpy(V_labl.v_volume, f_p->f_vol_p, 6);
	}
	if (dir == INPUT) {
		Bpi = V_labl.v_dens;
		if (!Eomflg) {
			R_len = V_labl.v_length;
			R_num = V_labl.v_reels;
		}
	}
	while (!Eomflg && (R_len <= 0 || R_len > 3600)) {
		(void)printf("Enter size of reel in feet for <%s> (default 600):   ", f_p->f_vol_p);
		fgets(resp, 10, Devtty);
		if (resp[0] == '\n')
			R_len = 600;
		else
			R_len = atoi(resp);
		if (R_len > 0 && R_len <= 3600)
			break;
		perr(0, "Size of reel must be > 0, <= 3600\n");
	}
	while (!Eomflg && again) {
		again = 0;
		if (!Bpi) {
			(void)printf("Tape density? (i.e., 800 | 1600 | 6250(default))   ");
			fgets(resp, 10, Devtty);
			if (resp[0] == '\n')
				Bpi = 6250;
			else
				Bpi = atoi(resp);
		}
		switch (Bpi) {
		case 800:
			R_blks = Ft800x10 * R_len;
			break;
		case 1600:
			R_blks = Ft1600x10 * R_len;
			break;
		case 6250:
			R_blks = Ft6250x50 * R_len;
			break;
		default:
			perr(0, "Bpi must be 800, 1600, or 6250\n");
			Bpi = 0;
			again = 1;
		} /* Bpi */
	} /* again */
	(void)printf("\nReel %.6s", V_labl.v_volume);
	if (!Eomflg) {
		V_labl.v_length = R_len;
		V_labl.v_dens = Bpi;
		(void)printf(", %d feet, %d BPI\n", R_len, Bpi);
	} else
		(void)printf(", ? feet\n");
	return(1);
}


/*
 * Procedure:     hdrck
 *
 * Restrictions:
 *                g_read:  none
 *
 * Notes:
 *
 *	Look for and validate a volcopy style tape label.
 */

hdrck(dev, fd, tvol)
register int dev, fd;
register char *tvol;
{
	register int verify;
	struct volcopy_label tlabl;

	alarm(15); /* don't scan whole tape for label */
	errno = 0;
	if (g_read(dev, fd, &tlabl, sizeof(tlabl)) != sizeof(tlabl)) {
		alarm(0);
		verify = Otape;
		prompt(verify, "Cannot read header\n");
		if (Itape)
			close(fd);
		else
			strncpy(V_labl.v_volume, tvol, 6);
		return(verify);
	}
	alarm(0);
	V_labl.v_reel = tlabl.v_reel;
	if (NOT_EQ(tlabl.v_volume, tvol, 6)) {
		perr(0, "Volume is <%.6s>, not <%s>.\n", tlabl.v_volume, tvol);
		if (ask("Want to override?   ")) {
			if (Otape)
				strncpy(V_labl.v_volume, tvol, 6);
			else
				strncpy(tvol, tlabl.v_volume, 6);
			return(1);
		}
		return(0);
	}
	return(1);
}


/*
 * Procedure:     mklabel
 *
 * Restrictions:  none
 *
 * Notes:
 *
 * 	Zero out and initialize a volcopy label.
 */

mklabel()
{

	(void)memcpy(&V_labl, Empty, sizeof(V_labl));
	if (!Eomflg)
		(void)strcpy(V_labl.v_magic, "Volcopy");
	else
		(void)strcpy(V_labl.v_magic, "VOLCOPY");
}


/*
 * Procedure:     rprt
 *
 * Restrictions:
 *                 printf: none
 * Notes:
 *
 *	Report activity to user.
 */

rprt()
{

	if (Itape)
		(void)printf("\nReading ");
	else /* Otape */
		(void)printf("\nWriting ");
	if (!Eomflg)
		(void)printf("REEL %d of %d VOL = %.6s\n", R_cur, R_num, In.f_vol_p);
	else
		(void)printf("REEL %d of ? VOL = %.6s\n", R_cur, In.f_vol_p);
}

/*
 * Procedure:     fslog
 *
 * Restrictions:
 *                access(2): P_MACREAD;P_MACWRITE;
 *                sprintf: none
 *                ctime: P_MACREAD
 *
 * Notes:
 *
 *	Log current activity.
 */

fslog()
{
	char cmd[500];

	procprivl(CLRPRV,MACREAD_W,0);
	if (access("/var/adm/log/filesave.log", 6) == -1)
		perr(1, "volcopy: cannot access /var/adm/log/filesave.log\n");
	procprivl(SETPRV,MACREAD_W,0);

	system("tail -200 /var/adm/log/filesave.log >/tmp/FSJUNK");
	system("cp /tmp/FSJUNK /var/adm/log/filesave.log");
	sprintf(cmd,"echo \"%s;%.6s;%.6s -> %s;%.6s;%.6s on %.24s\" >>/var/adm/log/filesave.log",
		In.f_dev_p, Ifname, Ifpack, 
		Out.f_dev_p, Ofname, Ofpack, ctime(&Tvec));
	system(cmd);
	system("rm /tmp/FSJUNK");
	exit(0);
}


/*
 * Procedure:     getname
 *
 * Restrictions:
 *                printf: none
 *                fgets: none
 *
 * Notes:
 *
 *	Get device name.
 */

getname(nam_p)
register char *nam_p;
{
	register int lastchar;
	char nam_buf[21];

	nam_buf[0] = '\0';
	(void)printf("Changing drives? (type RETURN for no,\n");
	(void)printf("\t`/dev/rmt/??\' or `/dev/rtp/??\' for yes: ");
	fgets(nam_buf, 20, Devtty);
	nam_buf[20] = '\0';
	lastchar = strlen(nam_buf) - 1;
	if (nam_buf[lastchar] == '\n')
		nam_buf[lastchar] = '\0'; /* remove it */
	if (nam_buf[0] != '\0')
		(void)strcpy(nam_p, nam_buf);
}


/*
 * Procedure:     chgreel
 *
 * Restrictions:
 *                printf: none
 *                fgets: none
 *                open(2): none
 *                perror: none
 *                g_init: none
 *                g_write: none
 * Notes:
 *
 *	Change reel on end-of-media.
 */

chgreel(f_p, dir)
register struct file_info *f_p;
register int dir;
{
	register int again = 1, lastchar, temp;
	char vol_tmp[11];

	R_cur++;
	while (again) {
		again = 0;
		errno = 0;
		(void)close(f_p->f_des);
		(void)getname(f_p->f_dev_p);
		(void)printf("Mount tape %d\nType volume-ID when ready:   ", R_cur);
		vol_tmp[0] = '\0';
		fgets(vol_tmp, 10, Devtty);
		vol_tmp[10] = '\0';
		lastchar = strlen(vol_tmp) - 1;
		if (vol_tmp[lastchar] == '\n')
			vol_tmp[lastchar] = '\0'; /* remove it */
		if (vol_tmp[0] != '\0') { /* if null string, use old vol-id */
			strncpy(f_p->f_vol_p, vol_tmp, 6);
			strncpy(V_labl.v_volume, vol_tmp, 6);
		}
		errno = 0;
		f_p->f_des = open(f_p->f_dev_p, 0);
		if (f_p->f_des <= 0 || f_p->f_des > 10) {
			if (dir == INPUT)
				perror("input ERR");
			else
				perror("output ERR");
		}
		errno = 0;
		if (g_init(&(f_p->f_dev), &(f_p->f_des)) < 0)
			perr(0, "Initialization error %d\n", errno);
		if (!IFTAPE(f_p->f_dev_p)) {
			(void)printf("\n'%s' is not a valid device", f_p->f_dev_p);
			(void)printf("\n\tenter device name `/dev/rmt/??\' or `/dev/rtp/??\' :");
			again = 1;
			continue;
		}
		if (!hdrck(f_p->f_dev, f_p->f_des, f_p->f_vol_p)) {
			again = 1;
			continue;
		}
		switch (dir) {
		case INPUT:
			if (V_labl.v_reel != R_cur) {
				perr(0, "Need reel %d, label says reel %d\n", R_cur, V_labl.v_reel);
				again = 1;
				continue;
			}
			break;
		case OUTPUT:
			V_labl.v_reel = R_cur;
			temp = V_labl.v_offset;
			V_labl.v_offset /= BUFSIZ;
			close(f_p->f_des);
			sleep(2);
			errno = 0;
			f_p->f_des = open(f_p->f_dev_p, 1);
			if (f_p->f_des <= 0 || f_p->f_des > 10)
				perror("output ERR");
			errno = 0;
			if (g_init(&(f_p->f_dev), &(f_p->f_des)) < 0)
				perr(1, "Initialization error %d\n", errno);
			errno = 0;
			if (g_write(f_p->f_dev, f_p->f_des, &V_labl, sizeof(V_labl)) < 0) {
 				perr(0, "Cannot re-write header -Try again!\n");
				again = 1;
				V_labl.v_offset = temp;
				continue;
			}
			V_labl.v_offset = temp;
			break;
		default:
			perr(1, "Impossible case\n");
		} /* dir */
	} /* again */
	rprt();
}


/*
 * Procedure:     perr
 *
 * Restrictions:
 *                fflush: none
 *                vfprintf: none
 *                fprintf: none
 * Notes:
 *
 *	Print error messages.
 */

int
perr(va_alist)
va_dcl
{
	register char *fmt_p;
	register int severity;
	va_list v_Args;

	va_start(v_Args);
	severity = va_arg(v_Args, int);
	fmt_p = va_arg(v_Args, char *);
	(void)fflush(stdout);
	(void)fflush(stderr);
	if (severity == 10) {
		(void)vfprintf(stderr, fmt_p, v_Args);
		(void)fprintf(stderr, "\t%d reel(s) completed\n", --R_cur);
		(void)fflush(stdout);
		(void)fflush(stderr);
		cleanup();
		exit(31+9);
	}
	(void)vfprintf(stderr, fmt_p, v_Args);
	(void)fflush(stderr);
	va_end(v_Args);
	if (severity > 0) {
		(void)cleanup();
		exit(31+severity);
	}
}



/*
 * Procedure:     getinfs
 *
 * Restrictions:
 *                g_read: none
 */

static void 
getinfs(dev, fd, buf)
	int 	dev;
	int 	fd;
	char 	*buf;
{

	if (g_read(dev, fd, (char *)buf, BBSIZE) != BBSIZE) {
		perr(10, "Unable to read on input\n");
	}
	if (g_read(dev, fd, (char *) buf, SBSIZE)!= SBSIZE ) {
			perr(10, "Unable to read on input\n");
	}
}


/*
 * Procedure:     getoutfs
 *
 * Restrictions:
 *                g_read: none
 */

static void 
getoutfs(dev, fd, buf, verify)
	int 	dev;
	int 	fd;
	char 	*buf;
	int	verify;
{
	int cnt;
	int i;

	errno = 0;
	if (g_read(dev, fd, (char *)buf, BBSIZE)  != BBSIZE)
		perr(10, "Unable to read BB on output\n");
	cnt = SBSIZE/DEV_BSIZE;
	for (i=0;i<cnt;i++) {
		if (g_read(dev, fd, (char *) buf + i*DEV_BSIZE, DEV_BSIZE) 
			!= DEV_BSIZE )
			perr(10, "Unable to read on output\n");
	}
}


/*
 * Procedure:     getfslabel
 *
 * Restrictions:  none
 */

static char *
getfslabel(sb)
	struct fs *sb;
{
	int i;
	int blk;
	/*
	 * calculate the available blocks for each rotational position
	 */
	blk = sb->fs_spc * sb->fs_cpc / sb->fs_nspf;
	for (i = 0; i < blk; i += sb->fs_frag)
		/* void */;
	i -= sb->fs_frag;
	blk = i / sb->fs_frag;

	return((char *) &(sb->fs_rotbl[blk]));
}


/*
 * Procedure:     getvolabel
 *
 * Restrictions:  none
 */

static char *
getvolabel(sb)
	struct fs *sb;
{
	char *p;
	p = getfslabel(sb);
	return(p+6);
}


/*
 * Procedure:     fill_sec_hdr
 *
 * Restrictions:
 *                stat(2): P_MACREAD
 *
 *
 * Notes:
 *
 * Fills the security header structure with information about the 
 * running system: system name, size and creation/modification dates
 * of level translation database file.
 *
 * If this is a disk-to-tape copy, the header will be written to the tape.
 * If it is a tape-to-disk copy, this information will be compared with
 * the security header read from the tape.
 *
*/

void
fill_sec_hdr(shp)
struct sec_hdr *shp; /* pointer to security header structure */
{
        char  *ltdbname   =  "/etc/security/mac/lid.internal";
	struct utsname sysinfo;
	struct stat ltdbinfo;

        shp->sh_fs_magic = SFS_MAGIC; /* sfs file system magic number */

	if (uname(&sysinfo)<0)
		perr(1, "Unable to determine system name\n");

#if (defined (_POSIX_SOURCE) || defined(_XOPEN_SOURCE)) && !defined(_KERNEL) &&!defined(_STYPES)
	strncpy (shp->sh_host, sysinfo.nodename, _SYS_NMLN);
#else
	strncpy (shp->sh_host, sysinfo.nodename, SYS_NMLN);
#endif
        if (macpkg)
	  {
	   procprivl(CLRPRV,MACREAD_W,0);
	   if (stat( ltdbname,&ltdbinfo) < 0)
	      perr(1, "Unable to access level data base\n");
	   procprivl(SETPRV,MACREAD_W,0);

	   shp->sh_db_cdate =ltdbinfo.st_ctime;
           shp->sh_db_mdate =ltdbinfo.st_mtime;
           shp->sh_db_size= ltdbinfo.st_size;
	 }
         else
          {
            shp->sh_db_size  = -1;
            shp->sh_db_cdate  = -1;
            shp->sh_db_mdate  = -1;
	  }
}


/*
 * Procedure:     chk_sec_hdr
 *
 * Restrictions:
 *                printf:  none
 *
 * Notes:
 *
 * Check archive security header against system information.
 *
 * If archive is not of an sfs file system, abort volcopy.
 *
 * Warn if name of backed up system does not equal that of 
 * current system, or if level translation data base differs in
 * size, creation time, or last modification time.
 *
*/

void
chk_sec_hdr(in_shp,out_shp)
struct sec_hdr *in_shp; /* pointer to security header read from tape */
struct sec_hdr *out_shp; /* ptr to struct with system security info  */
{
  int tmp, same_host = 1;

  tmp = Yesflg;
  Yesflg = 0;
  if(in_shp->sh_fs_magic !=  SFS_MAGIC)
    perr(10,"Archive is not from sfs file system.");
  if(strcmp(in_shp->sh_host, out_shp->sh_host))
    {
    (void)printf("UX:volcopy:WARN:system name %s",in_shp->sh_host);
    (void)printf(" of creating system does not match system name");
    (void)printf(" %s of destination system\n",out_shp->sh_host);
    if (!ask("Continue? (y or n) "))
      perr(10,"volcopy:STOP");
    else
      same_host = 0;
    }      

  if(macpkg)
    {
    if (in_shp->sh_db_size == -1)
      {
      (void)printf("UX:volcopy:WARN:");
      (void)printf("Archive was created without Mandatory Access Controls.\n");
      if (!ask("Continue? (y or n) "))
         perr(10,"volcopy:STOP");
     }
    else if (same_host && ((in_shp->sh_db_cdate != out_shp->sh_db_cdate) ||
			   (in_shp->sh_db_mdate != out_shp->sh_db_mdate) ||
			   (in_shp->sh_db_size != out_shp->sh_db_size)))
      {
      (void)printf("UX:volcopy:WARN:");
      (void)printf("ltdb has been modified since archive was created.  \n");
      if (!ask("Continue? (y or n) "))
         perr(10,"volcopy:STOP");
      }
    }
  else /* MAC not installed */
    if (in_shp->sh_db_size != -1)
       {
       (void)printf("UX:volcopy:WARN:");
       (void)printf("System is single level, archive is multilevel.");
       if (!ask ("Continue? (y or n) "))
       perr(10,"volcopy:STOP");
       }
  Yesflg = tmp;
}



/*
 * Procedure:     dev_range_chk
 *
 * Restrictions:
 *                fdevstat: none
 *
 * Notes:
 *
 * Compare level ranges of two devices.
 * Issue warning if the two don't match.
 * If user does not elect to override warning, exit.
 *
 */

void
dev_range_chk(indevice,outdevice)
int indevice;
int outdevice;
{
 struct devstat indev;
 struct devstat outdev;
 int tmp;
 if (macpkg) {
    (void)fdevstat (indevice,DEV_GET,&indev);
    (void)fdevstat (outdevice,DEV_GET,&outdev);
    if ((!(lvlequal(&indev.dev_hilevel,&outdev.dev_hilevel)))
	||(!(lvlequal(&indev.dev_lolevel,&outdev.dev_lolevel)))) {
	char	*in_lolvl = NULL, 
		*in_hilvl = NULL,
		*out_lolvl = NULL,
		*out_hilvl = NULL;
	int	in_losz, in_hisz, out_losz, out_hisz;
	int	errflg = 0;

	in_losz = lvlout(&indev.dev_lolevel, NULL, 0, LVL_ALIAS);
	in_hisz = lvlout(&indev.dev_hilevel, NULL, 0, LVL_ALIAS);
	out_losz = lvlout(&outdev.dev_lolevel, NULL, 0, LVL_ALIAS);
	out_hisz = lvlout(&outdev.dev_hilevel, NULL, 0, LVL_ALIAS);
	if (in_losz == -1 || in_hisz == -1 ||
		out_losz == -1 || out_hisz == -1) {
		errflg = 1;
	} else {
		in_lolvl = (char *)(malloc(in_losz));
		in_hilvl = (char *)(malloc(in_hisz));
		out_lolvl = (char *)(malloc(out_losz));
		out_hilvl = (char *)(malloc(out_hisz));
	}
	if (in_lolvl == NULL || in_hilvl == NULL ||
		out_lolvl == NULL || out_hilvl == NULL) {
		errflg = 1;
	} else if ((lvlout(&indev.dev_lolevel, in_lolvl, in_losz,
			LVL_ALIAS) == -1) || 
		(lvlout(&indev.dev_hilevel, in_hilvl, in_hisz,
			LVL_ALIAS) == -1) ||
		(lvlout(&outdev.dev_lolevel, out_lolvl, out_losz,
			LVL_ALIAS) == -1) ||
		(lvlout(&outdev.dev_hilevel, out_hilvl, out_hisz,
			LVL_ALIAS) == -1)) {
		errflg = 1;
		free((void *)in_lolvl);
		free((void *)in_hilvl);
		free((void *)out_lolvl);
		free((void *)out_hilvl);
	}

	(void)printf("UX:volcopy:WARN:");
	if (errflg) {
		(void)printf("level range %x-%x",indev.dev_lolevel,indev.dev_hilevel);
		(void)printf(" of source device does not match\nlevel range");
		(void)printf(" %x-%x ",outdev.dev_lolevel,outdev.dev_hilevel);
	} else {
		(void)printf("level range %s-%s", in_lolvl, in_hilvl);
		(void)printf(" of source device does not match\nlevel range");
		(void)printf(" %s-%s ", out_lolvl, out_hilvl);
		free((void *)in_lolvl);
		free((void *)in_hilvl);
		free((void *)out_lolvl);
		free((void *)out_hilvl);
	}
       (void)printf("of output device, or level ranges are invalid\n");
       tmp = Yesflg;
       Yesflg = 0;
       if (!ask("Continue? (y or n) "))
          perr(10,"volcopy:STOP");
       Yesflg = tmp;
    }
 }
}


equal(s1, s2, ct)
char *s1, *s2;
int ct;
{
	register i;

	for(i=0; i<ct; ++i) {
		if(*s1 == *s2) {;
			if(*s1 == '\0') return(1);
			s1++; s2++;
			continue;
		} else return(0);
	}
	return(1);
}
