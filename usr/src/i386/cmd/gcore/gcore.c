/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)gcore:i386/cmd/gcore/gcore.c	1.1.9.4"
#ident  "$Header: gcore.c 1.3 91/08/12 $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
*/

/*******************************************************************

		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 

/*
**	Note: a couple of the included header files are
**	Machine Specific.  Also, below, where we set the
**	Elf Header for the core file, are a couple of
**	Machine Specific lines of code, commented by
**		Machine Specific.
*/

#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/fstyp.h>
#include <sys/fsid.h>
#include <sys/stat.h>
#include <sys/file.h>

#include <sys/immu.h>	/* Machine Specific */

#include <sys/user.h>
#include <sys/sysmacros.h>
#include <sys/procfs.h>
#include <sys/elf.h>
#include <sys/elf_386.h>	/* Machine Specific */
#include <sys/mman.h>
#include <sys/utsname.h>

#define	TRUE	1
#define	FALSE	0

/* Error returns from Pgrab() */
#define	G_NOPROC	(-1)	/* No such process */
#define	G_ZOMB		(-2)	/* Zombie process */
#define	G_PERM		(-3)	/* No permission */
#define	G_BUSY		(-4)	/* Another process has control */
#define	G_SYS		(-5)	/* System process */
#define	G_SELF		(-6)	/* Process is self */
#define	G_STRANGE	(-7)	/* Unanticipated error, perror() was called */
#define	G_INTR		(-8)	/* Interrupt received while grabbing */

#define	TADDR	((caddr_t)(0x80800000))

typedef struct {
	Elf32_Word namesz;
	Elf32_Word descsz;
	Elf32_Word type;
	char	   name[8];
} Elf32_Note;

#define CF_T_PRSTATUS	10
#define CF_T_FPREG	12
#define CF_T_PRPSINFO	13
#define CF_T_PRCRED	14
#define CF_T_UTSNAME	15
#define CF_T_LWPSTATUS	16
#define CF_T_LWPSINFO	17

extern	void	exit();
extern	void	perror();
extern	unsigned alarm();
extern	long	lseek();
extern	int	ioctl();
extern	int	stat();
extern	long	ulimit();
extern	long	strtol();

extern	int	getopt();
extern	char *	optarg;
extern	int	optind;

static	void	alrm();
static	pid_t	getproc();
static	int	dumpcore();
static	int	grabit();
static	int	isprocdir();
static	int	Pgrab();
static	int	Ioctl();
static	int	Setioctl();

char *	command = NULL;		/* name of command ("gcore") */
char *	filename = "core";	/* default filename prefix */
char *	procdir = "/proc";	/* default PROC directory */
int	timeout = FALSE;	/* set TRUE by SIGALRM catcher */
long	buf[4096];		/* big buffer, used for almost everything */
pstatus_t prstat;		/* prstatus info */

char mapname[PATH_MAX];
char asname[PATH_MAX];
char ctlname[PATH_MAX];
char statusname[PATH_MAX];

int as_fd = -1;		/* as file descriptor */
int status_fd = -1;	/* status file descriptor */
int ctl_fd = -1;	/* ctl file descriptor */
int map_fd = -1;	/* map file descriptor */
int error = 0, err = 0;

main(argc, argv)
	int argc;
	char **argv;
{
	int retc = 0;
	int opt;
	int errflg = FALSE;

	command = argv[0];

	/* options */
	while ((opt = getopt(argc, argv, "o:p:")) != EOF) {
		switch (opt) {
		case 'o':		/* filename prefix (default "core") */
			filename = optarg;
			break;
		case 'p':		/* alternate /proc directory */
			procdir = optarg;
			break;
		default:
			errflg = TRUE;
			break;
		}
	}

	argc -= optind;
	argv += optind;

	if (errflg || argc <= 0) {
		(void) fprintf(stderr,
			"usage:\t%s [-o filename] [-p procdir] pid ...\n",
			command);
		exit(2);
	}

	if (!isprocdir(procdir)) {
		(void) fprintf(stderr,
			"%s: %s is not a PROC directory\n",
			command, procdir);
		exit(2);
	}

	/* catch alarms */
	(void) sigset(SIGALRM, alrm);

	while (--argc >= 0) {
		int pfd;
		pid_t pid;
		char *pdir;

		/* close /proc files's */

		if (as_fd != -1) {
			(void) close(as_fd);
			as_fd = -1;
		}
		if (status_fd != -1) {
			(void) close(status_fd);
			status_fd = -1;
		}
		if (ctl_fd != -1) {
			(void) close(ctl_fd);
			ctl_fd = -1;
		}

		/* get the specified pid and its /proc directory */
		pid = getproc(*argv++, &pdir);

		if (pid < 0 || (pfd = grabit(pdir, pid)) < 0) {
			retc++;
			continue;
		}

		if (dumpcore(pfd, pdir, pid) != 0)
			retc++;

		(void) close(pfd);
	}

	return(retc);
}

static pid_t		/* get process id and /proc directory */
getproc(path, pdirp)	/* return pid on success, -1 on failure */
	register char * path;	/* number or /proc/nnn */
	char ** pdirp;		/* points to /proc directory on success */
{
	register char * name;
	register pid_t pid;
	char *next;

	if ((name = strrchr(path, '/')) != NULL)	/* last component */
		*name++ = '\0';
	else {
		name = path;
		path = procdir;
	}

	pid = strtol(name, &next, 10);
	if (isdigit(*name) && pid >= 0 && *next == '\0') {
		if (strcmp(procdir, path) != 0
		 && !isprocdir(path)) {
			(void) fprintf(stderr,
				"%s: %s is not a PROC directory\n",
				command, path);
			pid = -1;
		}
	} else {
		(void) fprintf(stderr, "%s: invalid process id: %s\n",
			command, name);
		pid = -1;
	}

	if (pid >= 0)
		*pdirp = path;
	return(pid);
}

static int
grabit(dir, pid)		/* take control of an existing process */
	char * dir;
	pid_t pid;
{
	int gcode;

	gcode = Pgrab(dir, pid);

	if (gcode >= 0)
		return(gcode);
	
	if (gcode == G_INTR)
		return(-1);

	(void) fprintf(stderr, "%s: %s.%d not dumped", command, filename, pid);
	switch (gcode) {
	case G_NOPROC:
		(void) fprintf(stderr, ": %d: No such process", pid);
		break;
	case G_ZOMB:
		(void) fprintf(stderr, ": %d: Zombie process", pid);
		break;
	case G_PERM:
		(void) fprintf(stderr, ": %d: Permission denied", pid);
		break;
	case G_BUSY:
		(void) fprintf(stderr, ": %d: Process is traced", pid);
		break;
	case G_SYS:
		(void) fprintf(stderr, ": %d: System process", pid);
		break;
	case G_SELF:
		(void) fprintf(stderr, ": %d: Cannot dump self", pid);
		break;
	}
	(void) fputc('\n', stderr);

	return(-1);
}

/*ARGSUSED*/
static void
alrm(sig)
	int sig;
{
	timeout = TRUE;
}

	
/* 
 * 
 *      Following is the format of the core file that will be dumped:
 *
 *
 *      *********************************************************
 *      *                                                       *
 *      *               Elf header                              *
 *      *********************************************************
 *      *                                                       *
 *      *                                                       *
 *      *               Program header:                         *
 *      *                                                       *
 *      *                       One entry per note section.     *
 *      *                                                       *
 *      *                       One entry for each region of    *
 *      *                       memory in the addrress space    *
 *      *                       with different permissions.     *
 *      *                                                       *
 *      *********************************************************
 *      *                                                       *
 *      *               Note sections:                          *
 *      *                                                       *
 *      *                       For a process with N LWPs       *
 *      *                       there will be N+1 note          *
 *      *                       sections - (a note section per  *
 *      *                       LWP and a process-wide note     *
 *      *                       section.                        *
 *      *                                                       *
 *      *********************************************************
 *      *                                                       *
 *      *               Dump of the address space.              *
 *      *                                                       *
 *      *********************************************************
 */
static int
dumpcore(pfd, pdir, pid)
	int pfd;		/* process file descriptor */
	char *pdir;		/* proc directory */
	pid_t pid;		/* process-id */
{
	int dfd;			/* dump file descriptor */
	int nsegments;			/* current number of segments */
	Elf32_Ehdr ehdr;		/* ELF header */
	Elf32_Phdr *v;			/* ELF program header */
	psinfo_t psstat;
	prcred_t prcred;
	struct utsname u_name;
	lwpsinfo_t lwpsinfo;
	lwpstatus_t lwpstat;
	ulong hdrsz;
	off_t poffset;
	int nhdrs, i, psinfo_fd, lwpstat_fd, lwpinfo_fd, prstat_fd, prcred_fd;
	int size, count, ncount;
	char cname[PATH_MAX],prstatname[PATH_MAX],prcredname[PATH_MAX];
	char psinfoname[PATH_MAX],lwpstatname[PATH_MAX],lwpinfoname[PATH_MAX];
	struct stat statbuf;
	char * bp = (char *)&buf[0];	/* pointer to big buffer */
	prmap_t *pdp = (prmap_t *)bp;
	int ret, index = 0, lwp_i;;

	/*
	 * Fetch the memory map and look for text, data, and stack.
	 */
	sprintf(mapname, "%s/%d/map", pdir, pid); 
	if (stat(mapname, &statbuf) == -1) {
		fprintf(stderr, "stat of /proc map file %s failed, errno %d\n",
			mapname, errno);
		return(-1);
	}

	if ((nsegments = (statbuf.st_size/sizeof(prmap_t))) <= 0) {
		fprintf(stderr,"dumpcore(): file %s contains no segments\n",
			mapname);
		return(-1);
	}

	if (nsegments >= (sizeof(buf)/sizeof(prmap_t))) {
		(void) fprintf(stderr, "dumpcore(): too many segments\n");
		return(-1);
	}

	if ((map_fd = openprocfile(mapname)) < 0) return err;

	for (i=0; i<nsegments; i++, pdp++)
		if ((ret = read(map_fd,pdp,sizeof(prmap_t))) != sizeof(prmap_t)){
			fprintf(stderr, "dumpcore(): read of map file failed: read return %d\n", ret);
			return(-1);
		}

	pdp = (prmap_t *) &buf[0]; /* reset pointer to prmap_t area */

	sprintf(psinfoname, "%s/%d/psinfo", pdir, pid); 
	if ((psinfo_fd = openprocfile(psinfoname)) < 0) return err;

	if ((ret = read(psinfo_fd, &psstat, sizeof(psinfo_t)))
		!= sizeof(psinfo_t)) {
		fprintf(stderr, "dumpcore(): read of psinfo file %s failed\n",
			psinfoname);
		(void) close(as_fd);
		(void) close(status_fd);
		(void) close(map_fd);
		(void) close(ctl_fd);
		return(-1);
	}

	/*
	 *  The total number of note sections we will need will be one more
	 *  than the number of LWP's in the process (one note section for
	 *  process-wide info and one note section per LWP).  Therefore,
	 *  the number of entries in the program header is the total number
	 *  of regions of memory that have different protections in the
	 *  address space plus the total number of note sections.
	 */
	nhdrs = nsegments + psstat.pr_nlwp + 1;
	hdrsz = nhdrs * sizeof(Elf32_Phdr);

	v = (Elf32_Phdr *)malloc(hdrsz);

	memset(&ehdr, 0, sizeof(Elf32_Ehdr));
	ehdr.e_ident[EI_MAG0] = ELFMAG0;
	ehdr.e_ident[EI_MAG1] = ELFMAG1;
	ehdr.e_ident[EI_MAG2] = ELFMAG2;
	ehdr.e_ident[EI_MAG3] = ELFMAG3;
	ehdr.e_ident[EI_CLASS] = ELFCLASS32;
	ehdr.e_ident[EI_DATA] = ELFDATA2LSB;
	ehdr.e_ident[EI_VERSION] = EV_CURRENT;

	ehdr.e_type = ET_CORE;		/* Machine Specific */
	ehdr.e_machine = EM_386;	/* Machine Specific */

        ehdr.e_version = EV_CURRENT;
        ehdr.e_phoff = sizeof(Elf32_Ehdr);
        ehdr.e_ehsize = sizeof(Elf32_Ehdr);
        ehdr.e_phentsize = sizeof(Elf32_Phdr);
        ehdr.e_phnum = nhdrs;

	/*
	 * Create the core dump file.
	 */
	(void) sprintf(cname, "%s.%d", filename, pid);
	if ((dfd = creat(cname, 0666)) < 0) {
		perror(cname);
		return(-1);
	}

	/*
	 * Write out elf header 
	 */
	if (write(dfd, &ehdr, sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr)) {
		perror("dumpcore(): write");
		return(-1);
	}	

	/*
	 *  Initialize the entry for process-wide note section
	 */
	poffset = sizeof(Elf32_Ehdr) + hdrsz;
	v[index].p_type = PT_NOTE;
        v[index].p_flags = PF_R;
        v[index].p_offset = poffset;
        v[index].p_filesz = (sizeof(Elf32_Note) * 4) + 
		roundup(sizeof(pstatus_t), sizeof(Elf32_Word)) +
		roundup(sizeof(psinfo_t), sizeof(Elf32_Word)) +
		roundup(sizeof(prcred_t), sizeof(Elf32_Word)) +
		roundup(sizeof(struct utsname), sizeof(Elf32_Word));
	poffset += v[index].p_filesz;
        index++;

	/*
	 * Initialize the program header entries for the per-lwp note sections.
	 */
	for (lwp_i = 0; lwp_i < psstat.pr_nlwp; lwp_i++) {
                v[index].p_type = PT_NOTE;
                v[index].p_flags = PF_R;
                v[index].p_offset = poffset;
                v[index].p_filesz =
                        (sizeof (Elf32_Note) * 2) +
                         roundup(sizeof (lwpstatus_t), sizeof (Elf32_Word)) +
                         roundup(sizeof (lwpsinfo_t), sizeof (Elf32_Word));

                poffset += v[index].p_filesz;
                index++;
	}

	/*
	 *  Initialize the program header information
	 */
	for (i = index; i < nhdrs; i++, pdp++) {
		vaddr_t naddr;		
		v[i].p_vaddr = (Elf32_Word) pdp->pr_vaddr;
		naddr = pdp->pr_vaddr;
		while ((naddr += PAGESIZE) < (pdp->pr_vaddr + pdp->pr_size))
		           ;
		size = naddr - pdp->pr_vaddr;
		v[i].p_memsz = size;
		if (pdp->pr_mflags & MA_WRITE)
			v[i].p_flags |= PF_W;
		if (pdp->pr_mflags & MA_READ)
			v[i].p_flags |= PF_R;
		if (pdp->pr_mflags & MA_EXEC)
			v[i].p_flags |= PF_X;
		if ((pdp->pr_mflags & (MA_WRITE|MA_EXEC)) != MA_EXEC) {
			v[i].p_offset = poffset;
			v[i].p_filesz = size;
			poffset += size;
		}	
	}

	/*
	 *  Write out program header
	 */
	if (write(dfd, v, hdrsz) != hdrsz) {
		perror("dumpcore(): write");
		return(-1);
	}	
	sprintf(prstatname,"%s/%d/status",pdir,pid);
	sprintf(prcredname,"%s/%d/cred",pdir,pid);
	if ((prstat_fd = openprocfile(prstatname)) < 0) return err;

	if ((prcred_fd = openprocfile(prcredname)) < 0) return err;
	
	if (read(prstat_fd, &prstat, sizeof(pstatus_t))!=sizeof(pstatus_t)) {
		fprintf(stderr, "dumpcore(): read of status file %s failed\n",
			prstatname);
		(void) close(as_fd);
		(void) close(status_fd);
		(void) close(map_fd);
		(void) close(ctl_fd);
		return -1;
        }
	if (read(prcred_fd, &prcred, sizeof(prcred_t))!=sizeof(prcred_t)) {
		fprintf(stderr, "dumpcore(): read of cred file %s failed\n",
			prcredname);
		(void) close(as_fd);
		(void) close(status_fd);
		(void) close(map_fd);
		(void) close(ctl_fd);
		return -1;
        }

	if (uname(&u_name) < 0)
		return(EINVAL);

	/*
	 * Write the note sections for the process-wide data 
	 *	(pstatus, psinfo, credentials, utsname)
	 */
	elfnote(dfd, CF_T_PRSTATUS, (char *)&prstat, sizeof(pstatus_t));
	elfnote(dfd, CF_T_PRPSINFO, (char *)&psstat, sizeof(psinfo_t));
	elfnote(dfd, CF_T_PRCRED, (char *)&prcred, sizeof(prcred_t));
	elfnote(dfd, CF_T_UTSNAME, (char *)&u_name, sizeof(struct utsname));

	/*
	 * Dump the note sections for the per-lwp data.
	 */
	for (lwp_i = 0; lwp_i < psstat.pr_nlwp; lwp_i++) {
	        sprintf(lwpstatname,"%s/%d/lwp/%d/lwpstatus",pdir,pid,lwp_i+1);
		if ((lwpstat_fd = openprocfile(lwpstatname)) < 0) return err;

		if ((ret = read(lwpstat_fd, &lwpstat, sizeof(lwpstatus_t)))
		    != sizeof(lwpstatus_t)) {
		        fprintf(stderr,"dumpcore(): read of lwpstatus file %s failed\n",
			lwpstatname);
			(void) close(as_fd);
			(void) close(status_fd);
			(void) close(map_fd);
			(void) close(ctl_fd);
			return(-1);
		}		       
	        sprintf(lwpinfoname, "%s/%d/lwp/%d/lwpsinfo",pdir,pid,lwp_i+1);
		if ((lwpinfo_fd = openprocfile(lwpinfoname)) < 0) return err;

		if ((ret = read(lwpinfo_fd, &lwpsinfo, sizeof(lwpsinfo_t)))
		    != sizeof(lwpsinfo_t)) {
		        fprintf(stderr,"dumpcore(): read of lwpsinfo file %s failed\n",
			lwpinfoname);
			(void) close(as_fd);
			(void) close(status_fd);
			(void) close(map_fd);
			(void) close(ctl_fd);
			return(-1);
		}		       
                elfnote(dfd,CF_T_LWPSTATUS,(char *)&lwpstat,sizeof(lwpstatus_t));
                elfnote(dfd,CF_T_LWPSINFO,(char *)&lwpsinfo,sizeof(lwpsinfo_t));
        }

	/*
	 * Dump data and stack
	 */
	for (i = 1; i<nhdrs; i++) {
		if (v[i].p_filesz == 0)
			continue;
		(void) lseek(pfd, v[i].p_vaddr, 0);
		count = (v[i].p_filesz > sizeof(buf)) ? 
						sizeof(buf) : v[i].p_filesz;
		while (count > 0) {
			if ((ncount = read(pfd, buf, count)) <= 0)
				break;
			(void) write(dfd, buf, ncount); 
			count -= ncount;
		}
	}
		
	(void) fprintf(stderr,"%s: %s.%d dumped\n", command, filename, pid);
	(void) close(dfd);
	return(0);
}


static int
elfnote(dfd, type, ptr, size)
	int dfd;
	int type;
	char *ptr;
	int size;
{
	Elf32_Note note;		/* ELF note */

	memset(&note, 0, sizeof(Elf32_Note)); 
	strcpy(note.name,"CORE");
	note.type = type;
	note.namesz = 8;
	note.descsz = roundup(size, sizeof(Elf32_Word));
	(void) write(dfd, &note, sizeof(Elf32_Note));
	(void) write(dfd, ptr, note.descsz);
}

 

static int
isprocdir(dir)	/* return TRUE iff dir is a PROC directory */
	char *dir;
{
	/* this is accomplished by doing a stat on the directory */
	/* and checking the st_fstype entry of the stat structure */
	/* to see if the file system type is "proc".              */

	struct stat stat1;	/* dir  */
	char * path = (char *)&buf[0];
	register char * p;

	/* make a copy of the directory name without trailing '/'s */
	if (dir == NULL)
		(void) strcpy(path, ".");
	else {
		(void) strcpy(path, dir);
		p = path + strlen(path);
		while (p > path && *--p == '/')
			*p = '\0';
		if (*path == '\0')
			(void) strcpy(path, ".");
	}

	/* stat the directory */
	if (stat(path, &stat1) != 0)
		return(FALSE);

	/* check to see if the directory is a "proc" directory */
	if (strcmp(stat1.st_fstype, "proc"))
		return(FALSE);		/* not a "proc" directory */
	else
		return(TRUE);		/* it is a "proc" directory */

}

static int
Pgrab(pdir, pid)		/* grab existing process */
	char *pdir;			/* /proc directory */
	register pid_t pid;		/* UNIX process ID */
{
	int err;

again:	/* Come back here if we lose it in the Window of Vulnerability */
	if (as_fd >= 0) {
		(void) close(as_fd);
		as_fd = -1;
	}
	if (status_fd >= 0) {
		(void) close(status_fd);
		status_fd = -1;
	}
	if (ctl_fd >= 0) {
		(void) close(ctl_fd);
		ctl_fd = -1;
	}

	/* generate the /proc/pid filename */
	(void) sprintf(asname, "%s/%d/as", pdir, pid);

	/* Request exclusive open to avoid grabbing someone else's	*/
	/* process and to prevent others from interfering afterwards.	*/
	if ((as_fd = open(asname, (O_RDWR|O_EXCL))) < 0) {
		switch (errno) {
		case EBUSY:
			return(G_BUSY);
		case ENOENT:
			return(G_NOPROC);
		case EACCES:
		case EPERM:
			return(G_PERM);
		default:
			perror("Pgrab open()");
			return(G_STRANGE);
		}
	}

	/* Make sure the filedescriptor is not one of 0, 1, or 2 */
	if (0 <= as_fd && as_fd <= 2) {
		int dfd = fcntl(as_fd, F_DUPFD, 3);

		(void) close(as_fd);
		if (dfd < 0) {
			perror("Pgrab fcntl()");
			return(G_STRANGE);
		}
		as_fd = dfd;
	}

	/* ---------------------------------------------------- */
	/* We are now in the Window of Vulnerability (WoV).	*/
	/* The process may exec() a setuid/setgid or unreadable	*/
	/* object file between the open() and the PCSTOP.	*/
	/* We will get EBADF in this case and must start over.	*/
	/* ---------------------------------------------------- */

	/*
	 * Get the process's status.
	 */
	(void) sprintf(statusname, "%s/%d/status", pdir, pid);
	if ((status_fd = openprocfile(statusname)) < 0) return err;

	if (read(status_fd, &prstat, sizeof(pstatus_t)) != sizeof(pstatus_t)) {
		fprintf(stderr, "Pgrab - read failed for status file%s\n", statusname);
		(void) close(as_fd);
		(void) close(status_fd);
		return(G_STRANGE);
	}

	/*
	 * If the process is a system process, we can't dump it.
	 */
	if (prstat.pr_flags & PR_ISSYS) {
		(void) close(as_fd);
		(void) close(status_fd);
		return(G_SYS);
	}

	/*
	 * We can't dump ourself.
	 */
	if (pid == getpid()) {
		/*
		 * Verify that the process is really ourself:
		 * Set a magic number, read it through the
		 * /proc file and see if the results match.
		 */
		long magic1 = 0;
		long magic2 = 2;

		if (lseek(as_fd, (long)&magic1, 0) == (long)&magic1
		 && read(as_fd, (char *)&magic2, sizeof(magic2)) == sizeof(magic2)
		 && magic2 == 0
		 && (magic1 = 0xfeedbeef)
		 && lseek(as_fd, (long)&magic1, 0) == (long)&magic1
		 && read(as_fd, (char *)&magic2, sizeof(magic2)) == sizeof(magic2)
		 && magic2 == 0xfeedbeef) {
			(void) close(as_fd);
			(void) close(status_fd);
			return(G_SELF);
		}
	}

	/*
	 * If the process is already stopped or has been directed
	 * to stop via /proc, there us nothing more to do.
	 */
	if (prstat.pr_lwp.pr_flags & (PR_ISTOP|PR_DSTOP))
		return(as_fd);

	(void) sprintf(ctlname, "%s/%d/ctl", pdir, pid);
	if ((ctl_fd = open(ctlname, O_WRONLY)) < 0) {
		switch (errno) {
		case EBUSY:
			err = G_BUSY;
			break;
		case ENOENT:
			err = G_NOPROC;
			break;
		case EACCES:
		case EPERM:
			err = G_PERM;
			break;
		default:
			perror("Pgrab open()");
			err = G_STRANGE;
		}
		(void) close(as_fd);
		(void) close(status_fd);
		return(err);
	}

	/*
	 * Mark the process run-on-last-close so
	 * it runs even if we die from SIGKILL.
	 */
	if (Ioctl(ctl_fd, PR_RLC, 1) == -1) {
		int rc;

		if (errno == EBADF)	/* WoV */
			goto again;

		if (errno == ENOENT)	/* Don't complain about zombies */
			rc = G_ZOMB;
		else {
			perror("Pgrab PR_RLC");
			rc = G_STRANGE;
		}
		(void) close(as_fd);
		(void) close(status_fd);
		return(rc);
	}
	
	/*
	 * Direct the process to stop.
	 * Set an alarm to avoid waiting forever.
	 */
	timeout = FALSE;
	err = 0;
	(void) alarm(2);
	if (Ioctl(ctl_fd, PCSTOP, 0) == 0)
		(void) alarm(0);
	else {
		err = errno;
		(void) alarm(0);
		if (err == EINTR
		 && timeout
		&& (lseek(status_fd, 0, 0) != -1)
		&& (read(status_fd, &prstat, sizeof(pstatus_t))
		    != sizeof(pstatus_t))) {
			fprintf(stderr, "Pgrab - read failed for status file %s\n",
				statusname);
			(void) close(as_fd);
			(void) close(status_fd);
			(void) close(ctl_fd);
			timeout = FALSE;
			err = errno;
		}
	}
	if (err) {
		int rc;

		switch (err) {
		case EBADF:		/* we lost control of the the process */
			goto again;
		case EINTR:		/* timeout or user typed DEL */
			rc = G_INTR;
			break;
		case ENOENT:
			rc = G_ZOMB;
			break;
		default:
			perror("Pgrab PCSTOP");
			rc = G_STRANGE;
			break;
		}
		if (!timeout || err != EINTR) {
			(void) close(as_fd);
			return(rc);
		}
	}

	/* re-read status file to ensure process is stopped */
	(void)lseek(status_fd, 0, 0); /* reset status file */
	if (read(status_fd, &prstat, sizeof(pstatus_t))
	   != sizeof(pstatus_t)) {
		fprintf(stderr, "Pgrab - re-read of status file %s failed\n",
		 statusname);
	}

	/*
	 * Process should either be stopped via /proc or
	 * there should be an outstanding stop directive.
	 */
	if ((prstat.pr_lwp.pr_flags & (PR_ISTOP|PR_DSTOP)) == 0) {
		(void) fprintf(stderr, "Pgrab: process is not stopped\n");
		(void) close(as_fd);
		return(G_STRANGE);
	}

	return(as_fd);
}

static int
Ioctl(fd, request, type)
	int fd;
	ulong request;
{

ulong_t cmd[2];
int len;

if (type == 1) {
	cmd[0] = PCSET;
	cmd[1] = request;
	len = 2*sizeof(long);
} else {
	cmd[0] = request;
	len = sizeof(long);
}
return((write(fd, cmd, len) == len) ? 0 : -1);


}

int
openprocfile(filename)
char *filename;
{
        int fdname = 0;
  
        err=0;
	if ((fdname = open(filename, O_RDONLY)) < 0) {
		switch (errno) {
		case EBUSY:
			err = (G_BUSY);
			break;
		case ENOENT:
			err = (G_NOPROC);
			break;
		case EACCES:
		case EPERM:
			err = (G_PERM);
			break;
		default:
			perror("Pgrab open()");
			err = (G_STRANGE);
		}
		(void) close(as_fd);
		(void) close(status_fd);
		(void) close(ctl_fd);
		return(-1);
	}
	return(fdname);
}
