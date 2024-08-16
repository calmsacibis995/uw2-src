/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ktool:common/ktool/idtools/idtarg/idmkenv.c	1.11"
#ident	"$Header:"

/*
 *
 * Idmkenv: This command is part of the Installable Drivers (ID) 
 *	    scheme.  It is called at boot time by an sysinit 
 *	    entry in /etc/inittab.  It's purpose is to update 
 *	    /etc/idrc.d, /etc/idsd.d, /etc/inittab,
 *	    and the device nodes in /dev.
 *
 *          A second function of idmkenv is to recover the Kernel
 *          environment when a reconfiguration is aborted due to
 *          losing power or the user hitting RESET.
 *
 *	    Idmkenv is coded in C for System V Release 4.0 instead of shell
 *	    because a number of the commands it depended on (e.g., 
 *	    chown, chmod, cat, mkdir) were moved from /sbin to /usr/bin.  
 *	    On systems where /usr is a separate file system, idmkenv 
 *	    runs before /usr is mounted and these commands would not 
 *	    be available.
 */

#include "inst.h"
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <varargs.h>
#include <sys/ksym.h>
#include <errno.h>

#include <locale.h>
#include <pfmt.h>

#define RECOVERY_CODE	0	/* The recovery code is broken in a large
				   number of ways, so for now, disable it. */

#define	SIZE	1024
#define	MODE	0777

#define RD_ERROR	":98:Unable to read all contents of %s%s\n"

void mkenv(), recover();
void runcmd();
void build_str(), terminate_str();
extern int setlevel();

char *BIN = "/etc/conf/bin";
char *PACK = "/etc/conf/pack.d";
char *CF = "/etc/conf/cf.d";
char *RC = "/etc/idrc.d";
char *SD = "/etc/idsd.d";
#if RECOVERY_CODE
char *LASTDEV_A = "/etc/.last_dev_add";
char *LASTDEV_D = "/etc/.last_dev_del";
#endif

char errbuf[LINESZ];


/* High-level structure of the program consists of setting up the kernel
 * environment and attempting to recover an aborted reconfiguration.
 */

main()
{

	umask(022);

	mkenv();
	recover();
	exit(0);
}


/*
 *  idkname functions, borrowed from the idkname command
 *  included here for better performance during system boot
 */

idkname_links()
{
	char buf[MAXPATHLEN];
	char *base;
	struct mioc_rksym rks;
	int fd, m;
	struct stat statbuf;
	char kernel[MAXPATHLEN];
	char conf[MAXPATHLEN];

	rks.mirk_symname = "kernel_name";
	rks.mirk_buf = buf;
	rks.mirk_buflen = MAXPATHLEN - 1;

	if ((fd = open("/dev/kmem", O_RDONLY)) < 0) {
		perror("/dev/kmem");
		exit(errno);
	}

	if (ioctl(fd, MIOC_IREADKSYM, &rks) < 0) {
		perror("MIOC_IREADKSYM");
		exit(errno);
	}
	close(fd);

	for (base = buf + strlen(buf); base != buf; base--) {
		if (base[-1] == '/')
			break;
	}

	strcpy(kernel, rks.mirk_buf);

	if (strcmp(base, "unix") == 0) {
		strcpy(conf, ROOT);
	} else {
		sprintf(conf, "%s.%s", ROOT, base);

		if (stat(conf, &statbuf) != 0 ||
		   !(statbuf.st_mode & S_IFDIR))
			strcpy(conf, ROOT);
	}

	unlink("/unix");
	unlink("/etc/mod_register");

	symlink(kernel, "/unix");

	sprintf(buf, "%s/mod_register", conf);
	symlink(buf, "/etc/mod_register");
}

void
mkenv()
{
	struct stat pstat;
	struct stat fstat;
	struct dirent *dir_entry;
	DIR *dirp;
	struct passwd *pwd;
	struct group  *grp;
	char fullpath[MAXPATHLEN];
	char buf[MAXPATHLEN];
	char *ebuf;
	uid_t uid;
	gid_t gid;

        (void) setlocale(LC_ALL, "");
        (void) setcat("uxidtools");
        (void) setlabel("UX:idmkenv");

	idkname_links();

	strcpy(fullpath, ROOT);	/* ROOT=/etc/conf in inst.h */
	/* indicates whether new unix was built */
	strcat(fullpath, "/.new_unix");

	if (stat(fullpath, &pstat) == 0) {
		pfmt(stderr, MM_INFO, ":99:Setting up new kernel environment\n");
		fflush(stdout);
		runcmd("rm -f /etc/idrc.d/* /etc/idsd.d/* /etc/ps_data");

		/* copy contents of rc.d, sd.d directories onto /etc */

		sprintf(buf, "%s/rc.d", ROOT);
		if ((dirp = opendir(buf)) != NULL) {
			ebuf = buf + strlen(buf);
			*ebuf++ = '/';
			while (dir_entry = readdir(dirp)) {

				/* only link regular files; "." and ".." entries
				 * are ignored */
				if (stat(dir_entry->d_name, &fstat) == 0)
					if ((fstat.st_mode & S_IFMT) != S_IFREG)
						continue;

				/* buf contains the path of the file we want to
				 * link. "fullpath" contains the full path of
				 * the target dir/file.
				 */
				strcpy(ebuf, dir_entry->d_name);
				sprintf(fullpath, "%s/%s", RC, dir_entry->d_name);
				if(link(buf, fullpath)) {
					pfmt(stderr, MM_ERROR, ":97:link(%s, %s) failed: %s\n",
						buf, fullpath, strerror(errno));
					exit(1);
				}
				setlevel(fullpath, 2);
			}

		}
		closedir(dirp);

		sprintf(buf, "%s/sd.d", ROOT);
		if ((dirp = opendir(buf)) != NULL) {
			ebuf = buf + strlen(buf);
			*ebuf++ = '/';
			while (dir_entry = readdir(dirp)) {

				/* only link regular files; "." and ".." entries
				 * are ignored */
				if (stat(dir_entry->d_name, &fstat) == 0)
					if ((fstat.st_mode & S_IFMT) != S_IFREG)
						continue;

				/* buf contains the path of the file we want to
				 * link. "fullpath" contains the full path of
				 * the target dir/file.
				 */
				strcpy(ebuf, dir_entry->d_name);
				sprintf(fullpath, "%s/%s", SD, dir_entry->d_name);

				if(link(buf, fullpath)) {
					pfmt(stderr, MM_ERROR, ":97:link(%s, %s) failed: %s\n",
						buf, fullpath, strerror(errno));
					exit(1);
				}
				setlevel(fullpath, 2);
			}

		}
		closedir(dirp);

		/* invoke idmknod and idmkinit commands */
		runcmd("%s/idmknod", BIN);
		runcmd("%s/idmkinit", BIN);

		/* get numerical value for owner bin, group bin for chown syscall */

		pwd = getpwnam("bin");
		uid = pwd->pw_uid;

		grp = getgrnam("bin");
		gid = grp->gr_gid;

		sprintf(buf, "%s/inittab", CF);
		if (chown(buf, uid, gid)) {
			perror(buf);
			exit(1);
		}

		if (chmod(buf, 0444)) {
			perror(buf);
			exit(1);
		}
		runcmd("mv %s/inittab /etc", CF);

#if RECOVERY_CODE
		runcmd("rm -rf /etc/conf/.new_unix /etc/conf/.unix_reconf %s %s", LASTDEV_A, LASTDEV_D);
#else
		runcmd("rm -f /etc/conf/.new_unix /etc/conf/.unix_reconf");
#endif
		chmod("/etc/inittab", 0444);
		setlevel("/etc/inittab", 1);
		runcmd("sync");
		runcmd("telinit q");
	}
}

void recover()
{
	register int i;
	struct stat pstat;
	struct stat fstat;
	char fullpath[64];
	unsigned int size;
	char *buf;
	int fd, rsize;

	strcpy(fullpath, ROOT);			/* ROOT=/etc/conf in inst.h */
	strcat(fullpath, "/.unix_reconf");	/* indicates whether recovery is
						   needed */

	if (stat(fullpath, &pstat) == 0) {
#if RECOVERY_CODE
		printf("\n\tRecovering Kernel configuration.\n");
		fflush(stdout);
		if (stat(LASTDEV_A, &fstat) == 0) {
			size = (unsigned int)fstat.st_size;
			buf = (char *)malloc(size + 1);
			if ((fd = open(LASTDEV_A, O_RDONLY)) == -1) {
				free(buf);
				perror(LASTDEV_A);
				exit(1);
			}
			else {
				rsize = read(fd, buf, size);
				if (rsize != size) 
					pfmt(stderr, MM_ERROR, RD_ERROR, LASTDEV_A, "");
				buf[size] = '\0';

				build_str(buf);
				runcmd("%s/idinstall -d %s > /dev/null 2>&1",
					BIN, buf);
			}
		}
		else {
			if (stat(LASTDEV_D, &fstat) == 0)
				if ((fstat.st_mode & S_IFMT) == S_IFDIR) {

					if (chdir(LASTDEV_D)) {
						perror(LASTDEV_D);
						exit(1);
					}
					if(stat("mdevice", &fstat)) {
						sprintf(errbuf, "%s/mdevice", LASTDEV_D);
						perror(errbuf);
						exit(1);
					}
					runcmd("mv mdevice %s/mdevice",CF);

					if(stat("dev", &fstat)) {
						sprintf(errbuf, "%s/dev", LASTDEV_D);
						perror(errbuf);
						exit(1);
					}
					size = (unsigned int)fstat.st_size;
					buf = (char *)malloc(size + 1);
					if ((fd = open("dev", O_RDONLY)) == -1) {
						free(buf);
						perror(LASTDEV_D);
						exit(1);
					}

					rsize = read(fd, buf, size);
					if (rsize != size) 
						pfmt(stderr, MM_ERROR, RD_ERROR, LASTDEV_A, "/dev");
					buf[size] = '\0';

					terminate_str(buf);

					if (stat("pack.d", &fstat) == 0) {
						sprintf(fullpath, "%s/%s", PACK, buf);
						if (mkdir(fullpath, MODE)) {
							perror(fullpath);
							exit(1);
						}
						runcmd("mv pack.d/* %s", fullpath);
					}
					else {
						sprintf(errbuf, "%s/pack.d", LASTDEV_D);
						perror(errbuf);
						exit(1);
					}
				}
		}
		runcmd("rm -rf %s %s", LASTDEV_A, LASTDEV_D);
#endif /* RECOVERY_CODE */
		chdir(CF);
		runcmd("rm -f unix config.h conf.c direct ifile buffers.o");
		runcmd("rm -f conf.o gdt.o linesw.o space.o sdevice.new");
		runcmd("rm -f /etc/conf/pack.d/*/space.o");
		runcmd("rm -f /etc/conf/pack.d/*/tune.o");
		runcmd("rm -f /etc/conf/.unix_reconf");
	}
}

/* This routine takes a variable number of arguments to pass them to the "system"
 * library routine.
 */

void runcmd(va_alist)
va_dcl
{
	va_list args;
	char *fmt;
	char buf[SIZE];

	va_start(args);
	fmt = va_arg(args, char *);
	va_end(args);
	vsprintf(buf, fmt, args);
	system(buf);
	return;
}

/* This routine builds a string of files (one per line) by substituting all
 * new line characters by black space.
 */

void build_str(s)
char *s;
{
	register i;

	for (i = 0; s[i] != '\0'; i++)
		if (s[i] == '\n')
			s[i] = ' ';
}

/* This routine terminates a string that's assumed to consist of only
 * one file name (otherwise the mkdir will not work). It substitutes the
 * first white space seen with a null character.
 */

void terminate_str(s)
char *s;
{
	register i;

	for (i = 0; s[i] != '\0'; i++)
		if (s[i] == ' ' || s[i] == '\t' || s[i] == '\n') {
			s[i] = '\0';
			break;
		}
}

