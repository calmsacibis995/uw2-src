/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libadm:common/lib/libadm/getvol.c	1.3.6.13"
#ident  "$Header: $"

#include <stdio.h>
#include <sys/fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <devmgmt.h>
#include <dirent.h>

#include <limits.h>
#include <locale.h>
#include <unistd.h>
#include <valtools.h>

#include <pfmt.h>

extern int	ckquit;

#define LABELSIZ	6
#define BELL	"\007"
#define	LABELIT	"/sbin/labelit"
#define	MKFS	"/sbin/mkfs"
#define	UFS	"ufs"
#define	S5	"s5"

#define FORMFS_MSG gettxt("uxadm:105", ",\\n\\ \\ or [%c] to format %s and place a filesystem on it")
#define FORMAT_MSG gettxt("uxadm:106", ",\\n\\ \\ or [%c] to format the %s")
#define WLABEL_MSG gettxt("uxadm:107", ",\\n\\ \\ or [%c] to write a new label on the %s")
#define OLABEL_MSG gettxt("uxadm:108", ",\\n\\ \\ or [%c] to use the current label anyway")
#define QUIT_MSG   gettxt("uxadm:109", ",\\n\\ \\ or [%c] to quit")

#define ERR_ACCESS	gettxt("uxadm:61", "\n%s (%s) cannot be accessed.\n")
#define ERR_FMT		gettxt("uxadm:62", "\nAttempt to format %s failed.\n")
#define ERR_MKFS	gettxt("uxadm:63", "\nAttempt to place filesystem on %s failed.\n")
#define	GO_STR		gettxt("uxadm:104", "go")

extern int	puttext(),
		ckstr(),
		ckkeywd();

static void	elabel(), labelerr(), doformat(), chose_fstype();
static int	wilabel(), ckilabel(), insert();
static char	*cdevice; 	/* character device name */
static char	*pname; 	/* device presentation name */
static char	*volume; 	/* volume name */
static char	fs_mkfs[BUFSIZ]; 	/* file system type */
static char	strval[16];
static char	*voltxt;	/* volume type */
static char origfsname[LABELSIZ+1];
static char origvolname[LABELSIZ+1];

/* Return:
 *	0 - okay, label matches
 *	1 - device not accessable
 *	2 - unknown device (devattr failed)
 *	3 - user selected quit
 *	4 - label does not match
 */

/* macros from labelit to behave correctly for tape
   is a kludge, should use devmgmt
*/
#ifdef RT
#define IFTAPE(s) (!strncmp(s,"/dev/mt",7)||!strncmp(s,"mt",2))
#define TAPENAMES "'/dev/mt'"
#else
#define IFTAPE(s) (!strncmp(s,"/dev/rmt",8)||!strncmp(s,"rmt",3)||!strncmp(s,"/dev/rtp",8)||!strncmp(s,"rtp",3))
#define TAPENAMES "'/dev/rmt' or '/dev/rtp'"
#endif

int	noprompt = 0;		/* if non-zero, don't call insert() */

getvol(device, label, options, prompt)
char	*device;
int	options;
char	*label, *prompt;
{
	/* The NDELAY flag is passed to prevent SCSI tapes from moving
	    ahead on close */
	return _getvol(device, label, options, prompt, (char *)0, O_RDONLY|O_NDELAY);
}

_getvol(device, label, options, prompt, norewind, rwflag)
char	*device;
int	options, rwflag;
char	*label, *prompt, *norewind;
{
	int	tmp;
	char	*advice, *pt;
	int	n, override;
	char	*fstype;

	cdevice = devattr(device, "cdevice");
	if((cdevice == NULL) || !cdevice[0]) {
		cdevice = devattr(device, "pathname");
		if((cdevice == NULL) || !cdevice)
			return(2);	/* bad device */
	}

	pname = devattr(device, "desc");
	if(pname == NULL) {
		pname = devattr(device, "alias");
		if(!pname)
			pname = device;
	}

	volume = devattr(device, "volume");

	if (label) {
		(void)strncpy(origfsname, label, LABELSIZ);
		origfsname[LABELSIZ] = '\0';
		if ((pt = strchr(origfsname, ',')) != NULL) {
			*pt = '\0';
		}
		if ((pt = strchr(label, ',')) != NULL) {
			(void)strncpy(origvolname, pt + 1, LABELSIZ);
			origvolname[LABELSIZ] = '\0';
		} else {
			origvolname[0] = '\0';
		}
	}
	
	override = 0;
	for(;;) {
		if (!(options & DM_BATCH) && volume && !noprompt) {
			n = insert(device, label, options, prompt);
			if (n < 0) {
				override++;
			} else if (n) {
				return(n);	/* input function failed */
			}
		}
		noprompt = 0;	/* reset so next prompt will be displayed */

		if ((tmp = open((norewind ? norewind : cdevice), rwflag)) < 0) {
			/* device was not accessible */
			if (options & DM_BATCH) {
				return(1);
			}
			(void)fprintf(stderr, ERR_ACCESS, pname, cdevice);
			if ((options & DM_BATCH) || (volume == NULL)) {
				return(1);
			}
			/* display advice on how to ready device */
			if (advice = devattr(device, "advice")) {
				(void) puttext(stderr, advice, 0, 0);
			}
			continue;
		}
		(void)close(tmp);

		/* check label on device */
		if (label) {
			if (options & DM_ELABEL) {
				elabel(label);
			} else {
				/*check internal label using labelit*/
				if(options & DM_BATCH) {
					if((fstype=devattr(cdevice,"fstype")) == NULL)
						fstype="ufs";
				} else {
					(void) chose_fstype(device,0);
					if(strval[0] == 'q')
						return(0);
				}		
				if(ckilabel(label, options, override,
					fstype)) {
					if ((options & DM_BATCH) || volume == NULL)
						return(4);
					continue;
				}
			}
		}
		break;
	}
	return(0);
}

static int
ckilabel(label, options, flag, fstype)
char	*label;
char	*fstype;
int	options, flag;
{
	FILE	*pp;
	char	*pt, *look, buffer[512], buffer2[512];
	char	fsname[LABELSIZ+1], volname[LABELSIZ+1];
	char	*pvolname, *pfsname;
	int	n, c;
	char	*lc_messages;

	(void)strncpy(fsname, label, LABELSIZ);
	fsname[LABELSIZ] = '\0';
	if (pt = strchr(fsname, ',')) {
		*pt = '\0';
	}
	if(pt = strchr(label, ',')) {
		(void) strncpy(volname, pt+1, LABELSIZ);
		volname[LABELSIZ] = '\0';
	} else {
		volname[0] = '\0';
	}
	
	(void)sprintf(buffer, "%s -F %s %s",
			LABELIT, fstype ? fstype : "ufs", cdevice);
	/*
	 * Force C locale, to allow the format of the input of
	 * labelit command to be parseable.
	 */
	lc_messages = setlocale(LC_MESSAGES, NULL);

	(void)putenv("LC_MESSAGES=C");
	pp = popen(buffer, "r");
	pt = buffer2;
	while ((c = getc(pp)) != EOF) {
		*pt++ = (char)c;
	}
	*pt = '\0';
	/*(void)pclose(pp);
	if (fstype) {
		free(fstype);
	}*/

	/*
	 * Restore gettxt locale variable if necessary.
	 */
	if (lc_messages != NULL) {
		(void)setlocale(LC_MESSAGES, lc_messages);
	}

	pt = buffer2;
	pfsname = pvolname = NULL;
	if(strcmp(fstype,"ufs")== 0 ){
	  int nflg=0;
 	  while(*pt){
		if(nflg) {
			if(*pt=='f' && strncmp(pt,"fsname:",7) ==0){
				pt+=8;
				pfsname=pt;
				while(*pt&&*pt!='n')
					pt++;
				if(*pt=='n')
					*pt++='0';
			}
			if(*pt=='v' && strncmp(pt,"volume:",7) ==0){
				pt+=8;
				pvolname=pt;
				while(*pt&&*pt!='n')
					pt++;
				if(*pt=='n')
					*pt++='0';
			}
			nflg=0;
		}
		if(*pt=='n'){
			nflg++;
		}
		pt++;
	    } /* while */
	} 
	else
	{
	  look = "Current fsname: ";
	  n = strlen(look);
	  while (*pt) {
	     	if (!strncmp(pt, look, n)) {
			*pt = '\0';
			pt += strlen(look);
			if (pfsname == NULL) {
				pfsname = pt;
				look = ", Current volname: ";
				n = strlen(look);
			} else if (pvolname == NULL) {
				pvolname = pt;
				look = ", Blocks: ";
				n = strlen(look);
			} else 
				break;
		} else 
			pt++;
	  }
	  pt=pvolname;
	  while(*pt){
		if(*pt=='n'){
			*pt='0';
			break;
		}
		pt++;
	  }
	}
	if(pfsname==NULL || pvolname==NULL || 
 	  strcmp(fsname, pfsname) || strcmp(volname, pvolname)) {
		/* mismatched label */
		if (flag) {
			(void)sprintf(label, "%s,%s", pfsname, pvolname);
		} else {
			labelerr(pfsname==NULL?"":pfsname,
				 pvolname==NULL?"":pvolname);
			return(1);
		}
	}
	return(0);
}

static int
wilabel(label, device)
char *device;
char *label;
{
	char	buffer[512];
	char	fsname[LABELSIZ+1];
	char	volname[LABELSIZ+1];
	int	n;

	if (strcmp(strval, "ufs") && strcmp(strval, "s5")) {
		(void) chose_fstype(device, 0);
		if(strval[0] == 'q')
			return(0);
	}

	if (!label || !strlen(origfsname)) {
		if(n = ckstr(fsname, NULL, LABELSIZ, NULL, NULL, NULL, 
				gettxt("uxadm:076", "Enter text for fsname label:")))
			return(n);
	} else {
		strcpy(fsname, origfsname);
	}
	if (!label || !strlen(origvolname)) {
		if (n = ckstr(volname, NULL, LABELSIZ, NULL, NULL, NULL, 
				gettxt("uxadm:077", "Enter text for volume label:")))
			return(n);
	} else {
		strcpy(volname, origvolname);
	}

	if (IFTAPE(cdevice)) {
		(void) sprintf(buffer, "%s -F %s %s \"%s\" \"%s\" -n 1>&2", 
			LABELIT, strval, cdevice, fsname, volname);
	} else {
		(void) sprintf(buffer, "%s -F %s %s \"%s\" \"%s\" 1>&2", 
			LABELIT, strval, cdevice, fsname, volname);
	}
	if (system(buffer)) {
		(void) pfmt(stderr, MM_NOSTD,
			"uxadm:064:\nWrite of label to %s failed.", pname);
		strval[0] = '\0';
		return(1);
	}
	if (label) {
		(void) sprintf(label, "%s,%s", fsname, volname);
	}
	return(0);
}

static void
elabel()
{
}

static int
insert(device, label, options, prompt)
char *device, *label, *prompt;
int options;
{
	int	n;
	char	strval[16], prmpt[512];
	char	*pt, *keyword[5], *fmtcmd, *mkfscmd;
	char	*filetyp;

	voltxt = (volume ? volume : "volume");
	if(prompt) {
		(void) strcpy(prmpt, prompt);
		for(pt=prmpt; *prompt; ) {
			if((*prompt == '\\') && (prompt[1] == '%'))
				prompt++;
			else if(*prompt == '%') {
				switch(prompt[1]) {
				  case 'v':
					strcpy(pt, voltxt);
					break;

				  case 'p':
					(void) strcpy(pt, pname);
					break;

				  default:
					*pt = '\0';
					break;
				}
				pt = pt + strlen(pt);
				prompt += 2;
				continue;
			}
			*pt++ = *prompt++;
		}
		*pt = '\0';
	} else {
		(void) sprintf(prmpt, gettxt("uxadm:065", "Insert a %s into %s."),
				voltxt, pname);
		if(label && (options & DM_ELABEL)) {
			(void) sprintf(prmpt+strlen(prmpt), 
				gettxt("uxadm:078", " The following external label should appear on the %s:\\n\\t%s"),
				voltxt, label);
		}
		if(label && !(options & DM_ELABEL)) {
			(void) sprintf(prmpt+strlen(prmpt), 
			gettxt("uxadm:079", " The %s should be internally labeled as follows:\\n\\t%s\\n"),
				voltxt, label);
		}
	}

	pt = prompt = prmpt + strlen(prmpt);

	n = 0;
	pt += sprintf(pt, gettxt("uxadm:069", "\\nType [go] when ready"));
	keyword[n++] = GO_STR;

	if(options & DM_FORMFS) {
		if((fmtcmd = devattr(device, "fmtcmd")) && *fmtcmd &&
		  (mkfscmd = devattr(device, "mkfscmd")) && *mkfscmd) {
			pt += sprintf(pt, FORMFS_MSG, 'f', voltxt);
			keyword[n++] = "f";
		}
	} else if(options & DM_FORMAT) {
		if((fmtcmd = devattr(device, "fmtcmd")) && *fmtcmd) {
			pt += sprintf(pt, FORMAT_MSG, 'f', voltxt);
			keyword[n++] = "f";
		}
	}
	if(options & DM_WLABEL) {
		pt += sprintf(pt, WLABEL_MSG, 'w', voltxt);
		keyword[n++] = "w";
	}
	if(options & DM_OLABEL) {
		pt += sprintf(pt, OLABEL_MSG, 'o');
		keyword[n++] = "o";
	}
	keyword[n++] = NULL;
	if(ckquit)
		pt += sprintf(pt, QUIT_MSG, 'q');
	*pt++ = ':';
	*pt = '\0';

	pt = prmpt;
	fprintf(stderr, BELL);
	for(;;) {
		if (n = ckkeywd(strval, keyword, GO_STR, NULL, NULL, pt))
			return(n);

		pt = prompt; /* next prompt is only partial */
		if(!strcmp(strval, "f")) {
			doformat(voltxt, label, fmtcmd, mkfscmd, options, device);
			continue;
		} else if(!strcmp(strval, "w")) {
			(void) wilabel(label, device);
			continue;
		} else if(!strcmp(strval, "o"))
			return(-1);
		break;
	}
	return(0);
}

static void
doformat(voltxt, label, fmtcmd, mkfscmd, options, device)
char	*voltxt, *label, *fmtcmd, *mkfscmd, *device;
int options;
{
	char	buffer[512];

	(void) fprintf(stderr, "\t[%s]\n", fmtcmd);
	(void) sprintf(buffer, "(%s) 2>&2", fmtcmd);
	if(system(buffer)) {
		(void) fprintf(stderr, ERR_FMT, voltxt);
		return;
	}

	(void) chose_fstype(device, 1);
	if(strlen(fs_mkfs) > 0) {
		(void) fprintf(stderr, "\t[%s]\n", fs_mkfs);
		(void) sprintf(buffer, "(%s) 1>&2", fs_mkfs);
		if(system(buffer)) {
			(void) fprintf(stderr, ERR_MKFS, voltxt);
			return;
		}
	}
	else
		return;

	mkfscmd = NULL;
	fs_mkfs[0] = '\0';
	if (options != DM_FORMAT)
	   (void) wilabel(label, device);
}


static void
labelerr(fsname, volname)
char *fsname, *volname;
{
	(void) pfmt(stderr, MM_NOSTD, "uxadm:070:\nLabel incorrect.\n");
	if(volume)
		(void) pfmt(stderr, MM_NOSTD,
			"uxadm:080:The internal label on the inserted %s is\n\t%s,%s\n",
			volume, fsname, volname);
	else
		(void) pfmt(stderr, MM_NOSTD,
			"uxadm:081:The internal label for %s is\t%s,%s\n",
			pname, fsname, volname);
}

#define	FTYPNUMB	24
#define	ETCFS		"/etc/fs"

void
chose_fstype(char *device, int type)
{

	DIR 	*dirp;
	struct	dirent	*direntp;
	char	*capacity, *mkfscmd;
	char	*keyword[24];
	char	prmpt[512];
	char	*pt;
	char	validfs[PATH_MAX];
	int	n = 0;

	voltxt = (volume ? volume : "volume");
	if(type == 0)
		(void) sprintf(prmpt, gettxt("uxadm:082", "Enter file system type on %s in %s.\\n\\ "),
			voltxt, pname);
	else
		(void) sprintf(prmpt, gettxt("uxadm:083", "Select file system type to be created on %s in %s.\\n\\ "),
			voltxt, pname);
	capacity = devattr(cdevice, "capacity");
	if(capacity == NULL || !capacity[0]) {
		(void) pfmt(stderr, MM_NOSTD,
			"uxadm:075:could not determine capacity\n");
		return;
	}
	dirp = opendir(ETCFS);
	while((direntp = readdir(dirp)) != NULL) {
		if(strcmp(direntp->d_name, "s5") && strcmp(direntp->d_name, "ufs"))
			continue;
		/* 
		 * Valid file system types on which we can create packages will
		 * have an associated 'labelit' command in dir /etc/fs/<fstyp>.
		 */
		(void) sprintf(validfs, "%s/%s/labelit", ETCFS, direntp->d_name);
		if(access(validfs, X_OK) == 0) {
			keyword[n] = strdup(direntp->d_name);
			++n;
		}

	}
	keyword[n++] = NULL;
	closedir(dirp);
	for(;;) {
		strval[0] = NULL;
		if(n = ckkeywd(strval, keyword, UFS, NULL, NULL, prmpt)) {
			fs_mkfs[0] = NULL;
			return;
		}
		/* Make sure value entered is valid fs type */
		for(n=0; keyword[n]; n++) {
			/* 
			 * UFS and S5 file system types are the only
			 * file system types upon which packages may
			 * be placed.
			 */
			if(!strcmp(strval, UFS)) {
				(void) sprintf(fs_mkfs, "%s -F %s %s %s 1024", 
					MKFS, strval, cdevice, capacity);
				return;
			}
			else if(!strcmp(strval, S5)) {
				mkfscmd = devattr(device, "mkfscmd");
				if (mkfscmd)
					(void) sprintf(fs_mkfs, "%s", mkfscmd);
				else
					(void) sprintf(fs_mkfs, "%s -F s5 %s 2370", MKFS, cdevice); 
				return;
			}
		}
	}
}

