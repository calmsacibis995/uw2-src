/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ktool:common/ktool/idtools/idinstall.c	1.58"
#ident	"$Header: $"

/*
 *       The idinstall command is called by a driver software package's
 *       (DSP) Install script and its function is to install, remove
 *       or update a DSP.  The command syntax is as follows:
 *
 *	 idinstall -[adugGM] -[eNkz] -[bmsoptnirhATFIO] [-R dir] [-f file] [-P pkg_name] dev_name
 *		       |	            |	         |                  		|
 *		    action           	   DSP        rootdir    		internal device name
 *				        component(*)
 *
 *	       -a  Add the DSP components
 *	       -d  Remove the DSP components
 *	       -u  Update the DSP components
 *	       -M  Add/Update components as needed, based on modification times
 *	       -G  Get the DSP components in current format
 *		   (on stdout; -o and -b disallowed)
 *	       -g  Get the DSP components in original format 
 *		   (on stdout; -o and -b disallowed)
 *
 *	       -e  Disable free space checking (default on -g, -M, -d)
 *	       -N  New driver; inhibit entering of Sdevice information into
 *	           the Resource Manager database
 *
 *	       -k  Do not remove component from current directory on -a & -u
 *	       -f  Use the specified file for the reserved major number list
 *	       -P  Update the "contents" file with the package name specified.
 *	       -z  Don't assign major numbers; leave as is.
 *
 *	       -m Master component
 *	       -s System component
 *	       -o Driver*.o component
 *	       -p Space.c component
 *	       -t Stubs.c component
 *	       -b Modstub.o component
 *	       -n Node (special	file) component
 *	       -i Inittab component (Init)
 *	       -r Device Initialization	component (Rc)
 *	       -h Device Shutdown component (Sd)
 *	       -A Sassign component
 *	       -D Drvmap component
 *	       -T Mtune component
 *	       -C Autotune component
 *	       -F Ftab component
 *	       -O firmware.o component
 *
 *             -R directory: use this directory instead of /etc/conf
 *
 *
 *	       (*) If no component is specified	the default is all.
 *
 * exit 0 - success
 *	1 - error
 */

#include "inst.h"
#include "defines.h"
#include "devconf.h"
#include "mdep.h"
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h>
#include <locale.h>
#include <pfmt.h>

#ifndef CROSS
#include <ftw.h>
int rm_entry();
#endif

/* components */
#define	MASTER		0x1
#define	SYSTEM  	0x2
#define NNODE		0x4
#define INITF		0x8
#define	RCFILE		0x10
#define	SHUTDOWN	0x20
#define	DRIVER		0x100
#define	SPACE		0x200
#define	STUBS		0x400
#define SASSIGN		0x800
#define MTUNE		0x1000
#define MODSTUB		0x2000
#define FTABF		0x4000
#define	ATUNE		0x8000
#define	DRVMAPF		0x10000
#define	FWARE		0x20000		/* firmware.o */
#define	DTUNE		0x40000
#define	BCFG		0x80000
#define	ALL		0xfffff


/* copy type flags */
#define CP_BIN		0x01	/* binary, not text */
#define CP_MASTER	0x02	/* Master file, may need to be converted */
#define CP_SYSTEM	0x04	/* System file, may need to be converted */

/* error messages */
#define USAGE		":2:Usage:  idinstall -[adugGM] [-eNk] [-optbmsnirhATFIO] [-R rootdir] [-f maj_list] [-P pkg_name] module_name\n"
#define NOLINK		":17:Cannot link <%s> to <%s>: '%s'\n"
#define TOOLONG		":18:Line too long in %s file\n"
#define LINENONL	":276:Last line does not end in newline in %s file\n"

#define DIR_MODE	0755	/* File mode for directories */
#define FILE_MODE	0644	/* File mode for files */

#define	CONF		"/etc/conf"
static char *confdirs[] = {
	"mdevice.d",
	"sdevice.d",
	"node.d",
	"init.d",
	"rc.d",
	"sd.d",
	"sassign.d",
	"mtune.d",
	"mod.d",
	"autotune.d",
	"ftab.d",
	"drvmap.d"
};

#define TRUE	1
#define FALSE	0

char fbuf[512];

char device[15];	/* device name */
char ppath[PATH_MAX];	/* relative device driver package directory */
char fullpath[PATH_MAX]; /* complete path to device driver package directory */
char root[PATH_MAX] = CONF; /* root directory containing cf.d and packages */
char resfile[PATH_MAX];	/* file for reserved major numbers list */
char linebuf[LINESZ];
char pkginst[41];     /* name of the package which includes this module */
extern char instroot[];
extern char path[];

int debug=0;		/* debug flag */
int madedir=0;		/* Flag to remove partial install on error */

char actflag=0;	/* must have one of a, d, u, g, G, M */
char aflag=0;	/* -a flag specified, Add the component(s) */
char dflag=0;	/* -d flag specified, Delete the component(s) */
char uflag=0;	/* -u flag specified, Update the component(s) */
char Gflag=0;	/* -G flag specified, Get (to stdout) current component(s) */
char gflag=0;	/* -g flag specified, Get (to stdout) original component(s) */
char Mflag=0;	/* -M flag specified, Add/Update if out-of-date */

char eflag=0;	/* -e flag specified, disable free-space check */
char Nflag=0;	/* -N flag specified, do not call idresadd */
char kflag=0;	/* -k flag specified, do not remove local file */
char Rflag=0;	/* -R flag specified, use argument as ID root directory */
char fflag=0;	/* -f flag specified, use argument as res_major file */
char Pflag=0;	/* -P flag specified, add all the new files to contents file */
char zflag=0;	/* -z flag specified, don't assign major numbers */

long partflag=0;		/* component flag; none means all */

char old_master_cpu[32];	/* When converting old format Master & System
				   files, save the cpu field from the Master
				   file to add to the System file. */
char old_master_dma[32];	/* When converting old format Master & System
				   files, save the dma field from the Master
				   file to add to the System file. */
int need_dmachan;		/* True iff DMA channel needed to convert
				   version 0 System file. */
int set_master_loadable;	/* When converting version 1 $loadable to
				   version 2. */
int hwmod;			/* When converting to version 2, true iff
				   should be a hardware module. */
int saw_hardmod = 0;		/* HARDMOD was seen in Master file */
int redo_system;		/* System file needs to be reprocessed. */

struct devlist {
	short	start[2];
	short	end[2];
	struct devlist *link[2];
	struct mdev mdev;
	int	reserved;
	int	used;
} *devlist[2];
#define BDEVL	0
#define CDEVL	1

extern void exit();
extern char *optarg;
extern int optind;
extern char *sys_errlist[];

extern struct intfc *interfaces;

char contentsf[L_tmpnam];     /* The tmp file used to update contents file. */
FILE *cfp;                    /* File descriptor for the tmp file. */
int cfopen = 0;               /* flag to remove temp file when error occur. */

struct devlist *enter_dev();
void idcat(), idrm();
void nmkdir();

#define RECOVERY_CODE	0	/* The recovery code is broken in a large
				   number of ways, so for now, disable it. */


main(argc, argv)
int argc;
char *argv[];
{
	int	isthere, m;

	umask(022);

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxidtools");
	(void) setlabel("UX:idinstall");

	while ((m = getopt(argc, argv, "?#abdugGMekzmsnirhANTCFDoptR:f:P:O")) != EOF)
		switch (m) {
		case 'a':
			aflag++; 
			actflag++;
			break;
		case 'd':
			dflag++; 
			actflag++;
			break;
		case 'u':
			uflag++; 
			actflag++;
			break;
		case 'g':
			gflag++; 
			actflag++;
			break;
		case 'G':
			Gflag++; 
			actflag++;
			break;
		case 'M':
			Mflag++; 
			actflag++;
			break;

		case 'N':
			Nflag++;
			break;

		case 'e':
			eflag++; 
			break;

		case 'k':
			kflag++; 
			break;

		case 'z':
			zflag++; 
			break;

		case 'R':
			Rflag++;
			strcpy(root, optarg);
			break;
			
		case 'm':
			partflag |= MASTER;
			break;
		case 's':
			partflag |= SYSTEM;
			break;
		case 'o':
			partflag |= DRIVER;
			break;
		case 'p':
			partflag |= SPACE;
			break;
		case 't':
			partflag |= STUBS;
			break;
		case 'b':
			partflag |= MODSTUB;
			break;
		case 'n':
			partflag |= NNODE;
			break;
		case 'i':
			partflag |= INITF;
			break;
		case 'r':
			partflag |= RCFILE;
			break;
		case 'h':
			partflag |= SHUTDOWN;
			break;
		case 'A':
			partflag |= SASSIGN;
			break;
		case 'T':
			partflag |= MTUNE;
			break;
		case 'C':
			partflag |= ATUNE;
			break;
		case 'F':
			partflag |= FTABF;
			break;
		case 'D':
			partflag |= DRVMAPF;
			break;
		case 'O':
			partflag |= FWARE;
			break;
		case 'f':
			fflag++;
			strcpy(resfile, optarg);
			break;
		case 'P':
			Pflag++;
			strcpy(pkginst, optarg);
			tmpnam(contentsf);

			if ((cfp = fopen(contentsf, "w")) == NULL) {
				pfmt(stderr, MM_ERROR, ":1:Cannot open file %s for writing.\n", contentsf);
				exit(1);
			}
			cfopen++;
			break;
		case '#':
			debug++;
			break;
		case '?':
			pfmt(stderr, MM_ACTION, USAGE);
			exit(1);
		}

	if (actflag != 1) {
		pfmt(stderr, MM_ACTION, ":3:Must have exactly one of -a, -d, -u, -g, -G, -M options.\n");
		pfmt(stderr, MM_ACTION, USAGE);
		error_exit();
	}
	if (argc - optind != 1) {
		pfmt(stderr, MM_ERROR, ":4:Must specify exactly one device name.\n");
		pfmt(stderr, MM_ACTION, USAGE);
		error_exit();
	}
	strcpy(device, argv[optind]);
	if (device[0] == '\0') {
		pfmt(stderr, MM_ERROR, ":5:Device name must be nonblank.\n");
		pfmt(stderr, MM_ACTION, USAGE);
		error_exit();
	}

	if (!fflag)
		sprintf(resfile, "%s/%s/res_major", root, CFDIR);

	/* Tell getinst() where files reside */
	strcpy(instroot, root);
	/* (Don't need to set pathinst[] since we only use MDEV_D.) */

	if (!(aflag || uflag))
		eflag++;
	if (!(aflag || uflag))
		kflag++;

	if (!eflag) {
		if (access("/etc/conf/bin/idspace", X_OK) == 0 &&
		    system("/etc/conf/bin/idspace") != 0) {
			pfmt(stderr, MM_ERROR, ":6:Insufficient disk space to reconfigure.\n");
                        error_exit();
		}
	}

	if (partflag == 0)
		partflag = ALL;

	strcpy(ppath, "/pack.d/");	/* dir. for package (rel. to root) */
	strcat(ppath, device);
	strcpy(fullpath, root);
	strcat(fullpath, ppath);

	if(debug){
		fprintf(stderr, "ppath = %s, fullpath = %s\n", ppath, fullpath);
		fprintf(stderr, "parts= %x, act= %x, dev= %s\n",
				partflag, actflag, device);
	}

	if (access(fullpath, F_OK) < 0) {
		if (errno != ENOENT) {
			pfmt(stderr, MM_ERROR,
			     ":8:Cannot find device driver directory.\n");
			error_exit();
		}
		isthere = FALSE;
	} else
		isthere = TRUE;

	if (aflag || Mflag) {
		if (partflag & (DRIVER|SPACE|STUBS|MODSTUB)) {
			if (!isthere) {
				strcpy(fbuf, root);
				strcat(fbuf, "/pack.d");
				if (access(fbuf, F_OK) < 0)
					nmkdir(fbuf);
				nmkdir(fullpath);
				madedir++;
#if RECOVERY_CODE
				if (aflag)
					mksave();
#endif
			} else if (aflag) {
				pfmt(stderr, MM_ERROR, ":9:Device package already exists.\n");
                                error_exit();
			}
		}
	}
	if ((uflag || gflag || Gflag) && !isthere) {
		pfmt(stderr, MM_ERROR, ":10:Cannot open driver package directory.\n");
		error_exit();
	}
#if RECOVERY_CODE
	if (uflag || Mflag)
		mksave();
#endif

	if (aflag || Mflag || uflag) {
		if (aflag && partflag == ALL) {
			DIR *dp = NULL;
			if (!find_Driver(NULL, &dp, NULL)) {
				pfmt(stderr, MM_ERROR, ":11:Local directory does not contain a Driver object (Driver.o) file.\n");
				error_exit();
			}
			isthere = (access("Master", R_OK) == 0 ? 1 : 0);
			isthere += (access("System", R_OK) == 0 ? 2 : 0);
			if (isthere != 3) {
				pfmt(stderr, MM_ERROR, ":230:Local directory must contain readable Master and System files.\n");
				error_exit();
			}
		}
		if (partflag & SPACE) {
			if (ccopy("Space.c", ppath, "space.c", 0, 0))
				idunlink("Space.c");
		}
		if (partflag & STUBS) {
			if (ccopy("Stubs.c", ppath, "stubs.c", 0, 0))
				idunlink("Stubs.c");
		}
		if (partflag & MODSTUB) {
			if (ccopy("Modstub.o", ppath, "Modstub.o", CP_BIN, 0))
				idunlink("Modstub.o");
		}
		/*
		 * Some conversion cases require SYSTEM to be processed
		 * before MASTER and DRIVER, so always do it in that order.
		 */
		if (partflag & SYSTEM) {
			(void) ccopy("System", "/sdevice.d", device, CP_SYSTEM,
								     SDEV_VER);
		}
		if (partflag & MASTER) {
			if (ccopy("Master", "/mdevice.d", device, CP_MASTER,
								  MDEV_VER))
				idunlink("Master");
		}
		/* System must be unlinked after Master is done. */
		idunlink("System");
		if (partflag & DRIVER) {
			DIR *dp = NULL;
			char *filename;
			while (find_Driver(NULL, &dp, &filename)) {
				if (ccopy(filename, ppath, filename, CP_BIN, 0))
					idunlink(filename);
			}
		}
		if (partflag & NNODE) {
			if (ccopy("Node", "/node.d", device, 0, NODE_VER))
				idunlink("Node");
		}
		if (partflag & INITF) {
			if (ccopy("Init", "/init.d", device, 0, 0))
				idunlink("Init");
		}
		if (partflag & RCFILE) {
			if (ccopy("Rc", "/rc.d", device, 0, 0))
				idunlink("Rc");
		}
		if (partflag & SHUTDOWN) {
			if (ccopy("Sd", "/sd.d", device, 0, 0))
				idunlink("Sd");
		}
		if (partflag & SASSIGN) {
			if (ccopy("Sassign", "/sassign.d", device, 0,
								   SASSIGN_VER))
				idunlink("Sassign");
		}
		if (partflag & MTUNE) {
			if (ccopy("Mtune", "/mtune.d", device, 0, MTUNE_VER))
				idunlink("Mtune");
		}

		if (partflag & ATUNE) {
			if (ccopy("Autotune", "/autotune.d", device, 0, ATUNE_VER))
				idunlink("Autotune");
		}
		if (partflag & FTABF) {
			if (ccopy("Ftab", "/ftab.d", device, 0, FTAB_VER))
				idunlink("Ftab");
		}
		if (partflag & DRVMAPF) {
			if (ccopy("Drvmap", "/drvmap.d", device, 0, DRVMAP_VER))
				idunlink("Drvmap");
		}
		if (partflag & DTUNE) {
			if (ccopy("Dtune", "/dtune.d", device, 0, 0))
				idunlink("Dtune");
		}
		if (partflag & FWARE) {
			if (ccopy("firmware.o", ppath, "firmware.o", CP_BIN, 0))
				idunlink("firmware.o");
		}

		if (partflag & BCFG)
			install_bcfg();

/*
 *  Notify /dev/resmgr of new System file if necessary
 */

		if (!Mflag && !Nflag && !Rflag && (partflag & SYSTEM) &&
		    saw_hardmod) {
			char cmd[PATH_MAX];

			sprintf(cmd, "/etc/conf/bin/idresadd -r %s %s",
						root, device);
			system(cmd);
		}

	}
	if (dflag) {
		if (partflag == ALL){
#if RECOVERY_CODE
			mksave();
#endif

			if (!Nflag && !Rflag) {
				char cmd[PATH_MAX];

				sprintf(cmd,
					"/etc/conf/bin/idresadd -d -r %s %s",
							root, device);
				system(cmd);
			}

			delete_tunables();
			rmpack(1,1);	/* Save and be quiet on err */
		} else {
			if (partflag & DRIVER) {
				DIR *dp = NULL;
				char *filename;
				while (find_Driver("pack.d", &dp, &filename))
					idrm("pack.d", filename, 0);
			}
			if (partflag & SPACE)
				idrm("pack.d", "space.c", 0);
			if (partflag & STUBS)
				idrm("pack.d", "stubs.c", 0);
			if (partflag & MODSTUB)
				idrm("pack.d", "Modstub.o", 0);
			if (partflag & MASTER)
				idrm("mdevice.d", NULL, 0);
			if (partflag & SYSTEM) {
			    if (!Nflag && !Rflag) {
				char cmd[PATH_MAX];

				sprintf(cmd,
					"/etc/conf/bin/idresadd -d -r %s %s",
							root, device);
				system(cmd);
			    }

			    idrm("sdevice.d", NULL, 0);
			}
			if (partflag & NNODE)
				idrm("node.d", NULL, 0);
			if (partflag & INITF)
				idrm("init.d", NULL, 0);
			if (partflag & RCFILE)
				idrm("rc.d", NULL, 0);
			if (partflag & SHUTDOWN)
				idrm("sd.d", NULL, 0);
			if (partflag & SASSIGN)
				idrm("sassign.d", NULL, 0);
			if (partflag & MTUNE) {
				delete_tunables();
				idrm("mtune.d", NULL, 0);
			}
			if (partflag & ATUNE) 
				idrm("autotune.d", NULL, 0);
			if (partflag & FTABF)
				idrm("ftab.d", NULL, 0);
			if (partflag & DRVMAPF)
				idrm("drvmap.d", NULL, 0);
			if (partflag & DTUNE)
				idrm("dtune.d", NULL, 0);
			if (partflag & FWARE)
				idrm("pack.d", "firmware.o", 0);
			if (partflag & BCFG)
				rm_bcfg();
		}
	}
	if (Gflag || gflag) {
		if (partflag == ALL) {
			pfmt(stderr, MM_ERROR, ":14:Must have one of -p, -t, -m, -s, -n, -i, -r, -h, -A, -T\noptions when using -g or -G.\n");
			pfmt(stderr, MM_ACTION, USAGE);
			error_exit();
		}
		if (partflag & (DRIVER | MODSTUB)) {
			pfmt(stderr, MM_ERROR, ":15:-o and -b options not allowed with -g and -G.\n");
			pfmt(stderr, MM_ACTION, USAGE);
			error_exit();
		}
		if (partflag & SPACE)
			idcat("pack.d", "space.c");
		if (partflag & STUBS)
			idcat("pack.d", "stubs.c");
		if (partflag & MASTER)
			idcat("mdevice.d", NULL);
		if (partflag & SYSTEM)
			idcat("sdevice.d", NULL);
		if (partflag & NNODE)
			idcat("node.d", NULL);
		if (partflag & INITF)
			idcat("init.d", NULL);
		if (partflag & RCFILE)
			idcat("rc.d", NULL);
		if (partflag & SHUTDOWN)
			idcat("sd.d", NULL);
		if (partflag & SASSIGN)
			idcat("sassign.d", NULL);
		if (partflag & MTUNE)
			idcat("mtune.d", NULL);
		if (partflag & ATUNE)
			idcat("autotune.d", NULL);
		if (partflag & FTABF)
			idcat("ftab.d", NULL);
		if (partflag & DRVMAPF)
			idcat("drvmap.d", NULL);
		if (partflag & DTUNE)
			idcat("dtune.d", NULL);

		/* idcat for BCFG not provided */
	}
	if (Pflag) {
		char cmdline[LINESZ];

		fclose(cfp);
		if (debug)
			fprintf(stderr, "temp file is %s\n", contentsf);
		if (aflag || uflag || Mflag) {
			sprintf(cmdline, "installf %s - < %s",
				pkginst, contentsf);
			system(cmdline);
		}
		if (dflag) {
			sprintf(cmdline, "removef %s - < %s",
				pkginst, contentsf);
			system(cmdline);
		}
		unlink(contentsf);
	}
	exit(0);
}
	

install_bcfg()
{
	DIR *d;
	struct dirent *e;
	int l;
	char path[PATH_MAX];

	sprintf(path, "%s/bcfg.d", root);

	if (access(path, F_OK) != 0)
		nmkdir(path);

	d = opendir(".");

	if (d == NULL)
	{
		perror(".");
		return;
	}

	sprintf(path, "/bcfg.d/%s", device);

	while ((e = readdir(d)) != NULL)
	{
		l = strlen(e->d_name);

		if (l > 5 && 
		    strcmp(e->d_name + (l-5), ".bcfg") == 0)
		{
			if (ccopy(e->d_name, path, e->d_name, CP_BIN, 0))
				idunlink(e->d_name);
		}
	}

	closedir(d);
}


rm_bcfg()
{
	char cmd[PATH_MAX];

	sprintf(cmd, "rm -rf %s/bcfg.d/%s", root, device);
	system(cmd);
}


rmpack(savflg, quiet)
int savflg;
int quiet;
{
	int i;
	char rmdir[PATH_MAX];

	if (debug)
		fprintf(stderr,
			"Removing device driver directory and its contents.\n");
#ifndef CROSS
	if (Pflag)
		while (ftw(fullpath, rm_entry, 20) > 0)
			;
#endif
	sprintf(rmdir, "rm -rf %s", fullpath);
	if (system(rmdir) != 0 && !quiet) {
		pfmt(stderr, MM_ERROR, ":16:idinstall: Cannot remove driver package directory\n");
		exit(1);
	}

	for (i=0; i<(sizeof(confdirs)/sizeof(char *)); i++) {
		strcpy(rmdir, confdirs[i]);
		idrm(rmdir, NULL, savflg);
	}

	rm_bcfg();
}
	
#ifndef CROSS
/* ARGSUSED */
int
rm_entry(name, stat, flag)
char *name;
struct stat *stat;
int flag;
{
	if (debug)
		fprintf(stderr, "removing contents entry for %s\n", name);
	fprintf(cfp, "%s\n", name);
	return(0);
}
#endif


static void
idrm_sup(delfile, filname)
char *delfile;
char *filname;
{

	if (filname != NULL) {
		strcat(delfile, "/");
		strcat(delfile, filname);
	}

	if (unlink(delfile) < 0)
	{
		if (errno != ENOENT)
			pfmt(stderr, MM_WARNING, ":19:cannot remove file %s\n",
								delfile);
	}

	if (Pflag) {
		if (debug)
			fprintf(stderr, "removing contents file entry %s\n",
				delfile);
		fprintf(cfp, "%s\n", delfile);
	}
}


void
idrm(dirname,filname,savflg) 
char *dirname;
char *filname;
int savflg;
{
	char delfile[PATH_MAX];

	sprintf(delfile, "%s/%s/%s", root, dirname, device);
	idrm_sup(delfile, filname);

	sprintf(delfile, "%s/.%s/%s", root, dirname, device);
	idrm_sup(delfile, filname);
}


void
idcat(dirname, filname)
char *dirname;
char *filname;
{
	char catfile[PATH_MAX];
	char sfile[PATH_MAX];
	char cmdline[PATH_MAX + 12];

	sprintf(catfile, "%s/%s/%s", root, dirname, device);
	if (gflag) {
		sprintf(sfile, "%s/.%s/%s", root, dirname, device);
		if (access(sfile, F_OK) == 0) {
			if (debug)
				fprintf(stderr, "Use saved file: %s\n", sfile);
			strcpy(catfile, sfile);
		}
	}
		
	if (filname != NULL){
		strcat(catfile, "/");
		strcat(catfile, filname);
	}
	sprintf(cmdline, "cat -s %s", catfile);
	if (debug)
		fprintf(stderr, "%s\n", cmdline);
	if (system(cmdline) != 0) {
		pfmt(stderr, MM_ERROR, ":20:Cannot find driver component.\n");
		error_exit();
	}
}


show_line()
{

	if (show_line)
	{
		fprintf(stderr, "%s  %s", gettxt(":104", "FILE:"), path);
		fprintf(stderr, "%s  %s", gettxt(":21", "LINE:"), linebuf);
	}
}


error_exit()
{

	if (madedir) {
		rmpack(0, 1);	/* don't save, and be quiet */

#if RECOVERY_CODE
		unlink ("/etc/.last_dev_add");
#endif
	}

	if (cfopen) {
		fclose(cfp);
		unlink(contentsf);
	}
	exit(1);
}


insterror(errcode, ftype, dev)
int errcode;
int ftype;
char *dev;
{

	if (errcode != IERR_OPEN)
		pfmt(stderr, MM_ERROR, ":105:LINE: %s\n", linebuf);
	show_ierr(errcode, ftype, dev);
	exit(1);
}


int oversion;		/* original version of file before (any) conversion */
int need_oversion;	/* $oversion needs to be written */

idcp(src, destd, destf, flags, cur_ver)
char *src, *destd, *destf;
int flags;
int cur_ver;
{
	char tpath[PATH_MAX], spath[PATH_MAX];
	FILE *from, *to;
	int version = 0;
	int ver_written = 0;
	int errstat = 0;
	int n;

	oversion = 0;

	strcpy(tpath, root);
	strcat(tpath,destd);
	strcat(tpath, "/");
	strcat(tpath,destf);
#if RECOVERY_CODE
	if (uflag || Mflag) {
		strcpy(spath,"/etc/.last_dev_del");
		strcat(spath,destd);
		mkdir(spath, DIR_MODE);
		strcat(spath, "/");
		strcat(spath,destf);
		if (access(tpath, F_OK) == 0) {
			if (link(tpath,spath) < 0) {
				pfmt(stderr, MM_ERROR, NOLINK, tpath, spath,
					sys_errlist[errno]);
				error_exit();
			}
			unlink(tpath);
		}
	}
#endif /* RECOVERY_CODE */
	if (debug)
		fprintf(stderr, "copying %s\n", tpath);

	if ((from = fopen(src, "r")) == NULL) {
		pfmt(stderr, MM_ERROR, ":22:Cannot copy files - read open failed.\n");
                error_exit();
	}
	close(creat(tpath, FILE_MODE));
	if ((to = fopen(tpath, "w")) == NULL) {
		pfmt(stderr, MM_ERROR, ":23:Cannot copy files - write open failed.\n");
		error_exit();
	}

	if (flags & CP_MASTER)
		old_master_dma[0] = '\0';

	if (flags & CP_BIN) {
		while ((n = fread(linebuf, 1, sizeof linebuf, from)) > 0)
			fwrite(linebuf, 1, n, to);
	} else {
		need_oversion = 1;
		while (fgets(linebuf, sizeof linebuf, from) != NULL) {
			if (linebuf[strlen(linebuf) - 1] != '\n') {
				show_line();

				if (strlen(linebuf) < sizeof(linebuf) - 1)
					pfmt(stderr, MM_ERROR, LINENONL, src);
				else
					pfmt(stderr, MM_ERROR, TOOLONG, src);
				errstat = 1;
				break;
			}
			if (INSTRING("#*\n", linebuf[0])) {
				fputs(linebuf, to);
				continue;
			}
			if (strncmp(linebuf, "$version", 8) == 0 &&
			   (linebuf[8] == ' ' || linebuf[8] == '\t')) {
				version = atoi(linebuf + 9);
				if (version > cur_ver) {
					show_line();
					pfmt(stderr, MM_ERROR, TOONEW, src);
					errstat = 1;
					break;
				}
				oversion = version;
				if (version == cur_ver) {
					fputs(linebuf, to);
					need_oversion = 0;
				}
				continue;
			}
			if (strncmp(linebuf, "$oversion", 9) == 0 &&
			   (linebuf[9] == ' ' || linebuf[9] == '\t')) {
				oversion = atoi(linebuf + 10);
				if (version == cur_ver)
					fprintf(to, "$oversion %d\n", oversion);
				else
					need_oversion = 1;
				continue;
			}
			if (version != cur_ver) {
				if (!ver_written) {
					save_original(src, destd, destf);
					fprintf(to, "$version %d\n", cur_ver);
					ver_written = 1;
				}
				if (flags & CP_SYSTEM) {
				    if (!convert_system(linebuf, to, version))
					continue;
				} else if (flags & CP_MASTER) {
				    if (!convert_master(linebuf, to, version))
					continue;
				} else if (need_oversion) {
					fprintf(to, "$oversion %d\n", oversion);
					need_oversion = 0;
				}
			}
			if (flags & CP_MASTER) {
				if (process_master(linebuf, to))
					break;
				continue;
			}
			if (flags & CP_SYSTEM) {
				struct sdev sdev;

				if (rdinst(SDEV, linebuf, &sdev, 0, oversion) == 1) {
					if (old_master_cpu[0] != '\0') {
						sprintf(linebuf +
							 strlen(linebuf) - 1,
							" %s\n",
							old_master_cpu);
						old_master_cpu[0] = '\0';
					}
				}
			}
			fputs(linebuf, to);
		}
	}

	if (ferror(from)) {
		pfmt(stderr, MM_ERROR, ":24:Cannot copy files - read error on %s.\n", src);
		errstat = 1;
	}

	fclose(to);
	fclose(from);

	if (errstat) {
		pfmt(stderr, MM_INFO, ":25:%s removed\n", tpath);
		error_exit();
        }

	setlevel(tpath, 2);
	if (Pflag) {
		fprintf(cfp, "%s%s/%s v 0%o root sys 2 NULL NULL\n",
			root, destd, destf, FILE_MODE);
		if (debug)
			fprintf(stderr, "%s%s/%s v 0%o root sys 2 NULL NULL\n",
				root, destd, destf, FILE_MODE);
	}

/*
 *  Some old driver packages are using idinstall -u to install themselves.
 *  This is bad because if there was a newer driver with the same name
 *  previously on the system, the newer driver may have files which did
 *  not exist previously left lying around on the system interfering
 *  with the old driver, i.e. a Drvmap file.
 *
 *  To let these old broken drivers continue to work, we will remove
 *  files from the system which did not exist in the version 0-1 timeframe
 *  when idinstall -u is called to install a version 0-1 driver.
 *
 *  Drivers should not install themselves with idinstall -u.
 */

	if ((flags & CP_MASTER) && version < 2 && uflag)
	{
		char buf[PATH_MAX];

		sprintf(buf, "%s/pack.d/%s/Driver_atup.o", root, device);
		unlink(buf);
		sprintf(buf, "%s/pack.d/%s/Driver_atmp.o", root, device);
		unlink(buf);
		sprintf(buf, "%s/drvmap.d/%s", root, device);
		unlink(buf);
		sprintf(buf, "%s/dtune.d/%s", root, device);
		unlink(buf);
	}

	if ((flags & CP_MASTER) && redo_system) {
		if (need_dmachan && old_master_dma[0] == '\0') {
			pfmt(stderr, MM_ERROR, ":231:Version 0 System and Master files must be installed together\n");
			error_exit();
		}
		/* Re-do System file */
		idcp("System", "/sdevice.d", destf, CP_SYSTEM, SDEV_VER);
		redo_system = 0;
	}
}

static int nonconforming;	/* "$interface nonconforming" needed */
static int total_count;		/* sum of all interface counts */
static FILE *conv_file;		/* output file for conversion */
static unsigned column;		/* output column number */

/* interface flags */
#define DONTPICK	(1 << 0)	/* Don't pick this interface */
#define SELECTED	(1 << 1)	/* This interface has been selected */
#define PREFER		(1 << 2)	/* Prefer this interface */

static int
strncmp_ic(s1, s2, len)
char *s1, *s2;
int len;
{
	while (len-- > 0) {
		if (*s1 == '\0' || *s2 == '\0' ||
		    tolower(*s1) != tolower(*s2))
			return (*s1 - *s2);
		s1++;  s2++;
	}
	return 0;
}

int
select_singles(symname, arg)
	char	*symname;
	void	*arg;
{
	char *pfx = (char *)arg;
	struct intfc *intp, *intp2, *match;

	/*
	 * Ignore symbols which begin with the driver prefix.
	 * Really gross workaround: make the comparison case insensitive;
	 * some drivers use multiple case variants of their prefix.
	 */
	if (strncmp_ic(symname, pfx, strlen(pfx)) == 0)
		return 0;

	match = NULL;
	for (intp = interfaces; intp; intp = intp->next_intfc) {
		for (intp2 = intp; intp2; intp2 = intp2->next_ver) {
			if (intfc_getsym(intp2, symname) == NULL)
				continue;
			if (intp2->flags & SELECTED) {
				if (debug) {
					fprintf(stderr,
						"using <%s> from %s.%s\n",
						symname, intp2->name,
						intp2->version);
				}
				return 0;
			}
			if (intp2->flags & DONTPICK)
				continue;
			if (match != NULL)
				return 0;
			match = intp2;
		}
	}
	if (match) {
		match->flags |= SELECTED;
		if (debug)
			fprintf(stderr, "%s.%s selected for symbol <%s>\n",
				match->name, match->version, symname);
	} else {
		nonconforming = 1;
		if (debug) {
			fprintf(stderr,
			"<%s> not found; must use $interface nonconforming\n",
				symname);
		}
	}
	return 0;
}

/* ARGSUSED */
int
count_matches(symname, arg)
	char	*symname;
	void	*arg;
{
	struct intfc *intp, *intp2;

	/* first check to see if symbol is already covered */
	for (intp = interfaces; intp; intp = intp->next_intfc) {
		for (intp2 = intp; intp2; intp2 = intp2->next_ver) {
			if (!(intp2->flags & SELECTED))
				continue;
			if (intfc_getsym(intp2, symname) != NULL)
				return 0;
		}
	}
	for (intp = interfaces; intp; intp = intp->next_intfc) {
		for (intp2 = intp; intp2; intp2 = intp2->next_ver) {
			if (intp2->flags & DONTPICK)
				continue;
			if (intfc_getsym(intp2, symname) == NULL)
				continue;
			intp2->count++;
			total_count++;
		}
	}
	return 0;
}

int
adjust_counts(symname, arg)
	char	*symname;
	void	*arg;
{
	struct intfc *choice = (struct intfc *)arg;
	struct intfc *intp, *intp2;

	if (intfc_getsym(choice, symname) == NULL)
		return 0;

	/* first check to see if symbol is already covered */
	for (intp = interfaces; intp; intp = intp->next_intfc) {
		for (intp2 = intp; intp2; intp2 = intp2->next_ver) {
			if (!(intp2->flags & SELECTED))
				continue;
			if (intfc_getsym(intp2, symname) != NULL)
				return 0;
		}
	}

	if (debug)
		fprintf(stderr, "using <%s> from %s.%s\n",
			symname, choice->name, choice->version);

	for (intp = interfaces; intp; intp = intp->next_intfc) {
		for (intp2 = intp; intp2; intp2 = intp2->next_ver) {
			if (intp2->count == 0)
				continue;
			if (intfc_getsym(intp2, symname) == NULL)
				continue;
			intp2->count--;
			if (--total_count == 0)
				return 1;
		}
	}
	return 0;
}

void
select_one()
{
	struct intfc *intp, *intp2, *choice;

	choice = NULL;
	for (intp = interfaces; intp; intp = intp->next_intfc) {
		for (intp2 = intp; intp2; intp2 = intp2->next_ver) {
			if (intp2->count == 0)
				continue;
			if (choice == NULL || intp2->count > choice->count ||
			    (intp2->count == choice->count &&
			     (intp2->flags & PREFER)))
				choice = intp2;
			else if (choice && intp2->count == choice->count) {
				if (intfc_replaces(choice, intp2))
					choice = intp2;
			}
		}
	}

	if (debug) {
		fprintf(stderr, "%s.%s selected for maximum coverage (%d)\n",
			choice->name, choice->version, choice->count);
	}

	(void) scan_symbols(adjust_counts, (void *)choice, SS_UNDEF);
	choice->flags |= SELECTED;
	choice->flags &= ~PREFER;
}

int
show_nonconforming(symname, arg)
	char	*symname;
	void	*arg;
{
	char *pfx = (char *)arg;
	struct intfc *intp, *intp2;

	/*
	 * Ignore symbols which begin with the driver prefix.
	 * Really gross workaround: make the comparison case insensitive;
	 * some drivers use multiple case variants of their prefix.
	 */
	if (strncmp_ic(symname, pfx, strlen(pfx)) == 0)
		return 0;

	for (intp = interfaces; intp; intp = intp->next_intfc) {
		for (intp2 = intp; intp2; intp2 = intp2->next_ver) {
			if (intfc_getsym(intp2, symname) == NULL)
				continue;
			if (intp2->flags & SELECTED)
				return 0;
		}
	}

	if ((column += strlen(symname) + 1) >= 80) {
		fprintf(conv_file, "\n#  ");
		column = 3 + (strlen(symname) + 1);
	}
	fprintf(conv_file, " %s", symname);

	return 0;
}

int
convert_master(line, to_file, version)
	char	line[];
	FILE	*to_file;
	int	version;
{
	char	name[32], func[32], flg[32], pfx[32], bmaj[32], cmaj[32],
		umin[32], umax[32], dma[32], order[32], cpu[32];
	char	nflg[32], dummy[2], *op, *np;
	int	nflds;
	int	is_blk, is_chr;
	int	nintfc;
	static char dversion[8] = "";
	struct intfc *intp, *intp2;
	struct intfc *ddi_5mp, *ddi_6;
	struct entry_def *edef_extmodname;

	if (!(partflag & SYSTEM)) {
		pfmt(stderr, MM_ERROR, ":232:Can't convert old Master file w/o System file.\n");
		show_line();
		error_exit();
	}
	if (access("Driver.o", R_OK) != 0) {
		pfmt(stderr, MM_ERROR, ":233:Can't convert old Master file w/o Driver.o file.\n");
		show_line();
		error_exit();
	}

	if (debug)
		fprintf(stderr, "Converting: %s", line);

	if (version == 0) {
		nflds = sscanf(line,
		           "%32s %32s %32s %32s %32s %32s %32s %32s %32s %1s",
			       name, func, flg, pfx, bmaj, cmaj,
			       umin, umax, dma, dummy);
		if (nflds != 9) {
			pfmt(stderr, MM_ERROR, EMSG_NFLDS, "Master");
			show_line();
			error_exit();
		}
		if (strcmp(dma, "-1") != 0)
			hwmod = 1;
	} else {
		/* version == 1 */
		if (strncmp(line, "$dversion", 9) == 0 && isspace(line[9])) {
			strncpy(dversion, line + 10, sizeof dversion);
			dversion[sizeof dversion - 1] = '\0';
			dversion[strlen(dversion) - 1] = '\0';
			return 0;
		}
		if (line[0] == '$')
			return 1;
		nflds = sscanf(line,
			       "%32s %32s %32s %32s %32s %32s %32s %1s",
			       name, pfx, flg, order, bmaj, cmaj, cpu, dummy);
		if (nflds < 6) {
			pfmt(stderr, MM_ERROR, EMSG_NFLDS, "Master");
			show_line();
			error_exit();
		}
		if (nflds == 7) {
			strcpy(old_master_cpu, cpu);
			redo_system = 1;
		}
	}

	if (load_symtab("Driver.o", 1) != 0) {
		pfmt(stderr, MM_ERROR, ":234:Error reading symbol table for Driver.o file.\n");
		show_line();
		error_exit();
	}

	/*
	 * Derive the (most likely) appropriate set of $interface lines,
	 * since these did not exist before $version 2.
	 *
	 * To do this, we load the interface definition files and compare
	 * them against symbols in the Driver.o.  (We only check "Driver.o",
	 * since it assumed that any module old enough to predate $version 2
	 * is also old enough to predate type-specific Driver.o names.)
	 *
	 * We start selecting interfaces by scanning through the module's
	 * symbols, first choosing interface versions that are the only match
	 * for a particular symbol.
	 *
	 * Next, we scan the symbols a second time, looking for symbols
	 * which match only unselected interfaces.  These will always match
	 * multiple unselected interfaces.  This time through we just count
	 * the number of symbols which match in each interface.
	 *
	 * The third and successive passes through the symbols, we select
	 * the unselected interface with the highest symbol match count,
	 * find all symbols covered by that interface, and subtract them
	 * from all interfaces they match.  This is repeated until all the
	 * match counts go to zero.
	 *
	 * In order to avoid nasty potential problems which could occur if
	 * we get the wrong version of kmem_alloc(D3), we special case
	 * "ddi.6" (and greater)--we include "ddi.6" (or "ddi.6mp") if and
	 * only if the original file contains a "$dversion 6" and we never
	 * include ddi versions greater than 6.
	 *
	 * Other than this special case, if multiple interfaces satisfy the
	 * "highest symbol match count" test above, and they are relatively
	 * ordered by a $replace relationship, the most recent replacement
	 * is the one which is chosen; if there is no $replace relationship
	 * the choice is arbitrary.
	 */

	if (INSTRING(flg, EXECSW) || INSTRING(flg, FILESYS) ||
	    INSTRING(flg, DISP) || strcmp(name, "kernel") == 0) {
		/*
		 * These types of modules are inherently base modules,
		 * so we know we only need "$interface nonconforming".
		 */
		nintfc = 0;
		goto do_nonconforming;
	}

	if (interfaces == NULL) {
		load_interfaces();
		if (debug > 1)
			dump_interfaces();
		ddi_5mp = intfc_find("ddi", "5mp");
		ddi_6 = intfc_find("ddi", "6");
		for (intp = interfaces; intp; intp = intp->next_intfc) {
			for (intp2 = intp; intp2; intp2 = intp2->next_ver) {
				intp2->count = 0;
				intp2->flags = 0;
				if (intp2 == ddi_5mp || intp2 == ddi_6 ||
				    intfc_replaces(intp2, ddi_6) ||
				    (strcmp(intp2->name, "ddi") != 0 &&
				     strcmp(intp2->name, "sdi") != 0 &&
				     strcmp(intp2->name, "merge") != 0 &&
				     strcmp(intp2->name, "odieth") != 0 &&
				     strcmp(intp2->name, "oditok") != 0))
					intp2->flags = DONTPICK;
			}
		}
	}

	if (debug)
		fprintf(stderr, "Calculating $interface lines for %s\n", name);

	nonconforming = 0;
	total_count = 0;

	if (dversion[0] != '\0' || INSTRING(flg, 'p')) {
		/* select interface corresponding to explicit $dversion */
		if (INSTRING(flg, 'p')) {
			if (strcmp(dversion, "5") == 0)
				strcpy(dversion, "5mp");
			else if (strcmp(dversion, "6") == 0)
				strcpy(dversion, "6mp");
		}
		if ((intp = intfc_find("ddi", dversion)) != NULL) {
			intp->flags |= PREFER;
			intp->flags &= ~DONTPICK;
			if (debug) {
				fprintf(stderr,
					"%s.%s preferred by $dversion\n",
					intp->name, intp->version);
			}
		}
		dversion[0] = '\0';
	}

	(void) scan_symbols(select_singles, (void *)pfx, SS_UNDEF);
	(void) scan_symbols(count_matches, NULL, SS_UNDEF);
	while (total_count != 0)
		select_one();

	/*
	 * Output selected $interface lines.
	 */
	nintfc = 0;
	for (intp = interfaces; intp; intp = intp->next_intfc) {
		for (intp2 = intp; intp2; intp2 = intp2->next_ver) {
			if (!(intp2->flags & SELECTED))
				continue;
			fprintf(to_file, "$interface %s %s\n",
				intp2->name, intp2->version);
			++nintfc;
		}
	}
	if (nonconforming) {
do_nonconforming:
		fprintf(to_file, "$interface nonconforming\n");
		if (nintfc != 0) {
			fprintf(to_file,
				"# potentially non-conforming symbols:");
			conv_file = to_file;
			column = 80;
			(void) scan_symbols(show_nonconforming,
					    (void *)pfx, SS_UNDEF);
			fprintf(to_file, "\n");
		}
	}

	/*
	 * Check for xxxextmodname and convert it to $name
	 */
	edef_extmodname = define_entry("extmodname", 1);
	if (edef_extmodname == NULL) {
		pfmt(stderr, MM_ERROR, ":235:Not enough memory to convert Master file\n");
		error_exit();
	}
	lookup_entries(pfx, scan_symbols);
	if (edef_extmodname->has_sym) {
		fprintf(to_file, "$name %s\n",
			(char *)get_valptr(edef_extmodname->sname));
	}

	close_symtab();

	/*
	 * Convert the final Master line.
	 */
	is_blk = INSTRING(flg, BLOCK);
	is_chr = INSTRING(flg, CHAR);

	/* Convert flags */
	op = flg;
	np = nflg;
	if (set_master_loadable) {
		*np++ = LOADMOD;
		set_master_loadable = 0;
	}
	if (hwmod && (INSTRING(flg, HARDMOD) || is_blk || is_chr))
		*np++ = HARDMOD;
	while ((*np = *op++) != '\0') {
		/* KEEPNOD used to be 'r' (but 'r' meant required, too) */
		if (*np == 'r') {
			if (is_blk || is_chr)
				*np = KEEPNOD;
		}
		/* Old COMPAT flag ('C') means converted from version 0 */
		if (*np == 'C' && version == 1) {
			oversion = 0;
			continue;
		}
		/* CONSDEV used to be 'Q'; convert it */
		if (*np == 'Q')
			*np = CONSDEV;
		/* Strip off obsolete flags */
		if (!INSTRING(OLD_MFLAGS, *np) && *np != HARDMOD && *np != '-')
			np++;
	}
	if (np == nflg) {
		*np++ = '-';
		*np = '\0';
	}

	if (version == 1) {
		sprintf(line, "%s\t%s\t%s\t%s\t%s\t%s\n",
			name, pfx, nflg, order, bmaj, cmaj);
		goto new_line;
	}

	/*
	 * Conversion of execsw modules has cross-dependencies between the
	 * Master and System files, so must be done manually.
	 */
	if (INSTRING(nflg, EXECSW)) {
		pfmt(stderr, MM_ERROR, ":26:Old execsw module Master/System must be converted by hand.\n");
		show_line();
		error_exit();
	}

	/*
	 * Convert function list characters to new-style entry point names.
	 * In many cases, the old code ignored some of the function list
	 * characters for certain module types, providing no error checking.
	 * To allow for incorrectly-specified modules which "got away with it",
	 * we have to skip any such cases, since our interpretation of $entry
	 * is much stricter.
	 */
	for (op = func; *op != '\0';) {
		switch (*op++) {
		case 'o':
			if (is_blk || is_chr)
				fprintf(to_file, "$entry open\n");
			break;
		case 'c':
			if (is_blk || is_chr)
				fprintf(to_file, "$entry close\n");
			break;
		case 'r':
			if (is_chr)
				fprintf(to_file, "$entry read\n");
			break;
		case 'w':
			if (is_chr)
				fprintf(to_file, "$entry write\n");
			break;
		case 'i':
			if (is_chr)
				fprintf(to_file, "$entry ioctl\n");
			break;
		case 'L':
			if (is_chr)
				fprintf(to_file, "$entry chpoll\n");
			break;
		case 'M':
			if (is_chr)
				fprintf(to_file, "$entry mmap\n");
			break;
		case 'S':
			if (is_chr)
				fprintf(to_file, "$entry segmap\n");
			break;
		case 'z':
			if (is_blk)
				fprintf(to_file, "$entry size\n");
			break;
		case 'I':
			fprintf(to_file, "$entry init\n");
			break;
		case 's':
			fprintf(to_file, "$entry start\n");
			break;
		case 'p':
			fprintf(to_file, "$entry poll\n");
			break;
		case 'h':
			fprintf(to_file, "$entry halt\n");
			break;
		case 'E':
			fprintf(to_file, "$entry kenter\n");
			break;
		case 'X':
			fprintf(to_file, "$entry kexit\n");
			break;
		case '-':
			/*
			 * The following entry points have historically been
			 * ignored, so we continue to do so.
			 */
		case 'f':
		case 'e':
		case 'x':
			break;
		default:
			pfmt(stderr, MM_ERROR, ":27:Illegal function indicator in Master file\n");
			show_line();
			error_exit();
		}
	}

	/*
	 * Block device drivers must have strategy entry points,
	 * but they were not specified explicitly.
	 */
	if (INSTRING(nflg, BLOCK))
		fprintf(to_file, "$entry strategy\n");

	/*
	 * Filesystem modules must have init routines.
	 */
	if (INSTRING(nflg, FILESYS) && !INSTRING(func, 'I'))
		fprintf(to_file, "$entry init\n");

	/*
	 * Dispatcher modules must have _init routines.
	 */
	if (INSTRING(nflg, DISP))
		fprintf(to_file, "$entry _init\n");

	sprintf(line, "%s\t%s\t%s\t0\t%s\t%s\n",
		name, pfx, nflg, bmaj, cmaj);
	strcpy(old_master_dma, dma);

new_line:
	if (debug)
		fprintf(stderr, "New line: %s", line);

	if (need_oversion) {
		fprintf(to_file, "$oversion %d\n", oversion);
		need_oversion = 0;
	}

	return 1;
}

determine_hwmod(line)
char *line;
{
	struct sdev sdev;
	char conf;
	char fmt[78];
	char dummy[2];
	int n;

	memset(&sdev, 0, sizeof(sdev));
	sdev.dmachan = -1;

	sprintf(fmt, "%%%ds", NAMESZ - 1);
	strcat(fmt, " %c %ld %hd %hd %hd %lx %lx %lx %lx %hd %d %1s");
	n = sscanf(line, fmt,
			sdev.name,
			&sdev.conf,
			&sdev.unit,
			&sdev.ipl,
			&sdev.itype,
			&sdev.vector,
			&sdev.sioa,
			&sdev.eioa,
			&sdev.scma,
			&sdev.ecma,
			&sdev.dmachan,
			dummy);

	hwmod = (sdev.itype != 0 ||
		 sdev.eioa != 0 ||
		 sdev.ecma != 0 ||
		 (oversion != 0 && sdev.dmachan != -1));
}

int
convert_system(line, to_file, version)
	char	line[];
	FILE	*to_file;
	int	version;
{
	if (version == 1) {
		/*
		 * Convert version 1 System file to version 2:
		 *	Convert $loadable to Master file 'L' flag.
		 */
		if (strncmp(line, "$loadable", 9) == 0 && isspace(line[9])) {
			set_master_loadable = 1;
			return 0;
		}
	}
	if (need_oversion) {
		fprintf(to_file, "$oversion %d\n", oversion);
		need_oversion = 0;
	}

	determine_hwmod(line);

	if (version != 0)
		return 1;

	/* Version 0 System file conversion requires Master file for dmachan */
	if (!(partflag & MASTER)) {
		pfmt(stderr, MM_ERROR, ":28:Can't convert old System file w/o old Master file.\n");
		show_line();
		error_exit();
	}
	if (old_master_dma[0] != '\0') {
		sprintf(line + strlen(line) - 1, "\t%s\n", old_master_dma);
		need_dmachan = 0;
	} else
		need_dmachan = redo_system = 1;
	return 1;
}

int
process_master(line, to_file)
	char	line[];
	FILE	*to_file;
{
	struct mdev st_mast;
	int	is_blk, is_chr;
	int	stat;

	/*
	 * Tell rdinst() to ignore special directives, such as $entry,
	 * since we haven't set up all the conditions for accurate error
	 * checking, and since we don't care about them anyway.
	 */
	ignore_directives = 1;

	/* If this Master file entry is for a block or character device,
	 * and it is not marked 'required', we have to assign major number(s).
	 */
	if ((stat = rdinst(MDEV, line, (char *)&st_mast, 0, oversion)) != 1) {
		if (stat == I_MORE) {
			fputs(line, to_file);
			return 0;
		}
		strcpy(path, "Master");
		insterror(stat, MDEV, st_mast.name);
	}

	saw_hardmod = INSTRING(st_mast.mflags, HARDMOD);

	/*
	 * If the zflag is set, indicating that major numbers should be
	 * left as is, we don't have any special processing to do;
	 * just output the line and return.
	 */
	if (zflag) {
		fputs(line, to_file);
		return 0;
	}

	is_blk = INSTRING(st_mast.mflags, BLOCK);
	is_chr = INSTRING(st_mast.mflags, CHAR);
	if (is_blk || is_chr) {
		get_current_devs();
		rd_res_major();
		assign_devs(&st_mast, is_blk, is_chr);
	} else {
		st_mast.blk_start = st_mast.blk_end = 0;
		st_mast.chr_start = st_mast.chr_end = 0;
	}

	wrtmdev(&st_mast, to_file);
	return 1;
}

assign_devs(mdev, is_blk, is_chr)
	struct mdev *mdev;
	int	is_blk, is_chr;
{
	if (INSTRING(mdev->mflags, KEEPMAJ)) {
		if (is_blk)
			chk_res_major(BDEVL, mdev->name, mdev->blk_start, 
				mdev->blk_end);
		if (is_chr)
			chk_res_major(CDEVL, mdev->name, mdev->chr_start, 
				mdev->chr_end);
	} else {
		int	n_blk, n_chr;

		if ((mdev->blk_start | mdev->chr_start) != 0) {
			fprintf(stderr,
"Warning: %s Master file has non-zero major numbers, but no 'k' flag;\n",
				mdev->name);
			fprintf(stderr,
"\tignoring input numbers and assigning new major numbers.\n");
		}
		n_blk = mdev->blk_end - mdev->blk_start + 1,
		n_chr = mdev->chr_end - mdev->chr_start + 1,
		mdev->blk_start = mdev->blk_end = 0;
		mdev->chr_start = mdev->chr_end = 0;
		find_free_majors(is_blk, n_blk, is_chr, n_chr, mdev);
	}
}

get_current_devs()
{
	struct mdev md;
	int	is_blk, is_chr;
	int	stat;

	devlist[BDEVL] = devlist[CDEVL] = NULL;

	(void)getinst(MDEV_D, RESET, NULL);

	while ((stat = getinst(MDEV_D, NEXT, (char *)&md)) == 1) {
		is_blk = INSTRING(md.mflags, BLOCK);
		is_chr = INSTRING(md.mflags, CHAR);
		if (is_blk || is_chr)
			(void) enter_dev(&md, is_blk, is_chr, 0);
	}

	if (stat != 0)
		insterror(stat, MDEV_D, md.name);
}

struct devlist *
enter_dev(mdev, is_blk, is_chr, reserved)
	struct mdev *mdev;
	int	is_blk, is_chr;
	int	reserved;
{
	struct devlist *dv;

	dv = (struct devlist *)malloc(sizeof(struct devlist));
	if (dv == NULL) {
		pfmt(stderr, MM_ERROR, ":30:Not enough memory to search mdevice entries\n");
		error_exit();
	}

	dv->mdev = *mdev;
	dv->reserved = reserved;
	dv->used = 0;
	dv->link[0] = dv->link[1] = NULL;

	/* Link into block and char device lists, in order */
	if (is_blk)
		link_dev(dv, BDEVL, mdev->blk_start, mdev->blk_end);
	if (is_chr)
		link_dev(dv, CDEVL, mdev->chr_start, mdev->chr_end);

	return dv;
}

link_dev(dv, list, start, end)
	struct devlist *dv;
	int	list;
	int	start, end;
{
	struct devlist **dvp;

	dv->start[list] = start;
	dv->end[list] = end;
	dvp = &devlist[list];
	while (*dvp) {
		if ((*dvp)->start[list] < start) {
			dvp = &(*dvp)->link[list];
			continue;
		}
		if ((*dvp)->start[list] == start) {
			dv->used++;
		}
		break;
	}

	dv->link[list] = *dvp;
	*dvp = dv;
	if (debug)
		fprintf(stderr, "Entered %d-%d onto %s device list, used=%d\n", 
			start, end, list == BDEVL ? "block" : "char", dv->used);
}

unlink_dev(dv, list)
	struct devlist *dv;
	int	list;
{
	struct devlist **dvp;

	dvp = &devlist[list];
	while (*dvp != dv)
		dvp = &(*dvp)->link[list];
	*dvp = dv->link[list];
}


find_free_majors(is_blk, n_blk, is_chr, n_chr, mdev)
	int	is_blk, n_blk, is_chr, n_chr;
	struct mdev *mdev;
{
	struct devlist **bdp, **cdp;
	int	blk, chr;
	int	same;

	same = (is_blk && is_chr && INSTRING(mdev->mflags, UNIQ));

	bdp = &devlist[BDEVL];
	cdp = &devlist[CDEVL];
	blk = chr = 0;

	do {
		if (is_blk)
			blk = dev_search(&bdp, n_blk, BDEVL, same? chr : 0);
		if (is_chr)
			chr = dev_search(&cdp, n_chr, CDEVL, same? blk : 0);
	} while (same && blk != chr);

	if (is_blk)
		mdev->blk_end = (mdev->blk_start = blk) + n_blk - 1;
	if (is_chr)
		mdev->chr_end = (mdev->chr_start = chr) + n_chr - 1;

	if (debug) {
		if (is_blk)
			fprintf(stderr, "assigned free block majors %d-%d\n",
				mdev->blk_start, mdev->blk_end);
		if (is_chr)
			fprintf(stderr, "assigned free char majors %d-%d\n",
				mdev->chr_start, mdev->chr_end);
	}
}

/* find first hole in devlist at or past `maj', big enough for `n_maj' slots */
int
dev_search(dvpp, n_maj, list, maj)
	struct devlist ***dvpp;
	int	n_maj, list, maj;
{
	while (**dvpp != NULL) {
		if (maj + n_maj <= (**dvpp)->start[list]){
			maj = ((**dvpp)->start[list] - n_maj);
			break;
		}
		if (maj <= (**dvpp)->end[list])
			maj = (**dvpp)->end[list] + 1;
		*dvpp = &(**dvpp)->link[list];
	}
	return maj;
}


#if RECOVERY_CODE
mksave()
{
	int i, fd, from, to, ct;
	char cfile[PATH_MAX];
	char pfile[PATH_MAX];

	unlink ("/etc/.last_dev_add");
	if (!access("/etc/.last_dev_del", F_OK))
		system ("rm -rf /etc/.last_dev_del > /dev/null 2>&1");
	if (madedir) {
		if (debug)
			fprintf(stderr, "making /etc/.last_dev_add\n");
		if ((fd=creat("/etc/.last_dev_add", FILE_MODE))<0) {
			pfmt(stderr, MM_ERROR, ":31:Cannot create recovery files.\n");
			error_exit();
		}
		write(fd, device, sizeof(device));
		close(fd);
	}
	else if (dflag || uflag || Mflag) {
		if (debug)
			fprintf(stderr, "making /etc/.last_dev_del\n");
		mkdir("/etc/.last_dev_del", DIR_MODE);
		if ((fd=creat("/etc/.last_dev_del/dev", FILE_MODE))<0) {
			pfmt(stderr, MM_ERROR, ":32:Cannot create recovery file- /etc/.last_dev_del/dev\n");
			error_exit();
		}
		write(fd, device, sizeof(device));
		close(fd);

		if((from = open("/etc/conf/cf.d/mdevice", 0)) < 0) {
			pfmt(stderr, MM_ERROR, ":33:Cannot create recovery files - read mdevice.\n");
			error_exit();
		}
		if((to = creat("/etc/.last_dev_del/mdevice", FILE_MODE)) < 0) {
			pfmt(stderr, MM_ERROR, ":34:Cannot create recovery files - create mdevice.\n");
			error_exit();
		}
		while((ct = read(from, fbuf, SIZE)) != 0)
			if(ct < 0 || write(to, fbuf, ct) != ct) {
				pfmt(stderr, MM_ERROR, ":35:Cannot create recovery files - copy mdevice.\n");
				error_exit();
			}

		close(to);
		close(from);

		/* create all the other directories that may be needed in case 
		 * of removal or update of a DSP.
		 */

		if (debug)
			fprintf(stderr, "making /etc/.last_dev_del/pack.d\n");
		mkdir("/etc/.last_dev_del/pack.d", DIR_MODE);

		strcpy(cfile, root);
		for (i = sizeof(confdirs) / sizeof(char *); i-- > 0;) {
			strcpy(pfile, cfile);
			strcat(pfile, "/");
			strcat(pfile, confdirs[i]);
			strcat(pfile, "/");
			strcat(pfile, device);
			if (!access(pfile, F_OK)) {
				strcpy(pfile, "/etc/.last_dev_del/");
				strcat(pfile, confdirs[i]);
				if (debug)
					fprintf(stderr, "making %s\n", pfile);
				mkdir(pfile, DIR_MODE);
			}
		}
	}
}
#endif /* RECOVERY_CODE */


idunlink(unl)
char *unl;
{
	if (!kflag)
		unlink(unl);
}


int
ccopy(src, destd, destf, flags, cur_ver)
char *src, *destd, *destf;
int flags;
int cur_ver;
{
	struct stat sstat, dstat;
	char dpath[PATH_MAX];

	if (stat(src, &sstat) != 0)
		return FALSE; /* component file not present */

	strcpy(dpath, root);
	strcat(dpath, destd);

	/* If necessary, make directory */
	if (access(dpath, F_OK) != 0)
		nmkdir(dpath);

	strcat(dpath, "/");
	strcat(dpath, destf);

	if (Mflag) { /* check modification times */
		if (!(redo_system && (flags & (CP_SYSTEM|CP_MASTER))) &&
		    stat(dpath, &dstat) == 0 &&
		    dstat.st_mtime > sstat.st_mtime)
			return FALSE;
	}

	idcp(src, destd, destf, flags, cur_ver);
	return TRUE;
}

rd_res_major()
{
	FILE *fp;
	struct mdev mdev;
	int is_blk, is_chr, rtn, nfield;
	char type, range[RANGESZ], name[NAMESZ];

	if ((fp = fopen(resfile, "r")) == NULL) {
		pfmt(stderr, MM_ERROR, ":36:%s: can not open for mode %s\n", resfile, "r");
		error_exit();
	}

	if (debug)
		fprintf(stderr, "open file %s for reserved majors\n", resfile);

	while (fgets(linebuf, 80, fp) != NULL) {
		if (linebuf[0] == '#')
			continue;
		nfield = sscanf(linebuf, "%c %s %s", &type, range, name);
		if (nfield != 3) {
			pfmt(stderr, MM_ERROR, ":37:number of fields is incorrect\n");
		fprintf(stderr, "%s  %s", gettxt(":104", "FILE:"), resfile);
		fprintf(stderr, "%s  %s", gettxt(":21", "LINE:"), linebuf);
			error_exit();
		}
		
		if (debug)
			fprintf(stderr, "Reserved: %c\t%s\t%s\n", type, range, name);

		strcpy(mdev.name, name);
		is_blk = is_chr = 0;

		switch (type) {
		case 'b':
			is_blk = 1;
			rtn = getmajors(range, &mdev.blk_start, &mdev.blk_end);
			break;
		case 'c':
			is_chr = 1;
			rtn = getmajors(range, &mdev.chr_start, &mdev.chr_end);
			break;
		default:
			pfmt(stderr, MM_ERROR, ":38:unknown entry\n");
		fprintf(stderr, "%s  %s", gettxt(":104", "FILE:"), resfile);
		fprintf(stderr, "%s  %s", gettxt(":21", "LINE:"), linebuf);
			error_exit();
			break;
		}

		if (rtn != 0) {
			pfmt(stderr, MM_ERROR, ":39:illegal major number entry\n");
		fprintf(stderr, "%s  %s", gettxt(":104", "FILE:"), resfile);
		fprintf(stderr, "%s  %s", gettxt(":21", "LINE:"), linebuf);
			error_exit();
		}

		enter_dev(&mdev, is_blk, is_chr, 1);
	}

	fclose(fp);
}

chk_res_major(list, name, start, end)
{
	struct devlist **dvp;
	char modname[15];
	char errmsg[160];

	for (dvp = &devlist[list]; *dvp != NULL; dvp = &(*dvp)->link[list])
		if ((*dvp)->start[list] == start)
			if ((*dvp)->reserved && !(*dvp)->used)
				if ((*dvp)->end[list] == end) {
					if (debug)
						fprintf(stderr, "reserved major number (%d-%d) for %s is correct\n", start, end, name);

					return 0;
				} else {
					pfmt(stderr, MM_ERROR,
":40:%s Master file has %s major number range (%d-%d)\n\twhich don't match with the reserved range (%d-%d)\n",
					name, list == BDEVL? "block" : "char",
					start, end, (*dvp)->start[list],
					(*dvp)->end[list]);
					error_exit();
				}
			else {
				if ((*dvp)->reserved)
					strcpy(modname, (*dvp)->link[list]->mdev.name);
				else
					strcpy(modname, (*dvp)->mdev.name);

				pfmt(stderr, MM_ERROR, ":41:%s is using the %s major number reserved by %s (%d-%d)\n",
					modname, list == BDEVL? "block" : "char",
					name, start, end);
				error_exit();
			}

	pfmt(stderr, MM_ERROR,
":42:%s major number reserved by %s (%d-%d) is not in res_major file\n",
		list == BDEVL? "block" : "char", name, start, end);
	error_exit();
}

save_original(src, destd, destf)
char *src, *destd, *destf;
{
	char odestd[PATH_MAX], savfile[PATH_MAX], cmdline[PATH_MAX + 100];

	if (debug)
		fprintf(stderr, "Save original version of %s\n", src);

	sprintf(odestd, "%s/.%s", root, &destd[1]);
	sprintf(savfile, "%s/%s", odestd, destf);

	if (access(odestd, F_OK) != 0)
		nmkdir(odestd);
	else
		unlink(savfile);
	
	sprintf(cmdline, "cp %s %s", src, savfile);
	if (debug)
		fprintf(stderr, "%s\n", cmdline);
	if (system(cmdline) != 0) {
		pfmt(stderr, MM_ERROR, ":43:Cannot save the original file.\n");
		error_exit();
	}
}

void
nmkdir(dpath)
char *dpath;
{
	if (debug)
		fprintf(stderr, "making %s directory.\n", dpath);
	if (mkdir(dpath, DIR_MODE) != 0) {
		pfmt(stderr, MM_ERROR, ":44:Can't make %s directory.\n", dpath);
		error_exit();
	}
	setlevel(dpath, 2);
	if (Pflag) {
		fprintf(cfp, "%s d 0%o root sys 2 NULL NULL\n",
			dpath, DIR_MODE);
		if (debug)
			fprintf(stderr, "%sd 0%o root sys 2 NULL NULL\n",
				dpath, DIR_MODE);
	}
}

int
find_Driver(dirname, dpp, filenamep)
char *dirname;
DIR **dpp;
char **filenamep;
{
	char dirnm[PATH_MAX];
	struct dirent *direntp;

	if (*dpp == NULL) {
		if (dirname == NULL)
			dirname = ".";
		else {
			sprintf(dirnm, "%s/%s", root, dirname);
			dirname = dirnm;
		}
		*dpp = opendir(dirname);
		if (*dpp == NULL)
			return 0;
	}

	while ((direntp = readdir(*dpp)) != NULL) {
		if (strncmp(direntp->d_name, "Driver", 6) != 0)
			continue;
		if (strcmp(direntp->d_name + strlen(direntp->d_name) - 2,
			   ".o") != 0)
			continue;
		if (filenamep == NULL)
			closedir(*dpp);
		else
			*filenamep = direntp->d_name;
		return 1;
	}

	closedir(*dpp);

	return 0;
}


delete_tunables()
{
	char fnam[PATH_MAX];
	char patfile[PATH_MAX];
	char tmpfile[PATH_MAX];
	FILE *mtune_fp;
	FILE *pat_fp;
	char buf[PATH_MAX];
	char cmd[PATH_MAX];
	char *p;
	int count = 0;

	sprintf(fnam, "%s/mtune.d/%s", root, device);

	mtune_fp = fopen(fnam, "r");
	if (mtune_fp == NULL)
	{
		if (errno != ENOENT)
			perror(fnam);
		return;
	}

	strcpy(patfile, "/tmp/idXXXXXX");
	mktemp(patfile);

	pat_fp = fopen(patfile, "w");

	if (pat_fp == NULL)
	{
		perror(patfile);
		fclose(mtune_fp);
		return;
	}

	while (fgets(buf, PATH_MAX, mtune_fp) != NULL)
	{
		if (*buf == '*' || *buf == '#')
			continue;

		for (p = buf; *p && *p != ' ' && *p != '\t' && *p != '\n'; p++)
			;

		*p = '\0';

		if (*buf == '\0')
			continue;

		fprintf(pat_fp, "^%s[ \t]\n", buf);
		count++;
	}

	fclose(mtune_fp);
	fclose(pat_fp);

	strcpy(tmpfile, "/tmp/idXXXXXX");
	mktemp(tmpfile);

	if (count)
	{
		sprintf(cmd,
		"egrep -v -f %s %s/cf.d/stune > %s; cp %s %s/cf.d/stune",
				patfile, root, tmpfile, tmpfile, root);

		system(cmd);
	}

	unlink(tmpfile);
	unlink(patfile);
}

