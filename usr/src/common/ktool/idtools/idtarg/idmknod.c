/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ktool:common/ktool/idtools/idtarg/idmknod.c	1.16"
#ident	"$Header:"

/*
 * This program reads /etc/conf/node.d/* and creates nodes in /dev.
 *
 * Nodes that are not listed in mdevice.d as required (r) are removed
 * from /dev before the new nodes are created.
 *
 * The command line options are:
 *	-o directory	- the installation directory (/dev).
 *	-r directory	- the configuration directory (/etc/conf).
 *	-d sdevice_file - the file name for current sdevice info.
 *	-s 		- suppress removing nodes from /dev.
 *	-M module_name	- make nodes for the specified module only.
 *	-#		- print diagnostics.
 */

#include "inst.h"
#include "devconf.h"
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/mkdev.h>
#include <limits.h>

#include <locale.h>
#include <pfmt.h>

#define	TRUE	1
#define	FALSE	0

#define DIR_MODE	0755

#define MACDIR		1	/* SYS_PUBLIC */

/* directories */
#define	ENVIRON		0
#define OUTPUT		1
#define FULL_PATH	2

/* error messages */
#define USAGE	":104:Usage: idmknod [-o dev_dir] [-r conf_dir] [-M mod_name] [-s] [-d sdev] [-#]\n"
#define	STAT	":105:can not stat %s/%s\n"
#define	CHDIR	":106:can not chdir to %s\n"
#define MKNOD	":107:%s: can not make node. Errno = %d\n"
#define CHOWN	":108:%s: can not chown. Errno = %d\n"
#define	OPEN	":109:%s: can not open for mode '%s'\n"
#define UNKNOWN ":110:%s: unknown device\n"
#define ERASE	":111:can not erase node '%s'\n"
#define NOMATCH ":112:%s: type of node does not match the driver type\n"
#define OUTRANGE ":113:%s major offset produces major number out of range\n"
#define BADMINOR ":114:%s: node file for driver <%s> contains invalid minor number\n"

/* directories */
extern char current[];		/* current directory */
char output[PATH_MAX];		/* path name of '/dev' directory */

/* flags */
int sflag = 0;			/* suppress flag specified */
int mflag = 0;			/* individul module specified */
int oflag = 0;			/* output directory specified */
int rflag = 0;			/* root specified */
int debug = 0;			/* debug flag */
extern short eflag;		/* flag for errors */

DIR  *Opendir();
void mk_one_nod();
void proc_nodfile();

extern char *optarg;		/* used by getopt */
extern char instroot[];
extern char pathinst[];
extern char path[];
extern int setlevel();

struct modlist *modlist;

main(argc, argv)
int argc;
char *argv[];
{
	char root[PATH_MAX], input[PATH_MAX];
	int c;
	struct modlist *mod;

	umask(022);

        (void) setlocale(LC_ALL, "");
        (void) setcat("uxidtools");
        (void) setlabel("UX:idmknod");

	while ((c = getopt(argc, argv, "M:r:o:d:s#?")) != EOF)
		switch (c) {
		case 'r':	/* contains node.d, mdevice.d, etc */
			rflag++;
			strcpy(root, optarg);
			break;
		case 'o':	/* /dev directory */
			oflag++;
			strcpy(output, optarg);
			break;
		case 'd':
			ftypes[SDEV].fname = optarg;
			break;
		case 's':
			sflag++;
			break;
		case 'M':
			mflag++;
			mod = (struct modlist *)malloc(sizeof(struct modlist));
			strcpy(mod->name, optarg);
			mod->next = modlist;
			modlist = mod;
			break;
		case '#':
			debug++;
			break;
		case '?':
			pfmt(stderr, MM_ACTION, USAGE);
			fatal(0);
		}

	eflag = 0;

	/* get current directory */
	getcwd(current, 512);

	/* get full path name */
	getpath(rflag, root, ROOT);
	strcpy(instroot, root);
	sprintf(pathinst, "%s/%s", root, CFDIR);
	getpath(oflag, output, DROOT);

	/* get Master/System device info */

	/*
	 * Tell rdinst() to ignore special directives, such as $entry,
	 * since we haven't set up all the conditions for accurate error
	 * checking, and since we don't care about them anyway.
	 */
	ignore_directives = 1;

	getdevconf(NULL);

	if (mflag) {
		umask(0);

		sprintf(input, "%s/node.d", root);
		if (debug)
			fprintf(stderr, "idmknod:\ninput: %s\noutput: %s\n", input, output);

		for (mod = modlist; mod != NULL; mod = mod->next)
			proc_nodfile(mod->name);	
	} else {
		if (!sflag)
			deverase();		/* erase nodes */
		create_nodes();		/* create new node files */
	}

	exit(eflag);
}

/* dev erase nodes that are not required or automatically installed */

deverase()
{
	erase (output);
}

/* erase is recursive. It looks at /dev, and subdirecories therein. */

erase(rdir)
char *rdir;
{
	char buf[512];
	char cmd[80];
	struct stat sbuf;
	static rcnt = 0;

	struct dirent *direntp;
	DIR *dp;

	rcnt++;
	if (debug)
		fprintf(stderr, "debug: chdir to %s, depth= %d\n", rdir,rcnt);
	if(chdir(rdir) < 0){
		perror(rdir);
		exit(1);
	}
	dp = Opendir(".");
	while (direntp = readdir (dp)) {
		if (direntp->d_ino == 0 || direntp->d_name[0] == '.')
			continue;
		if (debug)
			fprintf(stderr, "debug: node='%s'\n", direntp->d_name);
		/* stat node */
		if (lstat(direntp->d_name, &sbuf) == -1) {
			pfmt(stderr, MM_ERROR, STAT, rdir, direntp->d_name);
			error(0);
		}
		/* check if directory (subdirectory of /dev) */
		if ((sbuf.st_mode & S_IFMT) == S_IFDIR) {
			if (debug) {
				fprintf(stderr,"debug: file mode 0x%x\n",sbuf.st_mode);
				fprintf(stderr,"debug: subdir %s\n", direntp->d_name);
			}
			strcpy(buf, direntp->d_name);
			erase(buf);	/* RECURSION */
			continue;
		}

		/* check if character or block special node */
		if ( ((sbuf.st_mode & S_IFMT) != S_IFBLK) &&
			((sbuf.st_mode & S_IFMT) != S_IFCHR) )
				continue;

		/* if mdevice lists as required or automatically installed
		 * device, do not erase node */
		if(debug) {
			fprintf(stderr,"sbuf.st_rdev is 0x%x\n",sbuf.st_rdev);
			fprintf(stderr,"major(sbuf.st_rdev) is 0x%x\n",major(sbuf.st_rdev));
		}
		if (required(major(sbuf.st_rdev), (sbuf.st_mode & S_IFBLK) == S_IFBLK))
			continue;

		/* erase node */
		if (debug)
			fprintf(stderr,"debug: unlinking %s\n",direntp->d_name);
		if (unlink(direntp->d_name) == -1) {
			/* /dev/fd doesn't support unlink operation */
			if (errno != ENOSYS) {
				pfmt(stderr, MM_ERROR, ERASE, direntp->d_name);
				error(0);
			}
		}
	}
	closedir(dp);
	if(chdir("..") < 0){
		pfmt(stderr, MM_ERROR, CHDIR, "..");
		fatal(0);
	}
	/* Try to rmdir subdirectories; don't report failure */
	if (rcnt-- > 1){
		if (debug)
			fprintf(stderr,"debug: trying to remove %s\n",rdir);
		rmdir(rdir);
	}
}


/* create new nodes (from /etc/conf/node.d files) */

create_nodes()
{
	struct node node;
	int stat;

	umask(0);

	(void)getinst(NODE_D, RESET, NULL);

	while ((stat = getinst(NODE_D, NEXT, &node)) != 0) {

		if (stat < 0) {
			show_ierr(stat, NODE_D, "");
			error(1);
			continue;
		}

		if (!configured(node.major))
			continue;

		mk_one_nod(&node);
	}
}


/* open a directory */

DIR *
Opendir(directory)
char *directory;
{
	DIR *dp;

	if (debug)
		fprintf(stderr, "debug: open directory '%s' for mode 'r'\n",
			directory);

	if ((dp = opendir(directory)) == NULL) {
		perror(directory);
		exit(1);
	}
	return(dp);
}


/* Check that the device type is the same as listed in the node file. */

checktype(dev, type)
char *dev;
int type;
{
	register driver_t *drv;

	if ((drv = mfind(dev)) == NULL) {
		/* could not find device */
		pfmt(stderr, MM_ERROR, UNKNOWN, dev);
		error(0);
		return(-1);
	}

	/* check that type, either b or c, is also specified in mdev.mflags */
	return(INSTRING(drv->mdev.mflags, type)); 
}


/* get the major number for a device from mdevice */

getmajor(dev, type, offset)
char *dev;
char type;
unsigned offset;
{
	register driver_t *drv;
	unsigned maj;

	if ((drv = mfind(dev)) == NULL) {
		/* could not find device */
		pfmt(stderr, MM_ERROR, UNKNOWN, dev);
		error(0);
		return(-1);
	}

	if (type == BLOCK) {
		if ((maj = drv->mdev.blk_start + offset) > drv->mdev.blk_end) {
			pfmt(stderr, MM_ERROR, OUTRANGE, "block");
			error(0);
			return(-1);
		}
	} else {
		if ((maj = drv->mdev.chr_start + offset) > drv->mdev.chr_end) {
			pfmt(stderr, MM_ERROR, OUTRANGE, "character");
			error(0);
			return(-1);
		}
	}
	return maj;
}


/* check if required device */

required(maj, blk)
unsigned maj;
int blk;
{
	register driver_t *drv;

	for (drv = driver_info; drv != NULL; drv = drv->next) {
		if (blk) {
			if (!INSTRING(drv->mdev.mflags, BLOCK))
				continue;
			if (maj < drv->mdev.blk_start ||
			    maj > drv->mdev.blk_end)
				continue;
		} else {
			if (!INSTRING(drv->mdev.mflags, CHAR))
				continue;
			if (maj < drv->mdev.chr_start ||
			    maj > drv->mdev.chr_end)
				continue;
		}
		return INSTRING(drv->mdev.mflags, KEEPNOD);
	}
	return(0);
}


mkdirs(dpath)
char *dpath;
{
	char tpath[PATH_MAX], fpath[PATH_MAX];
	int i;

	for (i=0; dpath[i] != '\0'; i++){
		if (dpath[i] != '/')
			tpath[i]=dpath[i];
		else{
			tpath[i] = '\0';
			sprintf(fpath, "%s/%s", output, tpath);
			if (debug)
				fprintf(stderr,"making subdirectory %s\n",fpath);
			if (mkdir(fpath, DIR_MODE) != 0)
				if (errno != EEXIST){
					perror(fpath);
					exit(1);
				}
			setlevel(fpath, MACDIR);
			tpath[i]=dpath[i];
		}
	}
}


configured(dev)
char *dev;
{
	register driver_t *drv;

	if ((drv = mfind(dev)) == NULL) {
		/* could not find device */
		pfmt(stderr, MM_ERROR, UNKNOWN, dev);
		fatal(0);
	}

	return (drv->n_ctlr != 0);
}

void
proc_nodfile(modname)
char *modname;
{
	struct node node;
	int stat;

	(void)getinst(NODE_D, RESET, NULL);

	while ((stat = getinst(NODE_D, modname, &node)) != 0) {
		if (stat < 0) {
			if (stat != IERR_OPEN) {
				show_ierr(stat, NODE_D, modname);
				error(1);
			}
			return;
		}
		if (!configured(node.major))
			return;
		mk_one_nod(&node);
	}
}

void
mk_one_nod(node)
struct node *node;
{
	char buf[PATH_MAX];
	unsigned maj;
	int mode;

        /* check that the type specified matches mdevice */
        if (checktype(node->major, node->type) <= 0) {
		pfmt(stderr, MM_ERROR, NOMATCH, node->major);
                error(0);
                return;
        }

	/* resolve minor number, if defined as major for another dev */
	/* find the major number of the device */
	if (node->majminor[0]) {
		node->minor = getmajor(node->majminor,
				      node->type, node->maj_off);
		if (node->minor == -1)
			return;
		if ((maj = getmajor(node->major, node->type, 0))
				== -1)
			return;
	} else {
		if ((maj = getmajor(node->major, node->type, node->maj_off))
				== -1)
			return;
	}

	mode = (node->type == BLOCK ? S_IFBLK : S_IFCHR) | node->mode;

	if (INSTRING(node->nodename, '/'))
		mkdirs(node->nodename);

	sprintf(buf, "%s/%s", output, node->nodename);
	if (debug)
		fprintf(stderr, "debug: mknod %s %c 0x%x\n",
			node->nodename, node->type,
			makedev(maj, node->minor));

	if (mflag)
		if (unlink(buf) == -1 && errno != ENOENT) {
			pfmt(stderr, MM_ERROR, ERASE, buf);
			error(0);
		}

	if (mknod(buf, mode, makedev(maj, node->minor)) == -1) {
		if (errno != EEXIST || node_exists(buf, mode,
				makedev(maj, node->minor)) == -1) {
		        if (errno == EINVAL)
		        	pfmt(stderr, MM_ERROR, BADMINOR, node->nodename,
					node->major);
			else
				pfmt(stderr, MM_ERROR, MKNOD, node->nodename, errno);
			error(0);
			return;
		}
	}

	if (node->uid != -1 && node->gid != -1) {
		if (chown(buf, node->uid, node->gid) == -1) {
			pfmt(stderr, MM_ERROR, CHOWN, node->nodename, errno);
			error(0);
		}
	}

	setlevel(buf, node->level);
}

/*
 * Check if requested device exists
 */
node_exists(file, mode, dev)
char *file; 
int mode; 
dev_t dev;
{
	int serrno, error = 0;
	struct stat st;

	serrno = errno;
	mode &= S_IFMT;
	if(stat(file, &st) == -1 || (st.st_mode & S_IFMT) != mode 
			|| st.st_rdev != dev)
		error = -1;

	errno = serrno;

	return(error);
}
