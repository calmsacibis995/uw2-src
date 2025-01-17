/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*LINTLIBRARY*/
#ident	"@(#)libpkg:common/lib/libpkg/pkgtrans.c	1.15.20.31"
#ident "$Header: $"

#include <stdio.h>
#include <varargs.h>
#include <limits.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <wait.h>
#include <errno.h>
#include <dirent.h>
#include <signal.h>
#include <fcntl.h>
#include <pkginfo.h>
#include <pkgstrct.h>
#include <pkgtrans.h>
#include <pkgdev.h>

#include <pfmt.h>

extern int	errno;
extern char	*pkgdir;
extern FILE	*epopen();
extern char	**gpkglist(), *devattr(), *fpkginst();
extern void	free(),
		ecleanup(),
		progerr(),
		logerr(),
		rpterr(),
		ds_order();
extern int	getvol(),
		_getvol(),
		rrmdir(),
		mkdir(),
		chdir(),
		access(),
		epclose(),
		ckvolseq(),
		isdir(),
		ds_init(),
		ds_findpkg(),
		ds_getpkg(),
		fpkginfo(),
		esystem(),
		devtype(),
		pkgmount(),
		pkgumount(),
		ds_ginit(),
		ds_readbuf(),
		ds_close(),
		write(),
		creat(),
		dup(),
		close(),
		gpkgmap();

siginfo_t infop;

#define CMDSIZ	4096
#define LSIZE	512
#define TMPSIZ	64
#define PKGLEN	10

#define PKGINFO	"pkginfo"
#define PKGMAP	"pkgmap"
#define SETINFO	"setinfo"
#define INSTALL	"install"
#define RELOC	"reloc"
#define ROOT	"root"
#define CPIOPROC	"/usr/bin/cpio"
#define LSPROC		"/usr/bin/ls"

#define MSG_FSXFER \
	gettxt("uxpkgtools:653", "Transferring <%s> package instance to\n 	<%s> in file system format\n")
#define MSG_DSXFER \
	gettxt("uxpkgtools:654", "Transferring <%s> package instance to\n 	<%s> in datastream format\n")
#define MSG_IFSXFER \
	gettxt("uxpkgtools:655", "Transferring information files for <%s> package instance to\n 	<%s> in file system format\n")
#define MSG_IDSXFER \
	gettxt("uxpkgtools:656", "Transferring information files for <%s> package instance to\n 	<%s> in datastream format\n")
#define MSG_RENAME 	"uxpkgtools:657:\t... instance renamed <%s> on destination\n"
#define MSG_CORRUPT \
	"uxpkgtools:602:Volume is corrupt or is not part of the appropriate package."
#define MSG_SINFO \
	"uxpkgtools:658:use the -s option to transfer package in datastream format"

#define ERR_TRANSFER	"uxpkgtools:605:unable to complete package transfer"
#define ERR_FSFORMAT	"uxpkgtools:659:unable to transfer package to file system format"
#define ERR_REOPEN	"uxpkgtools:774:unable to reopen stdin"
#define MSG_PKGBAD	"uxpkgtools:417:PKG parameter is invalid <%s>"
#define MSG_PKGMTCH	"uxpkgtools:418:PKG parameter <%s> does not match instance <%s>"
#define MSG_NOPARAM	"uxpkgtools:416:%s parameter is not defined in <%s>"
#define MSG_SEQUENCE	"uxpkgtools:660:- volume is out of sequence"
#define MSG_MEM		"uxpkgtools:661:- no memory"
#define MSG_CMDFAIL	"uxpkgtools:606:- process <%s> failed, exit code %d"
#define MSG_CMDBIG	"uxpkgtools:662:- command line too big - <%s>"
#define MSG_POPEN	"uxpkgtools:609:- popen of <%s> failed, errno=%d"
#define MSG_PCLOSE	"uxpkgtools:611:- pclose of <%s> failed, errno=%d"
#define MSG_BADDEV	"uxpkgtools:663:- invalid or unknown device <%s>"
#define MSG_GETVOL	"uxpkgtools:664:- unable to obtain package volume"
#define MSG_NOSIZE 	"uxpkgtools:665:- unable to obtain maximum part size from pkgmap"
#define MSG_CHDIR	"uxpkgtools:666:- unable to change directory to <%s>"
#define MSG_FSTYP	"uxpkgtools:667:- unable to determine filesystem type for <%s>" 
#define MSG_NOTEMP	"uxpkgtools:668:- unable to create or use temporary directory <%s>"
#define MSG_SAMEDEV	"uxpkgtools:669:- source and destination represent the same device"
#define MSG_NOTMPFIL	"uxpkgtools:670:- unable to create or use temporary file <%s>"
#define MSG_NOPKGMAP	"uxpkgtools:671:- unable to open pkgmap for <%s>"
#define MSG_BADPKGINFO	"uxpkgtools:672:- unable to determine contents of pkginfo file"
#define MSG_NOPKGS	"uxpkgtools:673:- no packages were selected from <%s>"
#define MSG_MKDIR	"uxpkgtools:674:- unable to make directory <%s>"
#define MSG_NOEXISTS \
	"uxpkgtools:675:- package instance <%s> does not exist on source device"
#define MSG_EXISTS \
	"uxpkgtools:676:- no permission to overwrite existing path <%s>"
#define MSG_DUPVERS \
	"uxpkgtools:677:- identical version of <%s> already exists on destination device"
#define MSG_TWODSTREAM \
	"uxpkgtools:678:- both source and destination devices cannot be a datastream"
#define MSG_NOSPACE	"uxpkgtools:679:- not enough space on device"
#define MSG_HDRLARGE	"uxpkgtools:680:- datastream header line too large"
#define MSG_OPEN	"uxpkgtools:610:- open of <%s> failed, errno=%d"
#define MSG_FSEEK	"uxpkgtools:681:- fseek on pkgmap file failed"
#define MSG_DUP		"uxpkgtools:682:- could not duplicate file descriptor"

static struct pkgdev srcdev, dstdev;
static char	*tmpdir;
static char	*tmppath;
static char	dstinst[16];
static char 	*ids_name, *ods_name;
static char	*out_dev;
static int	ds_volcnt;
static int	ds_volno;
static void	(*func)();
static void	cleanup(), sigtrap(), t_cleanup();
static int	pkgxfer(), wdsheader(), ckoverwrite(), ckpkginst();
int		pkgtrans();
extern int	ds_fd; /* open file descriptor for data stream */
static int	setinfo = 0;	/* flag for when setinfo exists */

char	**xpkg;	/* array of transferred packages */
int	nxpkg;
char	*pkgmap = NULL; /*variable needed for ESD(Electronic Software Distribution */

static	char *allpkg[] = {
	"all",
	NULL
};

int
pkghead(device)
char	*device;
{
	char	*pt;
	int	n;

	cleanup();
	if(tmppath) {
		/* remove any previous tmppath stuff */
		rrmdir(tmppath);
		free(tmppath);
		tmppath = NULL;
	}

	if(device == NULL)
		return(0);
	else if((device[0] == '/') && !isdir(device)) {
		pkgdir = device;
		return(0);
	} else if((pt = devattr(device, "pathname")) && !isdir(pt)) {
		pkgdir = pt;
		return(0);
	}

	/* check for datastream */
	if(n=pkgtrans(device, (char *)0, allpkg, PT_SILENT|PT_INFO_ONLY))
		return(n);
		/* pkgtrans has set pkgdir */
	return(0);
}

static char *hdrbuf;
static char *pinput, *nextpinput;

static char *
mgets(buf, size)
char *buf;
int size;
{
	nextpinput = strchr(pinput, '\n');
	if(nextpinput == NULL) 
		return 0;
	*nextpinput = '\0';
	if((int)strlen(pinput) > size)
		return 0;
	(void)strncpy(buf, pinput, strlen(pinput));
	buf[strlen(pinput)] = '\0';
	pinput = nextpinput + 1;
	return buf;
}


/* will return 0, 1, 3, or 99 */
int
pkgtrans(device1, device2, pkg, options)
char	*device1, *device2, **pkg;
int	options;
{
	char	*src, *dst;
	int	errflg, i, n;
	char	path[PATH_MAX];
	struct stat statbuf;

	out_dev = device2;

	func = signal(SIGINT, sigtrap);

	/* transfer spool to appropriate device */
	if(devtype(device1, &srcdev)) {
		progerr(ERR_TRANSFER);
		logerr(MSG_BADDEV, device1);
		return(1);
	}
	srcdev.rdonly++;

	if ( device2 ) {
		if ( strcmp(device2,"-") == 0 )
			options |= PT_ODTSTREAM;

		if(devtype(device2, &dstdev)) {
			progerr(ERR_TRANSFER);
			logerr(MSG_BADDEV, device2);
			return(1);
		}
		if ( (srcdev.bdevice && dstdev.bdevice)  &&
			 !strcmp(srcdev.bdevice, dstdev.bdevice) ) {
			progerr(ERR_TRANSFER);
			logerr(MSG_SAMEDEV);
			return(1);
		}
	}

	/* check for datastream */
	ids_name = NULL;
	if(srcdev.bdevice) {
		if(n = _getvol(srcdev.bdevice, NULL, NULL, gettxt("uxpkgtools:137", "Insert %v into %p."), srcdev.norewind, 0)) {
			cleanup();
			if(n == 3)
				return(3);
			progerr(ERR_TRANSFER);
			logerr(MSG_GETVOL);
			return(1);
		}
		if(ds_readbuf(srcdev.cdevice))
			ids_name = srcdev.cdevice;
	}

	if(srcdev.cdevice && !srcdev.bdevice) 
		ids_name = srcdev.cdevice;
	else if(srcdev.pathname) {
		ids_name = srcdev.pathname;
		if(strcmp(srcdev.pathname, "-") && access(srcdev.pathname, 0) == -1) {
			progerr(ERR_TRANSFER);
			logerr(MSG_GETVOL);
			return(1);
		}
	}
		
	if(!ids_name && device2 == (char *)0) {
		if(n = pkgmount(&srcdev, NULL, 1, 0, 0)) {
			cleanup();
			return(n);
		}
		else if (srcdev.mount) {
			pkgdir = srcdev.mount;
			return(0);
		}
		return(1);
	}

	if(ids_name && device2 == (char *)0) {
		tmppath = tmpnam(NULL);
		tmppath = strdup(tmppath);
		if(tmppath == NULL) {
			progerr(ERR_TRANSFER);
			logerr(MSG_MEM);
			return(1);
		}
		if(mkdir(tmppath, 0755)) {
			progerr(ERR_TRANSFER);
			logerr(MSG_MKDIR, tmppath);
			return(1);
		}
		device2 = tmppath;
	}

	if(devtype(device2, &dstdev)) {
		progerr(ERR_TRANSFER);
		logerr(MSG_BADDEV, device2);
		return(1);
	}

	if((srcdev.cdevice && dstdev.cdevice) &&
	   !strcmp(srcdev.cdevice, dstdev.cdevice)) {
		progerr(ERR_TRANSFER);
		logerr(MSG_SAMEDEV);
		return(1);
	}

	ods_name = NULL;
	if(dstdev.cdevice && !dstdev.bdevice || dstdev.pathname)
		if(!(options & PT_ODTSTREAM)) {
			progerr(ERR_FSFORMAT);
			logerr(MSG_SINFO);
			return(1);
		}

	if(options & PT_ODTSTREAM) {
		if(!((ods_name = dstdev.cdevice) || (ods_name = dstdev.pathname))) {
			progerr(ERR_TRANSFER);
			logerr(MSG_BADDEV, device2);
			return(1);
		}
		if(ids_name) {
			progerr(ERR_TRANSFER);
			logerr(MSG_TWODSTREAM);
			return 1;
		}
	}

	if((srcdev.dirname && dstdev.dirname) &&
	!strcmp(srcdev.dirname, dstdev.dirname)) {
		progerr(ERR_TRANSFER);
		logerr(MSG_SAMEDEV);
		return(1);
	}

	if((srcdev.pathname && dstdev.pathname) &&
	!strcmp(srcdev.pathname, dstdev.pathname)) {
		progerr(ERR_TRANSFER);
		logerr(MSG_SAMEDEV);
		return(1);
	}

	if(ids_name) {
		if(srcdev.cdevice && !srcdev.bdevice && 
		(n = _getvol(srcdev.cdevice, NULL, NULL, NULL, srcdev.norewind, 0))) {
			cleanup();
			if(n == 3)
				return(3);
			progerr(ERR_TRANSFER);
			logerr(MSG_GETVOL);
			return(1);
		}
		if(srcdev.dirname = tmpnam(NULL)) 
			tmpdir = srcdev.dirname = strdup(srcdev.dirname);
		if((srcdev.dirname == NULL) || mkdir(srcdev.dirname, 0755) || 
		   chdir(srcdev.dirname)) {
			progerr(ERR_TRANSFER);
			logerr(MSG_NOTEMP, srcdev.dirname);
			cleanup();
			return(1);
		}
		if(strcmp(ids_name, "-") == 0) {
			if(!pkg || pkg[0] == NULL) {
				ds_fd = fcntl(0, F_DUPFD, 1);
				if(freopen("/dev/tty", "r", stdin) == NULL) {
					progerr(ERR_TRANSFER);
					logerr(ERR_REOPEN);
					cleanup();
					return(1);
				}
			}
		}
		if(ds_init(ids_name, pkg, srcdev.norewind)) {
			cleanup();
			return(1);
		}
	} else if(srcdev.mount) {
		if(n = pkgmount(&srcdev, NULL, 1, 0, 0)) {
			cleanup();
			return(n);
		}
	}

	src = srcdev.dirname;
	dst = dstdev.dirname;

	if(chdir(src)) {
		progerr(ERR_TRANSFER);
		logerr(MSG_CHDIR, src);
		cleanup();
		return(1);
	}

	xpkg = pkg = gpkglist(src, pkg, NULL);
	if(!pkg) {
		if(errno) {
			progerr(ERR_TRANSFER);
			logerr(MSG_NOPKGS, src);
			cleanup();
			return(1);
		} else {
			cleanup();
			return(0);
		}
	}
	for(nxpkg=0; pkg[nxpkg]; )
		nxpkg++; /* count */

	if(ids_name)
		ds_order(pkg); /* order requests */

	if(options & PT_ODTSTREAM) {
		char line[128];

		if(!dstdev.pathname && (n = _getvol(ods_name, NULL, NULL, NULL, dstdev.norewind, 1))) {
			cleanup();
			if(n == 3)
				return(3);
			progerr(ERR_TRANSFER);
			logerr(MSG_GETVOL);
			return(1);
		} 

		if(wdsheader(src, ods_name, pkg)) {
			cleanup();
			return(1);
		}
		ds_volno = 1; /* number of volumes in datastream */
		pinput = hdrbuf;
		/* skip past first line in header */
		(void)mgets(line, 128);
	}

	errflg = 0;
	
	for(i=0; pkg[i]; i++) {
		if(!(options & PT_ODTSTREAM) && dstdev.mount) {
			if(n = pkgmount(&dstdev, NULL, 0, 0, 1)) {
				cleanup();
				return(n);
			}
		}
		/*
		 * If this is a set installation package there should also
		 * be a "setinfo" file that must be placed on the package
		 * image.  If the setinfo flag was not already set do it here.
		 */
		(void) sprintf(path, "%s/%s/%s", src, pkg[i], SETINFO);
		if(stat(path, &statbuf) == 0)
			setinfo++;
		errflg = pkgxfer(pkg[i], options);
		setinfo = 0;	/* Always reset in case we incramented */
		if (errflg) {
			/*
			 * If pkgxfer fails, the diskette may still be
			 * mounted.  Unmount here.
			 */
			if(dstdev.mount && !ods_name)
				pkgumount(&dstdev);

			pkg[i] = NULL;
			if((options & PT_ODTSTREAM) || (errflg != 2))
				break;
		} else if(strcmp(dstinst, pkg[i]))
			pkg[i] = strdup(dstinst);
	}

	if(!(options & PT_ODTSTREAM) && dst)
		pkgdir = strdup(dst);

	cleanup();
	return(errflg);
}


static int
wdsheader(src, device, pkg)
char	*src, *device, **pkg;
{
	FILE	*fp;
	char	path[PATH_MAX], cmd[CMDSIZ];
	int	i, n, nparts, maxpsize;
	int partcnt, totsize;
	int hdrsize = 512;
	char *hp;
	struct stat statbuf;

	/*
	 * Set up 512 byte null padded area for
	 * datastream header section.
	 */
	if((hdrbuf = (char *)malloc(512)) == NULL) {
		progerr(ERR_TRANSFER);
		logerr(MSG_MEM);
		return(1);
	}	
	(void)memset(hdrbuf, '\0', 512);
	
	(void)ds_close(0);
 if(strcmp(device, "-") == 0)
		ds_fd = 1;
 else	if(dstdev.pathname)
		ds_fd = creat(device, 0644);
	else
		ds_fd = open(device, 1);
	if(ds_fd < 0) {
		progerr(ERR_TRANSFER);
		logerr(MSG_OPEN, device, errno);
		return(1);
	}	
	if(ds_ginit(device) < 0) {
		progerr(ERR_TRANSFER);
		logerr(MSG_OPEN, device, errno);
		(void)ds_close(0);
		return(1);
	}	
	nparts = maxpsize = 0;
	(void) sprintf(hdrbuf, "# PaCkAgE DaTaStReAm\n");
	hp = hdrbuf + strlen(hdrbuf);
	
	totsize = 0;
	for(i=0; pkg[i]; i++)  {
		(void) sprintf(path, "%s/%s/%s", src, pkg[i], PKGINFO);
		if(stat(path, &statbuf) < 0) {
			progerr(ERR_TRANSFER);
			logerr(MSG_BADPKGINFO);
			ecleanup();
			return(1);
		}
		totsize += statbuf.st_size/512 + 1;
	}

	/*
	 * totsize contains number of blocks used by header plus
	 * extra pkginfo files
	 */
	totsize += i/4 + 1;
	if(dstdev.capacity && totsize > dstdev.capacity) {
		progerr(ERR_TRANSFER);
		logerr(MSG_NOSPACE);
		ecleanup();
		return(1);
	}

	ds_volcnt = 1;		
	for(i=0; pkg[i]; i++) {
		partcnt = 0;
		(void) sprintf(path, "%s/%s/%s", src, pkg[i], PKGMAP);
		if((fp = fopen(path, "r")) == NULL) {
			progerr(ERR_TRANSFER);
			logerr(MSG_NOPKGMAP, pkg[i]);
			sighold(SIGINT);
			sigrelse(SIGINT);
			ecleanup();
			return(1);
		}
		if(fscanf(fp, ":%d%d", &nparts, &maxpsize) != 2) {
			progerr(ERR_TRANSFER);
			logerr(MSG_NOSIZE);
			(void) fclose(fp);
			ecleanup();
			return(1);
		}
		if(dstdev.capacity && maxpsize > dstdev.capacity) {
			progerr(ERR_TRANSFER);
			logerr(MSG_NOSPACE);
			(void) fclose(fp);
			ecleanup();
			return(1);
		}
			

		(void) sprintf(hp, "%s %d %d", pkg[i], nparts, maxpsize);
		totsize += nparts * maxpsize;
		if(dstdev.capacity && dstdev.capacity < totsize) { 
			int lastpartcnt = 0;
			/* if(i != 0) {
				progerr(ERR_TRANSFER);
				logerr(MSG_NOSPACE);
				(void) fclose(fp);
				ecleanup();
				return(1);
			} */
				
			if(totsize)
				totsize -= nparts * maxpsize;
			while(partcnt < nparts) {
				while(totsize <= dstdev.capacity && partcnt <= nparts) {
					totsize +=  maxpsize;
					partcnt++;
				}
				/* partcnt == 0 means skip to next volume */
				if(partcnt)
					partcnt--;
				(void) sprintf(hdrbuf + strlen(hdrbuf), " %d", partcnt - lastpartcnt);
				ds_volcnt++;
				totsize = 0;
				lastpartcnt = partcnt;
			}
			/* first parts/volume number does not count */
			ds_volcnt--;
		}
		(void) sprintf(hdrbuf + strlen(hdrbuf), "\n");

		if(strlen(hp) > (size_t)128) {
			progerr(ERR_TRANSFER);
			logerr(MSG_HDRLARGE);
			(void) fclose(fp);
			ecleanup();
			return(1);
		}

		if(strlen(hdrbuf) + 128 > (size_t)hdrsize) {
			if((hdrbuf = (char *)realloc(hdrbuf,  hdrsize + 512)) == NULL) {
				progerr(ERR_TRANSFER);
				logerr(MSG_MEM);
				(void) fclose(fp);
				ecleanup();
				return(1);
			}
			(void)memset(hdrbuf + hdrsize, '\0', 512);
			hdrsize += 512;
		}	
		hp = hdrbuf + strlen(hdrbuf); 		
		(void) fclose(fp);
	}
	sighold(SIGINT);
	sigrelse(SIGINT);

	if(strlen(hdrbuf) + 17 > (size_t)hdrsize) {
			
		if((hdrbuf = (char *)realloc(hdrbuf, hdrsize + 512)) == NULL) {
			progerr(ERR_TRANSFER);
			logerr(MSG_MEM);
			(void) fclose(fp);
			ecleanup();
			return(1);
		}
		(void)memset(hdrbuf + hdrsize, '\0', 512);
		hdrsize += 512;
	}	
	(void) sprintf(hdrbuf + strlen(hdrbuf), "# end of header\n");
	if(strlen(hdrbuf) < (size_t)(hdrsize-512))
		write(ds_fd, hdrbuf, hdrsize-512);
	else
		write(ds_fd, hdrbuf, hdrsize);

	/*
	 * write the first cpio() archive to the datastream
	 * which should contain the pkginfo & pkgmap files
	 * for all packages
	 */
	sprintf(cmd, "%s ", LSPROC);
	for(i=0; pkg[i]; i++) {
		if(strlen(cmd) + 70 > (size_t)CMDSIZ) {
			progerr(ERR_TRANSFER);
			logerr(MSG_CMDBIG, cmd);
			cleanup();
			return 1;
		}		
	
		(void) sprintf(cmd + strlen(cmd), "%s/%s ", pkg[i], PKGINFO);
		(void) sprintf(cmd + strlen(cmd), "%s/%s ", pkg[i], PKGMAP);

		/*
		 * If this is a set installation package there should also
		 * be a "setinfo" file that must be placed on the first cpio
		 * archive on the datastream. 
		 */
		(void) sprintf(path, "%s/%s/%s", src, pkg[i], SETINFO);
		if(stat(path, &statbuf) == 0)
			(void) sprintf(cmd + strlen(cmd), "%s/%s ", pkg[i], SETINFO);
	}
	if(strlen(cmd) + 30 > (size_t)CMDSIZ) {
		progerr(ERR_TRANSFER);
		logerr(MSG_CMDBIG, cmd);
		cleanup();
		return 1;
	}		
	(void) sprintf(cmd + strlen(cmd), " | %s -ocD -C 512", CPIOPROC);
	if(n = esystem(cmd, -1, ds_fd)) {
		rpterr();
		progerr(ERR_TRANSFER);
		logerr(MSG_CMDFAIL, cmd, n);
		cleanup();
		return(1);
	}
	return(0);
}

static int
ckoverwrite(dir, inst, options)
char	*dir;
char	*inst;
int	options;
{
	char	path[PATH_MAX];

	(void) sprintf(path, "%s/%s", dir, inst);
	if(access(path, 0) == 0) {
		if(options & PT_OVERWRITE)
			return(rrmdir(path));
		progerr(ERR_TRANSFER);
		logerr(MSG_EXISTS, path);
		return(1);
	}
	return(0);
}

static int
pkgxfer(srcinst, options)
char	*srcinst;
int	options;
{
	struct pkginfo info;
	FILE	*fp, *pp;
	char	*pt, *src, *dst, *t_pkgmap;
	char	dstdir[PATH_MAX],
		temp[PATH_MAX], 
		srcdir[PATH_MAX],
		dspath[PATH_MAX],
		cmd[CMDSIZ],
		pkgname[16],
		buf[512];
	int	i, n, part, nparts, maxpartsize, curpartcnt, xfd, fd1, fd2;
	char	volnos[128], tmpvol[128];
	struct cfent entry;

	info.pkginst = NULL; /* required initialization */

	/*
	 * when this routine is entered, the first part of
	 * the package to transfer is already available in
	 * the directory indicated by 'src' --- unless the
	 * source device is a datstream, in which case only
	 * the pkginfo and pkgmap files are available in 'src'
	 */
	src = srcdev.dirname;
	dst = dstdev.dirname;

	if(!(options & PT_SILENT))
		if(options & PT_ODTSTREAM) {
			/* if output datastream does not start with '/'
			 * then this is file to be placed under cwd
			 */
			(void) strcpy(dspath, ods_name);
			if(ods_name[0] != '/')
				sprintf(dspath, "%s/%s", src, ods_name);
			(void) fprintf(stdout, ((options & PT_INFO_ONLY) ? 
				MSG_IDSXFER : MSG_DSXFER), srcinst, dspath);
		} else {
			(void) fprintf(stdout, ((options & PT_INFO_ONLY) ? 
				MSG_IFSXFER : MSG_FSXFER), srcinst, out_dev);
		}
	(void) strcpy(dstinst, srcinst);

	/* check for the correct name of the package instance */
	if (ckpkginst(src, srcinst) != 0) {
		return(2);
	}

	if(!(options & PT_ODTSTREAM)) {
		/* destination is a (possibly mounted) directory */
		(void) sprintf(dstdir, "%s/%s", dst, dstinst);

		/*
		 * need to check destination directory to assure
		 * that we will not be duplicating a package which
		 * already resides there (though we are allowed to
		 * overwrite the same version)
		 */
		pkgdir = src;
		if(fpkginfo(&info, srcinst)) {
			progerr(ERR_TRANSFER);
			logerr(MSG_NOEXISTS, srcinst);
			(void) fpkginfo(&info, NULL);
			return(1);
		}
		pkgdir = dst;

		(void) strcpy(temp, srcinst);
		if(pt = strchr(temp, '.'))
			*pt = '\0';
		(void) strcat(temp, ".*");

		if(pt = fpkginst(temp, info.arch, info.version)) {
			/* the same instance already exists, although
			 * its pkgid might be different
			 */
			if(options & PT_OVERWRITE) {
				(void) strcpy(dstinst, pt);
				(void) sprintf(dstdir, "%s/%s", dst, dstinst);
			} else if(options & PT_RENAME) {
				/* 
				 * find next available instance by appending numbers
				 * to the package abbreviation until the instance
				 * does not exist in the destination directory
				 */
				if(pt = strchr(temp, '.'))
					*pt = '\0';
				for(i=2; (access(dstdir, 0) == 0); i++) {
					(void) sprintf(dstinst, "%s.%d", temp, i);
					(void) sprintf(dstdir, "%s/%s", dst, dstinst);
				}
			} else {
				progerr(ERR_TRANSFER);
				logerr(MSG_DUPVERS, srcinst);
				(void) fpkginfo(&info, NULL);
				(void) fpkginst(NULL);
				if(dstdev.mount) {
					(void) chdir("/");
					if(pkgumount(&dstdev))
						return(1);
				}
				return(2);
			}
		} else if(options & PT_RENAME) {
			/* 
			 * find next available instance by appending numbers
			 * to the package abbreviation until the instance
			 * does not exist in the destination directory
			 */
			if(pt = strchr(temp, '.'))
				*pt = '\0';
			for(i=2; (access(dstdir, 0) == 0); i++) {
				(void) sprintf(dstinst, "%s.%d", temp, i);
				(void) sprintf(dstdir, "%s/%s", dst, dstinst);
			}
		} else if(options & PT_OVERWRITE) {
			/* we're allowed to overwrite, but there seems
			 * to be no valid package to overwrite, and we are
			 * not allowed to rename the destination, so act
			 * as if we weren't given permission to overwrite
			 * --- this keeps us from removing a destination
			 * instance which is named the same as the source
			 * instance, but really reflects a different pkg!
			 */
			options &= (~PT_OVERWRITE);
		}
		(void) fpkginfo(&info, NULL);
		(void) fpkginst(NULL);

		if(ckoverwrite(dst, dstinst, options))
			return(2);

		if(isdir(dstdir) && mkdir(dstdir, 0755)) {
			progerr(ERR_TRANSFER);
			logerr(MSG_MKDIR, dstdir);
			return(1);
		}
	}

	if(!(options & PT_SILENT) && strcmp(dstinst, srcinst))
		(void) pfmt(stderr, MM_NOSTD, MSG_RENAME, dstinst);

	(void) sprintf(srcdir, "%s/%s", src, srcinst);
	if(chdir(srcdir)) {
		progerr(ERR_TRANSFER);
		logerr(MSG_CHDIR, srcdir);
		return(1);
	}

	if(ids_name) {	/* unpack the datastream into a directory */
		/*
		 * transfer pkginfo & pkgmap first
		 */
		(void) sprintf(cmd, "%s -pudm %s", CPIOPROC, dstdir);
		if((pp = epopen(cmd, "w")) == NULL) {
			rpterr();
			progerr(ERR_TRANSFER);
			logerr(MSG_POPEN, cmd, errno);
			return(1);
		}
		/*
		 * For the case of a set installation package, let's try
		 * and get a "setinfo" file off of the datastream.  If the
		 * this fails, let's reopen the pipe and try without a
		 * "setinfo" file.
		 */
		(void)fprintf(pp, "%s\n%s\n%s\n", PKGINFO, PKGMAP, SETINFO);
		sighold(SIGINT);
		if(epclose(pp)) {
			/*
			 * It could have failed because this package is not
			 * a set and therefore no "setinfo" file exists for
			 * this package.  Try again with no "setinfo".
			 */
			(void)fprintf(pp, "%s\n%s\n", PKGINFO, PKGMAP);
			if((pp = epopen(cmd, "w")) == NULL) {
				rpterr();
				progerr(ERR_TRANSFER);
				logerr(MSG_POPEN, cmd, errno);
				return(1);
			}
			if(epclose(pp)) {
				sigrelse(SIGINT);
				rpterr();
				progerr(ERR_TRANSFER);
				logerr(MSG_PCLOSE, cmd, errno);
				return(1);
			}
		}
		sigrelse(SIGINT);

		if(options & PT_INFO_ONLY) {
			if(dstdev.mount) {
				(void) chdir("/");
				if(pkgumount(&dstdev))
					return(1);
			}
			return(0); /* don't transfer objects */
		}

		if(chdir(dstdir)) {
			progerr(ERR_TRANSFER);
			logerr(MSG_CHDIR, dstdir);
			return(1);
		}

		/*
		 * for each part of the package, use cpio() to
		 * unpack the archive into the destination directory
		 */
		nparts = ds_findpkg(srcdev.cdevice, srcinst);
		if(nparts < 0) {
			progerr(ERR_TRANSFER);
			return(1);
		}
		for(part=1; part <= nparts;) {
			if(ds_getpkg(srcdev.cdevice, part, dstdir)) {
				progerr(ERR_TRANSFER);
				return(1);
			}
			part++;
			if(dstdev.mount) { 
				(void) chdir("/");
				if(pkgumount(&dstdev))
					return(1);
				if(part <= nparts) {
					if(n = pkgmount(&dstdev, NULL, part+1, 
					  nparts, 1))
						return(n);
					if(ckoverwrite(dst, dstinst, options))
						return(1);
					if(isdir(dstdir) && mkdir(dstdir, 0755)) {
						progerr(ERR_TRANSFER);
						logerr(MSG_MKDIR, dstdir);
						return(1);
					}
					/* 
					 * since volume is removable, each part
					 * must contain a duplicate of the 
					 * pkginfo file to properly identify the
					 * volume
					 */
					if(chdir(srcdir)) {
						progerr(ERR_TRANSFER);
						logerr(MSG_CHDIR, srcdir);
						return(1);
					}
					if((pp = epopen(cmd, "w")) == NULL) {
						rpterr();
						progerr(ERR_TRANSFER);
						logerr(MSG_POPEN, cmd, errno);
						return(1);
					}
					(void) fprintf(pp, "pkginfo");
					if(epclose(pp)) {
						rpterr();
						progerr(ERR_TRANSFER);
						logerr(MSG_PCLOSE, cmd, errno);
						return(1);
					}
					if(chdir(dstdir)) {
						progerr(ERR_TRANSFER);
						logerr(MSG_CHDIR, dstdir);
						return(1);
					}
				}
			}
		}
		return(0);
	}

	/* The next two lines are needed for ESD to
	 * use an alternative pkgmap, if specified, to create update
	 */
	if (pkgmap == NULL)	/* ESD */
		pkgmap = PKGMAP;	/* ESD */

	/* make a copy of the pkgmap file to be used
	 * later to get all files in the pkgmap
	 */
	t_pkgmap=tmpnam(NULL);
	t_pkgmap=strdup(t_pkgmap);
	if ((fd2 = open(t_pkgmap, (O_CREAT | O_WRONLY), 0644)) == -1) {
		progerr(ERR_TRANSFER);
		logerr(MSG_OPEN, t_pkgmap, errno);	
		return(1);
	}
	if ((fd1 = open(pkgmap, O_RDONLY)) == -1) {
		progerr(ERR_TRANSFER);
		logerr(MSG_OPEN, pkgmap, errno);	
		(void)close(fd2);
		(void)unlink(t_pkgmap);
		return(1);
	}
	while ((i=read(fd1, buf, 512)) > 0) {
		(void)write(fd2, buf, i);
	}
	(void)close(fd1);
	(void)close(fd2);
	

	/* 
	 * read nparts and maxpartsiz from pkgmap
	 */
	if((fp = fopen(t_pkgmap, "r")) == NULL) {
		progerr(ERR_TRANSFER);
		logerr(MSG_NOPKGMAP, srcinst);
		(void)unlink(t_pkgmap);
		return(1);
	}

	nparts = 1;
	if(fscanf(fp, ":%d%d", &nparts, &maxpartsize) != 2) {
		progerr(ERR_TRANSFER);
		logerr(MSG_NOSIZE);
		(void)t_cleanup(fp, t_pkgmap);
		return(1);
	}

	if(srcdev.mount) {
		if(ckvolseq(srcdir, 1, nparts, LOG)) {
			progerr(ERR_TRANSFER);
			logerr(MSG_SEQUENCE);
			(void)t_cleanup(fp, t_pkgmap);
			return(1);
		}
	}

	/* write each part of this package */
	if(options & PT_ODTSTREAM) {
		char line[128];
		(void)mgets(line, 128);
		curpartcnt = -1;
		if(sscanf(line, "%s %d %d %[ 0-9]", &pkgname, &nparts, &maxpartsize, volnos) == 4) {
			sscanf(volnos, "%d %[ 0-9]", &curpartcnt, tmpvol);
			strcpy(volnos, tmpvol);
		}
	}
	/* Don't need this field for pkgtrans, make sure it's zero */
	entry.pinfo = (struct pinfo *)0;
	for(part=1; part <= nparts; ) {
		long offset;
		if(curpartcnt == 0 && (options & PT_ODTSTREAM)) {
			char prompt[128];
			int index;
			ds_volno++;
			(void)ds_close(0);
			(void)sprintf(prompt, gettxt("uxpkgtools:616", "Insert %%v %d of %d into %%p"), ds_volno, ds_volcnt);
			if(n = getvol(ods_name, NULL, NULL, prompt)) {
				(void)t_cleanup(fp, t_pkgmap);
				return n;
			}
			if((ds_fd = open(dstdev.cdevice, 1)) < 0) {
				progerr(ERR_TRANSFER);
				logerr(MSG_OPEN, dstdev.cdevice, errno);
				(void)t_cleanup(fp, t_pkgmap);
				return 1;
			}
			if(ds_ginit(dstdev.cdevice) < 0) {
				progerr(ERR_TRANSFER);
				logerr(MSG_OPEN, dstdev.cdevice, errno);
				(void)ds_close(0);
				(void)t_cleanup(fp, t_pkgmap);
				return(1);
			}	
		
			(void)sscanf(volnos, "%d %[ 0-9]", &index, tmpvol);
			(void)strcpy(volnos, tmpvol);
			curpartcnt += index;
		}
			

		if(options & PT_INFO_ONLY)
			nparts = 0;

		/* if the output is to go to a datastream, redirect stdout
		 * to the datastream file descriptor
		 */
		if (options & PT_ODTSTREAM) {
			if(strcmp(ods_name, "-")) {
				if ((xfd=dup(1)) == -1) {
					progerr(ERR_TRANSFER);
					logerr(MSG_DUP);
					(void)t_cleanup(fp, t_pkgmap);
					return 1;
				}
				(void)close(1);
				if (fcntl(ds_fd, F_DUPFD, 1) == -1) {
					progerr(ERR_TRANSFER);
					logerr(MSG_DUP);
					(void)t_cleanup(fp, t_pkgmap);
					return 1;
				}
			}
			(void) sprintf(cmd, "%s -ocLD -C 512", CPIOPROC);
		} else 
			(void) sprintf(cmd, "%s -pdumL %s", CPIOPROC, dstdir);

		/* do a popen to pipe all files in the pkgmap to the 
		 * cpio command
		 */
		if ((pp = epopen(cmd, "w")) == NULL) {
			rpterr();
			progerr(ERR_TRANSFER);
			logerr(MSG_POPEN, cmd, errno);
			(void)t_cleanup(fp, t_pkgmap);
			return(1);
		}
			
		/* Send the pkginfo and pkgmap files to the pipe */
		if (part == 1) {

			/*
			 * In the case where this package instance represents
			 * a set installation package, then a setinfo file must
			 * also be sent down the pipe.
			 */
			if (setinfo) {
				(void) fprintf(pp, "%s\n%s\n%s\n", PKGINFO, PKGMAP, SETINFO);
			}
			else
				(void) fprintf(pp, "%s\n%s\n", PKGINFO, PKGMAP);
		}
		else
			fprintf(pp, "%s\n", PKGINFO);	


		if (nparts > 0) {
			while(gpkgmap(&entry, fp, 0) > 0) {
				if ((entry.volno != part) || (strchr("dxslcbpL", entry.ftype)))
					continue;
				if (entry.ftype == 'i') {
					if (!strcmp(entry.path, "pkginfo") || !strcmp(entry.path, "setinfo"))
						continue;
					fprintf(pp, "install/%s\n", entry.path);
				} else if (nparts > 1) {
					if (entry.path[0] == '/')
						fprintf(pp, "root.%d%s\n", 
							entry.volno, entry.path);
					else
						fprintf(pp, "reloc.%d/%s\n", 
							entry.volno, entry.path);
				} else {
					if (entry.path[0] == '/') {
						fprintf(pp, "root%s\n", entry.path);	
					}
					else
						fprintf(pp, "reloc/%s\n", entry.path);
				}
			}
		}

		/* seek to second line of the pkgmap file if
		 * the number of volumes is greater than the prsent
		 * volume
		 */
		offset=sizeof(nparts) + sizeof(maxpartsize) + 1;
		if (fseek(fp, offset, 0) != 0 ) {
			progerr(ERR_TRANSFER);
			logerr(MSG_FSEEK);
			(void)t_cleanup(fp, t_pkgmap);
			return 1;
		}

		/* if the output goes to a datastream, change file descriptor
		 * 1 back to stdout
		 */
		if ((options & PT_ODTSTREAM) && strcmp(ods_name, "-")) {
			(void)close(1);
			(void)dup(xfd);
			(void)close(xfd);
		}

		if (epclose(pp)) {
			rpterr();
			progerr(ERR_TRANSFER);
			logerr(MSG_PCLOSE, cmd, errno);
			(void)t_cleanup(fp, t_pkgmap);
			return(1);
		}

		part++;
		if (srcdev.mount && (nparts > 1) && ckvolseq(srcdir, part, nparts, NOLOG)) {
			/* loop until volume is mounted successfully */
			while(part <= nparts) {
				/* read only */

				/* unmount current source volume */
				(void) chdir("/");
				if(pkgumount(&srcdev)) {
					(void)t_cleanup(fp, t_pkgmap);
					return(1);
				}
				if(n = pkgmount(&srcdev, NULL, part, nparts, 1)) {
					(void)t_cleanup(fp, t_pkgmap);
					return(n);
				}
				if(chdir(srcdir)) {
					progerr(ERR_TRANSFER);
					logerr(MSG_CORRUPT, srcdir);
					(void) chdir("/");
					pkgumount(&srcdev);
					continue;
				}
				if(ckvolseq(srcdir, part, nparts, LOG)) {
					(void) chdir("/");
					pkgumount(&srcdev);
					continue;
				}
				break;
			}
		}
		if(!(options & PT_ODTSTREAM) && dstdev.mount) {
			/* unmount current volume */
			if(pkgumount(&dstdev)) {
				(void)t_cleanup(fp, t_pkgmap);
				return(1);
			}
			/* loop until next volume is mounted successfully */
			while(part <= nparts) {
				/* writable */
				if(n = pkgmount(&dstdev, NULL, part, nparts, 1)) {
					(void)t_cleanup(fp, t_pkgmap);
					return(n);
				}
				if(ckoverwrite(dst, dstinst, options))
					continue;
				if(isdir(dstdir) && mkdir(dstdir, 0755)) {
					progerr(ERR_TRANSFER);
					logerr(MSG_MKDIR, dstdir);
					continue;
				}
				break;
			}
		}

		if((options & PT_ODTSTREAM) && part <= nparts) {
			if(curpartcnt >= 0 && part > curpartcnt) {
				char prompt[128];
				int index;
				ds_volno++;
				if(ds_close(0)) {
					(void)t_cleanup(fp, t_pkgmap);
					return 1;
				}
				(void)sprintf(prompt, gettxt("uxpkgtools:616", "Insert %%v %d of %d into %%p"), ds_volno, ds_volcnt);
				if(n = getvol(ods_name, NULL, NULL, prompt))  {
					(void)t_cleanup(fp, t_pkgmap);
					return n;
				}
				if((ds_fd = open(dstdev.cdevice, 1)) < 0) {
					progerr(ERR_TRANSFER);
					logerr(MSG_OPEN, dstdev.cdevice, errno);
					(void)t_cleanup(fp, t_pkgmap);
					return 1;
				}
				if(ds_ginit(dstdev.cdevice) < 0) {
					progerr(ERR_TRANSFER);
					logerr(MSG_OPEN, dstdev.cdevice, errno);
					(void)ds_close(0);
					(void)t_cleanup(fp, t_pkgmap);
					return 1;
				}	
			
				(void)sscanf(volnos, "%d %[ 0-9]", &index, tmpvol);
				(void)strcpy(volnos, tmpvol);
				curpartcnt += index;
			}
		}

	}
	(void)t_cleanup(fp, t_pkgmap);
	return(0);
}

static void
sigtrap(signo)
int	signo;
{

	cleanup();

 	if(tmppath) { 
		rrmdir(tmppath);
		free(tmppath);
		tmppath = NULL;
	}
	if(func && (func != SIG_DFL) && (func != SIG_IGN))
		/* must have been an interrupt handler */
		(*func)(signo);
}

static void
cleanup()
{


	chdir("/");
 	if(tmpdir) { 
		rrmdir(tmpdir);
		free(tmpdir);
		tmpdir = NULL;
	}

	(void)ds_close(1);

	if(srcdev.mount && !ids_name)
		pkgumount(&srcdev);
	if(dstdev.mount && !ods_name)
		pkgumount(&dstdev);
}

static void
t_cleanup(fp, t_pkgmap)
FILE *fp;
char *t_pkgmap;
{
	(void)fclose(fp);
	(void)unlink(t_pkgmap);
}

static int
ckpkginst(srcdir, pkginst)
char *srcdir, *pkginst;
{
	FILE 	*fp;
	char	pkgabrv[PKGLEN],
		param[CMDSIZ], 
		path[PATH_MAX];
	int	pkgflg = 0;

	(void)sprintf(path, "%s/%s/%s", srcdir, pkginst, PKGINFO);
	if ((fp = fopen(path, "r")) == NULL) {
		progerr(ERR_TRANSFER);
		logerr(MSG_BADPKGINFO);
		return(1);
	}	
	while(fgets(param, CMDSIZ, fp) != NULL) {
		param[strlen(param) - 1] = '\0';
		if(!strcmp(strtok(param, "="), "PKG")) 
			if(strcpy(pkgabrv, strtok(NULL, "="))) {
				pkgflg++;
				break;
			}
	}

	if(!pkgflg) {
		progerr(ERR_TRANSFER);
		logerr(MSG_NOPARAM, "PKG", path);
		return(1);
	}

	if(pkgnmchk(pkgabrv, NULL, 0) || strchr(pkgabrv, '.')) {
		progerr(ERR_TRANSFER);
		logerr(MSG_PKGBAD, pkgabrv);
		return(1);
	}
	/*
	 * verify consistency between PKG parameter and pkginst that
	 * was determined from the directory structure
	 */
	(void) sprintf(param, "%s.*", pkgabrv);
	if(pkgnmchk(pkginst, param, 0)) {
		progerr(ERR_TRANSFER);
		logerr(MSG_PKGMTCH, pkgabrv, pkginst);
		return(1);
	}
	
	(void)fclose(fp);
	return(0);
}
