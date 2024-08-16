/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pdi.cmds:mkdev.c	1.6.3.50"
#ident	"$Header: $"

/*
 * pdimkdev [-d tcindexfile] [-s] [-f] [-i] [-S]
 *
 * /etc/scsi/pdimkdev is a utility that creates device nodes for
 * equipped PDI devices. It determines the device equippage
 * by examining the edt. Since the device 
 * nodes that are created for each device are unique to that 
 * device type, template files are used to specify the device 
 * naming conventions. The location of the template files is
 * specified in a target controller index file which may be 
 * supplied as a command line argument. The display of a new
 * device can also be controlled by an argument.
 *
 * pdimkdev has been modified to set the level on all nodes
 * to SYS_PRIVATE.  This change should be OK on a non-sfs filesystem
 * since the attempt to set the level will be ignored.
 *
 */

#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/statfs.h>
#include	<ctype.h>
#include	<sys/errno.h>
#include	<sys/stat.h>
#include	<sys/signal.h>
#include	<sys/mkdev.h>
#include	<sys/fcntl.h>
#include	<sys/buf.h>
#include	<sys/vtoc.h>
#include	<search.h>
#include	<string.h>
#include 	<ftw.h>

#include	<devmgmt.h>
#include	<unistd.h>
#include	<libgen.h>
#include	<mac.h>
#include	<sys/vfstab.h>
#include	<sys/fdisk.h>
#include	<sys/sd01_ioctl.h>
#include	<sys/fs/s5param.h>
#include	<sys/fs/s5filsys.h>

#include	<sys/sysi86.h>
#include	<sys/scsi.h>
#include	<sys/sdi_edt.h>
#include	<sys/sdi.h>
#include	"edt_sort.h"
#include	"scsicomm.h"

#include	<locale.h>
#include	<pfmt.h>

#include	<mac.h>

#define NO_CHANGES_EXIT 1
#define	CMDNAME		"pdimkdev"
#define	MKDTABNAME	"pdimkdtab"
#define	TCINDEX		"/etc/scsi/tc.index"
#define LAST_BOOT_EDT	"/etc/scsi/pdi_edt"
#define TEMP_EDT	"/etc/scsi/tmp.edt"
#define INSTALLF_INPUT	"/etc/scsi/installf.input"
#define MASK		00644
#define BLANK		'-'
#define YES		1
#define NO		0
#define DEV_MODE	0600
#define DEV_UID		0	/* owner root */
#define DEV_GID		3	/* group sys */
#define DIRMODE		0775
#define	MAXLINE		256
#define MAXMAJOR	255
#define	MAXTCTYPE	32
#define INODECRITICAL	50
#define INODELOW	200
#define	BIG		017777777777L
#define	BIG_I		2000

/* definitions of template file fields */
#define KEY		0
#define MINOR		1
#define MODE		2
#define	BLOCK		3
#define CHAR		4
#define SA		5
#define RSA		6
#define NUMFIELDS	7 /* the total number of fields in the template file */

/* Type Definitions */
typedef	struct	stat		STAT;

typedef struct  tokens {
	char string[32];
	int  token;
} TOKENS_T;

typedef struct  devices {
	char string[32];
} DEVICE_T;

struct scsi_addr{
	unsigned int	sa_c;		/* controller occurrence        */
	unsigned int	sa_t;		/* target controller number     */
	unsigned int	sa_l;		/* logical unit number          */
	unsigned int	sa_b;		/* bus number                   */
	unsigned int	sa_n;		/* admin name unit number       */
};

typedef struct disk_info {
	int	disk_valid;
	major_t	disk_bmajor;
	major_t	disk_cmajor;
	int	disk_count;
	minor_t	disk_minor[2];
} disk_info_t;

extern int	errno;
extern void	error(), warning();
extern EDT *readxedt(int *);
extern void make_bnodes(char *, major_t, major_t, level_t, FILE *, char);

/* Static Variables */
static char	Pdimkdev = FALSE;
static char	Silent = FALSE;
static char	UpdateMode = FALSE;
static char	Cmdname[64];
static char	TCindex[128];
static FILE	*contents_fp = NULL;
static FILE	*TCindexfp;
static FILE	*last_edt_fp;
static FILE	*temp_edt_fp;
static char	SCSI_name[49];
static level_t level;
static char	*ES_attr = "";
static disk_info_t	disks;
int	Debug;

/* Token Definitions:
 *	To add a token, add a define below and update NUMTOKENS. Then
 *	add the string and token to the Tokens array that follows.
 */
#define MKDEV		0
#define TCTYPE		1
#define QUERY		2
#define POSTMSG		3
#define COMMENT		4
#define DATA		5
#define TCINQ		6
#define ALIAS		7
#define DGRP 		8
#define ATTR 		9
#define FSATTR 		10
#define DPATTR 		11
#define GENERIC		12
#define UNKNOWN		13
#define TCLEN		14
#define NUMTOKENS	15 /* Should be one beyond maximum number of tokens */
#define UNTOKEN		-2

TOKENS_T	Tokens[] = {
	"MKDEV",	MKDEV,
	"TCTYPE",	TCTYPE,
	"QUERY",	QUERY,
	"POSTMSG",	POSTMSG,
	"#",		COMMENT,
	"DATA",		DATA,
	"TCINQ",	TCINQ,
	"ALIAS",	ALIAS,
	"DGRP",		DGRP,
	"ATTR",		ATTR,
	"FSATTR",	FSATTR,
	"DPATTR",	DPATTR,
	"GENERIC",	GENERIC,	
	"UNKNOWN",	UNKNOWN,	
	"TCLEN",	TCLEN,	
};

DEVICE_T	Device_Types[] = {
	"Disk",			/*	ID_RANDOM	*/
	"Tape",			/*	ID_TAPE		*/
	"Printer",		/*	ID_PRINTER	*/
	"Processor",		/*	ID_PROCESSOR	*/
	"WORM",			/*	ID_WORM		*/
	"CD-ROM",			/*	ID_ROM		*/
	"Scanner",		/*	ID_SCANNER	*/
	"Optical",		/*	ID_OPTICAL	*/
	"Changer",		/*	ID_CHANGER	*/
	"Communication",	/*	ID_COMMUNICATION*/
};

struct HBA HBA[MAX_EXHAS];

struct DRVRinfo {
	int	valid;
	char	name[NAME_LEN];	/* driver name			*/
	struct drv_majors majors;	/* array of all major sets	*/
	int	subdevs;	/* number of subdevices per LU	*/
	int	lu_limit;
	int	lu_occur;	/* occurrence of the current LU */
	int	instances; /* how many of each of these exist */
} DRVRinfo[MAXTCTYPE];

void E333A_nodes(void);
int MakeDeviceTable(char *, char *, uchar_t *, uchar_t, char *);

#ifdef DPRINTF
void
PrintEDT(EDT *xedtptr, int edtcnt)
{
	int e;
	EDT *xedtptr2;
	fprintf(stderr,"driver\tha_slot\tSCSIbus\tTC\tnumlus\tmemaddr\tPDtype\tTCinq\n");
	for ( xedtptr2 = xedtptr, e = 0; e < edtcnt; e++, xedtptr2++ ) {
		fprintf(stderr,"%s\t%d\t%d\t%d\t%d\t0x%x\t%s\n",
			xedtptr2->xedt_drvname,
			xedtptr2->xedt_ctl,
			xedtptr2->xedt_bus,
			xedtptr2->xedt_target,
			xedtptr2->xedt_lun,
			xedtptr2->xedt_memaddr,
			xedtptr2->xedt_tcinquiry);
	}
}
#endif

struct nodes {
	dev_t	device;
	char	type;
};
typedef struct nodes nodes_t;
static int *block_majors, *char_majors, block_count, char_count;

/*
 * the RemoveMatchingNode routine is called from the search_and_remove routine.
 * It is used in a call to nftw() which will recursively descend a directory and
 * execute RemoveMatchingNode for each file in that directory tree.
 * It compares the major number of each file to a list of PDI major numbers.
 * Files matching those major numbers are removed that are not in the hash
 * table are removed.
 */
int 
RemoveMatchingNode(const char *name, const STAT *statbuf, int code, struct FTW *ftwbuf)
{
	char	found;
	major_t	maj;
	int	search_index;
	ENTRY	new_item;

	found = FALSE;
	switch(code) {
	case FTW_F:
		maj = major(statbuf->st_rdev);
		switch(statbuf->st_mode & S_IFMT) {
		case S_IFCHR:
#ifdef DEBUG
			fprintf(stderr,"char special %s found\n",name);
#endif
			for(search_index = 0; char_majors[search_index] >= 0; search_index++){
				if ( char_majors[search_index] == maj )
					found = TRUE;
			}
			break;
		case S_IFBLK:
#ifdef DEBUG
			fprintf(stderr,"block special %s found\n",name);
#endif
			for(search_index = 0; block_majors[search_index] >= 0; search_index++){
				if ( block_majors[search_index] == maj )
					found = TRUE;
			}
			break;
		default:
			return(0);
		}

		if (found) {
			new_item.key = (void *)name;
			if (hsearch(new_item,FIND) == NULL) {
#ifdef DEBUG
				fprintf(stderr, "unlinking %s\n", name);
#endif
				unlink(name);
			}
#ifdef DEBUG
			else
				fprintf(stderr, "leaving alone %s\n", name);
#endif
		}
	}
	return(0);
}

int
search_and_remove(char *path)
{
	return(nftw(path, RemoveMatchingNode, 4, FTW_PHYS));
}

void
remove_nodes(FILE *fp)
{
	char device[MAXFIELD], type, *result, found;
	int search_index, new_major, new_minor, count, index;
	dev_t	new_device;
	nodes_t *nodes, *node;
	ENTRY	new_item, *found_item;
	
	rewind(fp);

	count = 0;
	while (fscanf(fp,"%*[^\n]\n") != EOF) {
		count++;
	}

	nodes = (nodes_t *)calloc(count, sizeof(nodes_t));
	block_majors = (int *)calloc(count, sizeof(int));
	char_majors = (int *)calloc(count, sizeof(int));
	(void)hcreate(count);

	rewind(fp);
	for(search_index = 0; search_index < count; search_index++) {
		block_majors[search_index] = -1;
		char_majors[search_index] = -1;
	}

	block_count = char_count = index = 0;
	while (fscanf(fp,"%s %c %d %d %*[^\n]\n",device, &type, &new_major, &new_minor) != EOF) {
		if ( type == 'c' ) {
			found = FALSE;
			for(search_index = 0; char_majors[search_index] >= 0; search_index++){
				if ( char_majors[search_index] == new_major )
					found = TRUE;
			}
			if ( !found ) {
				char_majors[search_index] = new_major;
				char_count++;
			}
		} else {
			found = FALSE;
			for(search_index = 0; block_majors[search_index] >= 0; search_index++){
				if ( block_majors[search_index] == new_major )
					found = TRUE;
			}
			if ( !found ) {
				block_majors[search_index] = new_major;
				block_count++;
			}
		}
		
		new_item.key = strdup(device);
		node = &nodes[index++];
		node->device = makedev(new_major, new_minor);
		node->type = type;
		new_item.data = (void *)node;
		(void)hsearch(new_item, ENTER);
	}

#ifdef SEARCH_DEBUG
	for(search_index = 0; block_majors[search_index] >= 0; search_index++){
		printf("block_major=%d\n",block_majors[search_index]);
	}
	for(search_index = 0; char_majors[search_index] >= 0; search_index++){
		printf("char_major=%d\n",char_majors[search_index]);
	}
#endif
#ifdef SEARCH_DEBUG_PROMPT

	new_item.key = device;
	printf("enter a device name:");
	while (scanf("%s", device) != EOF) {
		if ((found_item = hsearch(new_item,FIND)) != NULL) {
			printf("found %s, type=%c, major=%d, minor=%d\n",
				found_item->key, ((nodes_t *)found_item->data)->type,
				major(((nodes_t *)found_item->data)->device),
				minor(((nodes_t *)found_item->data)->device));
		} else {
			printf("not found\n");
		}
		printf("enter a device name:");
	}
	printf("\n");
#endif
	search_and_remove("/dev");
}

void
before_exit(int remove_nodes_flag)
{
	char system_line[MAXLINE];
	long	length;
	struct stat stat_buf;

	if ( Pdimkdev ) {

		E333A_nodes(); /* last ditch attempt to make basic nodes */

		if (contents_fp) {
			length = ftell(contents_fp);
			if ( length && remove_nodes_flag ) {
				remove_nodes(contents_fp);
			}

			fclose(contents_fp);
		}

		if ( UpdateMode )
			(void)unlink(INSTALLF_INPUT);

	} else if ( stat(INSTALLF_INPUT, &stat_buf) == 0 ) {
#ifndef DEBUG
		sprintf(system_line,"installf base - >/dev/null 2>&1 < %s",INSTALLF_INPUT);
#else
		sprintf(system_line,"installf base - < %s",INSTALLF_INPUT);
#endif
		(void)system(system_line);
		(void)system("installf -f base");
#ifndef DEBUG
		(void)unlink(INSTALLF_INPUT);
#endif
	}
}


/*
 * This routine writes a scsi address string to the location provided.
 */
void
scsi_name(struct scsi_addr *sa, char *message)
{
	if (sa == NULL) {
		sprintf(message, "");
	} else {
		/* occurrence based addressing */
		if (sa->sa_b)
			sprintf(message, "HA %u BUS %u TC %u LU %u",
				sa->sa_c, sa->sa_b, sa->sa_t, sa->sa_l);
		else
			sprintf(message, "HA %u TC %u LU %u",
				sa->sa_c, sa->sa_t, sa->sa_l);
	}
}

/*
 * Check for the existence and correctness of a special device. Returns TRUE if
 *	the file exists and has the correct major and minor numbers
 *	otherwise it returns FALSE, warns for error.
 */
int
SpecialExists(int type, char *path, major_t dev_major, minor_t dev_minor)
{
	STAT	statbuf;

	if (lstat(path,&statbuf) < 0) {
		/* 
		 * errno == ENOENT if file doesn't exist.
		 * Otherwise exit because something else is wrong.
		 */
		if (errno != ENOENT) {
			warning(":396:stat failed for %s\n", path);
			return(TRUE);
		} else {/* file does not exist */
			return(FALSE);
		}
	}

	/* file exists */

	statbuf.st_mode &= S_IFMT;

	/* do it's major and minor numbers, match the one we need */

	if ( ((statbuf.st_mode & S_IFCHR) == S_IFCHR) && type == S_IFCHR ) {
		if (major(statbuf.st_rdev) == dev_major && minor(statbuf.st_rdev) == dev_minor) {
			return(TRUE);
		}
	} else if ( ((statbuf.st_mode & S_IFBLK) == S_IFBLK) && type == S_IFBLK ) {
		if (major(statbuf.st_rdev) == dev_major && minor(statbuf.st_rdev) == dev_minor) {
			return(TRUE);
		}
	}

	return(FALSE);
}


/*
 * Check for the existence of a file. Returns TRUE if the file exists,
 * returns FALSE if the file doesn't exist, warns for error.
 */
int
FileExists(char *path)
{
	STAT	statbuf;

	if (stat(path,&statbuf) < 0) {
		/* 
		 * errno == ENOENT if file doesn't exist.
		 * Otherwise exit because something else is wrong.
		 */
		if (errno != ENOENT) {
			warning(":396:stat failed for %s\n", path);
			return(TRUE);
		}
		else /* file does not exist */
			return(FALSE);
	}

	/* file exists */
	return(TRUE);
}


/* GetToken() - reads the SCSI template file and returns the token found */
int
GetToken(FILE *templatefp)
{
	char	token[MAXLINE];
	int	curtoken;

	/* Read the next token from the template file */
	switch (fscanf(templatefp,"%s",token)) {
	case EOF :
		return(EOF);
	case 1	: /* normal return */
		break;
	default :
		return(UNKNOWN);
		/*NOTREACHED*/
		break;
	}

	/* Determine which token */
	for (curtoken = 0; curtoken < NUMTOKENS; curtoken++) {
		if (strcmp(Tokens[curtoken].string,token) == 0) {
			return(Tokens[curtoken].token);
		}
	}

	/* allow comment tokens to not be separated by white space from text */
	if (strncmp(Tokens[COMMENT].string,token,strlen(Tokens[COMMENT].string)) == 0)
		return(Tokens[COMMENT].token);

	/* Token not found */
	return(UNTOKEN);
}	/* GetToken() */


FILE *
OpenTemplate(uchar_t tc_pdtype, uchar_t *tcinq)
{
	register int	tctype_match, templatefound, tokenindex, i;
	FILE	*templatefp;
	int	end;
	char	templatefile[MAXFIELD], tctypestring[MAXFIELD], gfile[MAXFIELD];
	char	tcinqstring[INQ_EXLEN], tctypetoken[MAXFIELD];
	int	num_pdtype;
	int	gflag, tcinqlen;
	unsigned char	generic_type;
	TOKENS_T	Generics[] = {
		"NO_DEV",	ID_NODEV,
		"RANDOM",	ID_RANDOM,
		"TAPE",		ID_TAPE,
		"PRINTER",	ID_PRINTER,
		"PROCESSOR",	ID_PROCESOR,
		"WORM",		ID_WORM,
		"ROM",		ID_ROM,
		"SCANNER",	ID_SCANNER,
		"OPTICAL",	ID_OPTICAL,
		"CHANGER",	ID_CHANGER,
		"COMMUNICATION",	ID_COMMUNICATION,
	};

	templatefile[0] = '\0';
	tctypestring[0] = '\0';
	tcinqstring[0]  = '\0';
	tctypetoken[0]  = '\0';
	num_pdtype = 12;
	gflag = 0;
	gfile[0] = '\0';

	strcpy(tcinqstring, (char *)tcinq);
	/* pad inquiry string with blanks if necessary */
	if (strlen(tcinqstring) != (VID_LEN + PID_LEN + REV_LEN)) {
		end = strlen(tcinqstring);
		for(i = end; i < (VID_LEN + PID_LEN + REV_LEN); i++)
			tcinqstring[i] = ' ';
		tcinqstring[VID_LEN + PID_LEN + REV_LEN] = '\0';
	}
	tctype_match = FALSE;
	templatefound = FALSE;
	generic_type = UNKNOWN;
	tcinqlen = PID_LEN;

	/* start at the beginning of the tcindex file */
	rewind(TCindexfp);
	while (!templatefound) {
		tokenindex = GetToken(TCindexfp);
		switch (tokenindex) {
		case TCLEN:
			if (fscanf(TCindexfp," %d\n",&tcinqlen) == EOF) {
				errno = 0;
				warning(":397:Format of the target controller index file: %s\n", TCindex);
				return(NULL);
			}
			tcinqlen -= VID_LEN;
			if ( tcinqlen < 1 )
				tcinqlen = PID_LEN;
			break;

		/* A new TC INQuiry token was added to allow the TC's inquiry 
		 * string to specify the devices template file.
		 */
		case TCINQ:
			if (fscanf(TCindexfp," %[^\n]\n",tctypetoken) == EOF) {
				errno = 0;
				warning(":397:Format of the target controller index file: %s\n", TCindex);
				return(NULL);
			}
			/* pad inquiry string with blanks if necessary */
			if ((end = strlen(tctypetoken)) != (VID_LEN + PID_LEN)) {
				for(i = end; i < (VID_LEN + PID_LEN); i++)
					tctypetoken[i] = ' ';
				tctypetoken[VID_LEN + PID_LEN] = '\0';
			}
			/* Compares given length of vendor and product names */
			if (strncmp(&tctypetoken[0 + VID_LEN],&tcinqstring[0 + VID_LEN],
			   tcinqlen) == 0)
				tctype_match = TRUE;

			tcinqlen = PID_LEN;
			break;

		case MKDEV:
			if (tctype_match) {
				if(fscanf(TCindexfp," %[^\n]\n",templatefile) == EOF) {
					errno = 0;
					warning(":397:Format of the target controller index file: %s\n", TCindex);
					return(NULL);
				}
				templatefound = TRUE;
			}
			else {
				if (gflag) {
					if(fscanf(TCindexfp," %[^\n]\n",gfile) == EOF) {
						errno = 0;
						warning(":397:Format of the target controller index file: %s\n", TCindex);
						return(NULL);
					}
					gflag--;
				}
				else 
					/* read the remainder of the input line */
					fscanf(TCindexfp,"%*[^\n]%*[\n]");
			}
			break;
		case GENERIC:
			if (!tctype_match) {
				if(fscanf(TCindexfp," %[^\n]\n",tctypestring) == EOF) {
					errno = 0;
					warning(":397:Format of the target controller index file: %s\n", TCindex);
					return(NULL);
				}
				for (i=0; i < num_pdtype; i++) 
					if (strcmp(Generics[i].string,tctypestring) == 0) {
						generic_type = Generics[i].token;
						break;
					}
				if (tc_pdtype == generic_type)
					gflag++;
			}
			break;
		case EOF:
			if (!tctype_match && gfile[0] != '\0' ) {
				strcpy (templatefile, gfile);
				templatefound = TRUE;
				break;
			}
			else {
				errno = 0;
				warning(":398:TC entry not found in %s for \"%s\".\n", TCindex, tcinq);
				return(NULL);
			}
		case COMMENT:
		case UNTOKEN:
		case UNKNOWN:
		default:       /* read the remainder of the input line */
			fscanf(TCindexfp,"%*[^\n]%*[\n]");
			break;
		}
	}
	/* Check to see that the template file exists. */
	if (!FileExists(templatefile)) {
		warning(":399:template file %s does not exist.\n", templatefile);
		return(NULL);
	}
	/* Open the target controller template file. */
	if ((templatefp = fopen(templatefile,"r")) == NULL) 
		warning(":400:Could not open %s.\n", templatefile);
	
	return(templatefp);
}

/*
 * Convert a numeric string arg to binary				
 * Arg:	string - pointer to command arg					
 *									
 * Always presume that operators and operands alternate.		
 * Valid forms:	123 | 123*123 | 123+123 | L*16+12			
 * Return:	converted number					
 */									
unsigned int
CalculateMinorNum(char *token, struct scsi_addr *sa, int drvr, EDT *edtptr)
{
/*
 * The BIG parameter is machine dependent.  It should be a long integer
 * constant that can be used by the number parser to check the validity	
 * of numeric parameters.  On 16-bit machines, it should probably be	
 * the maximum unsigned integer, 0177777L.  On 32-bit machines where	
 * longs are the same size as ints, the maximum signed integer is more	
 * appropriate.  This value is 017777777777L.				
 */
	register char *cs;
	long n;
	long cut = BIG / 10;	/* limit to avoid overflow */

	cs = token;
	n = 0;
	/* check for operand */
	switch (*cs) {
	case 'C':
		n = sa->sa_c;
		cs++;
		break;
	case 'B':
		n = sa->sa_b;
		cs++;
		break;
	case 'T':
		n = sa->sa_t;
		cs++;
		break;
	case 'L':
		n = sa->sa_l;
		cs++;
		break;
	case 'D':
		n = DRVRinfo[drvr].lu_occur;
		cs++;
		break;
	case 'S':
		n = DRVRinfo[drvr].subdevs;
		cs++;
		break;
	case 'M':
		n = edtptr->xedt_first_minor;
		cs++;
		break;
	case 'P':
		n = SDI_MINOR(sa->sa_c, sa->sa_t, sa->sa_l, sa->sa_b);
		cs++;
		break;
	default:
		while ((*cs >= '0') && (*cs <= '9') && (n <= cut))
			n = n*10 + *cs++ - '0';
	}

	/* then check for the subsequent operator */
	switch (*cs++) {

	case '+':
		n += CalculateMinorNum(cs,sa,drvr,edtptr);
		break;
	case '*':
	case 'x':
		n *= CalculateMinorNum(cs,sa,drvr,edtptr);
		break;

	/* End of string, check for a valid number */
	case '\0':
		if ((n > BIG) || (n < 0)) {
			before_exit(FALSE);
			errno = 0;
			scsi_name(sa, SCSI_name);
			error(":401:minor number out of range for %s\n", SCSI_name);
		}
		return(n);
		/*NOTREACHED*/
		break;

	default:
		before_exit(FALSE);
		errno = 0;
		error(":402:bad token in template file: \"%s\"\n", token);
		break;
	}

	if ((n > BIG) || (n < 0)) {
		before_exit(FALSE);
		errno = 0;
		scsi_name(sa, SCSI_name);
		error(":401:minor number out of range for %s\n", SCSI_name);
	}

	return(n);
	/*NOTREACHED*/
}

/*
 * Using the directory and the token, substitute the ha, tc, and lu
 * numbers for the corresponding key letters in the token and concatenate
 * with the directory name to make the full path name of the device.
 * Return 1 if successful, return zero and a null devname if not.
 */
int
MakeNodeName(char *devname, char *dir, char *token,struct scsi_addr *sa)
{
	int i;
	char c;

	devname[0] = '\0';
	if ((dir[0] == BLANK) || (token[0] == BLANK))
		return(0);

	i = 0;
	(void) strcat(devname,dir);
	while ( (c = token[i++]) != '\0') {

		switch (c) {

		case '\\':
			if ( token[i] != '\0' ) {
				c = token[i++];
				(void) sprintf(devname,"%s%c",devname,c);
			}
			break;
		case 'U':
			(void) sprintf(devname,"%s%d",devname,sa->sa_n-1);
			break;
		case 'N':
			(void) sprintf(devname,"%s%d",devname,sa->sa_n);
			break;
		case 'C':
			(void) sprintf(devname,"%s%d",devname,sa->sa_c);
			break;
		case 'B':
			(void) sprintf(devname,"%s%d",devname,sa->sa_b);
			break;
		case 'T':
			(void) sprintf(devname,"%s%d",devname,sa->sa_t);
			break;
		case 'L':
			(void) sprintf(devname,"%s%d",devname,sa->sa_l);
			break;
		default :
			(void) sprintf(devname,"%s%c",devname,c);
			break;
		}
	}
	return(1);
}

/* 
 * The CreateDirectory routine takes a directory path argument and creates
 * that directory if it does not yet exist. Error handling is not
 * performed since any errors in stat or mkdir that are ignored here
 * would be handled in the MakeDeviceNodes routine anyway.
 */
void
CreateDirectory(char *dir)
{
	STAT	statbuf;
	char	newdir[MAXFIELD], tmpdir[MAXFIELD];
	char	*newdirp, *tok;

	/* check to see if directory field is blank */
	if (dir[0] == BLANK)
		return;

	/* check to see if the directory already exists */
	if (stat(dir,&statbuf) == 0)
		return;
	
	/*
         * Now start at beginning of path and create each
	 * directory in turn.
	 */
	strcpy(newdir,dir);
	newdirp=newdir;
	strcpy(tmpdir,"");
	while( (tok=strtok(newdirp,"/")) != NULL) {
		newdirp=NULL; 		/* set to null for next call to strtok */
		strcat(tmpdir,"/");
		strcat(tmpdir,tok);
		if (stat(tmpdir, &statbuf) < 0) {
			mkdir(tmpdir,DIRMODE);
		}
	}
}


/*
 * The MakeDeviceNodes routine is called for every logical unit. 
 * It checks the free inodes and then makes the device nodes.
 */
void
MakeDeviceNodes(struct scsi_addr *sa, int drvr, EDT *edtptr)
{
	int	p, tokenindex;
	int		data_begin;
	minor_t		minornum;
	mode_t		modenum;
	char		field[NUMFIELDS][MAXFIELD], dir[NUMFIELDS][MAXFIELD];
			/*
			 * the devname array below is used to hold the block,
			 * character, SA, and rSA filenames for the device.
			 * The values in these fields correspond to the current
			 * line being examined in the device template file
			 */
	char		devname[NUMFIELDS][MAXFIELD];
	char		query[MAXLINE];
	char		tempmsg[MAXLINE], postmsg[MAXLINE], alias[MAXLINE];
	char		bdevlist[MAXLINE*2], cdevlist[MAXLINE*2], dtab_alias[MAXLINE];
	FILE		*templatefp;
	int		NodesCreated;
	int		good_name[NUMFIELDS];
	major_t tc_bmajor, tc_cmajor;
	char *strp, *endp;
	
	if (Debug) {
		scsi_name(sa, SCSI_name);
		printf("MakeDeviceNodes:%s occ=%d\n", SCSI_name, DRVRinfo[drvr].lu_occur);
	}

	postmsg[0]='\0';


	/* if this is an unknown tctype then simply return */
	if ((templatefp = OpenTemplate(edtptr->xedt_pdtype, edtptr->xedt_tcinquiry)) == NULL)
		return;

	data_begin=FALSE;
	while (!data_begin) {
		tokenindex = GetToken(templatefp);
		switch (tokenindex) {
		case QUERY:
			fscanf(templatefp, " %[^\n]\n", query);
			break;
		case POSTMSG:
			/*
			 * Copy token value to postmsg with "\n" character
			 * pairs converted to new line characters
			 */
			fscanf(templatefp, " %[^\n]\n", tempmsg);
			strp = tempmsg;
			while ((endp = strstr(strp, "\\n")) != NULL) {
				strncat(postmsg, strp, endp - strp);
				strcat(postmsg, "\n");
				strp = endp + 2;
			}
			strcat(postmsg, strp);
			break;
		case ALIAS:
			fscanf(templatefp, " %[^\n]\n", alias);
			break;
		case DATA:
			data_begin=TRUE;
			break;
		case EOF:
			(void) fclose(templatefp);
			return;
			/* NOTREACHED */
			break;
		default:
		case COMMENT:
			fscanf(templatefp, "%*[^\n]%*[\n]");
			break;
		}
	}

	/* 
	 * The next line contains the device directories in cols 4 to 7
	 */
	(void)ParseLine(dir,templatefp,NUMFIELDS);

	/*
	 * Create directories if they don't already exist.
	 */
	CreateDirectory(dir[BLOCK]);
	CreateDirectory(dir[CHAR]);
	CreateDirectory(dir[SA]);
	CreateDirectory(dir[RSA]);

	NodesCreated = FALSE;
	(void)sprintf(bdevlist,"%s=",DDB_BDEVLIST);
	(void)sprintf(cdevlist,"%s=",DDB_CDEVLIST);
	dtab_alias[0] = '\0';

	/* read input lines for each subdevice, then make the device nodes */
	while ((p = ParseLine(field,templatefp,NUMFIELDS)) != EOF) {

		if (p != NUMFIELDS) {
			break; /* corrupted template file */
		}

		minornum = CalculateMinorNum(field[MINOR],sa,drvr,edtptr);
		modenum = (int) strtol(field[MODE],(char **)NULL,8);

		/* generate the special device file names */
		good_name[BLOCK] = MakeNodeName(devname[BLOCK],dir[BLOCK],field[BLOCK],sa);
		good_name[CHAR] = MakeNodeName(devname[CHAR],dir[CHAR],field[CHAR],sa);
		good_name[SA] = MakeNodeName(devname[SA],dir[SA],field[SA],sa);
		good_name[RSA] = MakeNodeName(devname[RSA],dir[RSA],field[RSA],sa);

		if ( Pdimkdev || UpdateMode ) {
			tc_cmajor = DRVRinfo[drvr].majors.c_maj;
			tc_bmajor = DRVRinfo[drvr].majors.b_maj;
			
			if ( good_name[BLOCK] ) {
				if (!SpecialExists(S_IFBLK,devname[BLOCK],tc_bmajor,minornum)) {
					(void) unlink(devname[BLOCK]);
					if (mknod(devname[BLOCK],modenum | S_IFBLK,
						  makedev(tc_bmajor,minornum)) < 0) {
						warning(":403:mknod failed for %s.\n", devname[BLOCK]);
					} else {
						NodesCreated = TRUE;
						(void)chown(devname[BLOCK],(uid_t)DEV_UID,(gid_t)DEV_GID);
						(void)lvlfile(devname[BLOCK], MAC_SET, &level);
					}
				}
				fprintf(contents_fp,"%s b %d %d ? ? ? %d NULL NULL\n",devname[BLOCK],tc_bmajor,minornum,level);
			}
			
			if ( good_name[CHAR] ) {
				if (!SpecialExists(S_IFCHR,devname[CHAR],tc_cmajor,minornum)) {
					(void) unlink(devname[CHAR]);
					if (mknod(devname[CHAR],modenum | S_IFCHR,
						  makedev(tc_cmajor,minornum)) < 0) {
						warning(":403:mknod failed for %s.\n", devname[CHAR]);
					} else {
						NodesCreated = TRUE;
						(void)chown(devname[CHAR],(uid_t)DEV_UID,(gid_t)DEV_GID);
						(void)lvlfile(devname[CHAR], MAC_SET, &level);
					}
				}
				fprintf(contents_fp,"%s c %d %d ? ? ? %d NULL NULL\n",devname[CHAR],tc_cmajor,minornum,level);
			}
			
			/* link the system admin names if necessary */
			if ( good_name[SA] ) {
				if (!SpecialExists(S_IFBLK,devname[SA],tc_bmajor,minornum)) {
					(void) unlink(devname[SA]);
					if (link(devname[BLOCK],devname[SA]) < 0) {
						warning(":404:%s\n", devname[SA]);
					} else {
						NodesCreated = TRUE;
						chown(devname[BLOCK],(uid_t)DEV_UID,(gid_t)DEV_GID);
					}
				}
				fprintf(contents_fp,"%s b %d %d ? ? ? %d NULL NULL\n",devname[SA],tc_bmajor,minornum,level);
			}
			
			if ( good_name[RSA] ) {
				if (!SpecialExists(S_IFCHR,devname[RSA],tc_cmajor,minornum)) {
					(void) unlink(devname[RSA]);
					if (link(devname[CHAR],devname[RSA]) < 0) {
						warning(":404:%s\n", devname[SA]);
					} else {
						NodesCreated = TRUE;
						chown(devname[CHAR],(uid_t)DEV_UID,(gid_t)DEV_GID);
					}
				}
				fprintf(contents_fp,"%s c %d %d ? ? ? %d NULL NULL\n",devname[RSA],tc_cmajor,minornum,level);
			}
		}

		if ( !Pdimkdev || UpdateMode ) {
			if (strchr(field[KEY], 'O') != NULL) {
				/* Make device table entry.  Also make
				 * entries for disk slices for disk devices.
				 */
				(void)MakeDeviceTable(devname[BLOCK], devname[CHAR], edtptr->xedt_tcinquiry, edtptr->xedt_pdtype, dtab_alias );
			} else if (strcmp(alias, "disk") != 0) {
				if ( good_name[BLOCK] ) {
					strcat(bdevlist, devname[BLOCK]);
					strcat(bdevlist, ",");
				}
				if ( good_name[CHAR] ) {
					strcat(cdevlist, devname[CHAR]);
					strcat(cdevlist, ",");
				}
			}
			if ( good_name[SA] ) {
				strcat(bdevlist, devname[SA]);
				strcat(bdevlist, ",");
			}
			if ( good_name[RSA] ) {
				strcat(cdevlist, devname[RSA]);
				strcat(cdevlist, ",");
			}
		}

	} /* end of ParseLine while loop */

	if ( !Pdimkdev || UpdateMode ) {
		(void)UpdateDeviceTable( bdevlist, cdevlist,  dtab_alias );
	}

	if (!Silent && NodesCreated) {
		(void) pfmt(stdout, MM_INFO, ":405:\nDevice files have been created for a new %s device:\n", Device_Types[edtptr->xedt_pdtype].string);
		(void) pfmt(stdout, MM_NOSTD, ":406:Host Adapter (HA)          = %d\n", sa->sa_c);
		if (sa->sa_b) {
		(void) pfmt(stdout, MM_NOSTD, ":407:SCSI Bus (BUS)             = %d\n", sa->sa_b);
		}
		(void) pfmt(stdout, MM_NOSTD, ":408:Target Controller (TC) ID  = %d\n", sa->sa_t);
		(void) pfmt(stdout, MM_NOSTD, ":409:Logical Unit (LU) ID       = %d\n",sa->sa_l);
		if (postmsg[0] != BLANK) {
			(void) pfmt(stdout, MM_NOSTD, postmsg);
		}
	}

	(void) fclose(templatefp);
}

/* routine to create disk nodes that look like old systems for compatibility.
 * If the disk driver is configured to support 16 subdevices then the nodes are:
 *	/dev/dsk/0s0 through /dev/dsk/0sf
 *	/dev/rdsk/0s0 through /dev/rdsk/0sf
 *	/dev/dsk/1s0 through /dev/dsk/1sf
 *	/dev/rdsk/1s0 through /dev/rdsk/1sf
 *
 */
void
E333A_nodes(void)
{
	static char obdsk[] = "/dev/dsk/xsx";
	static char ordsk[] = "/dev/rdsk/xsx";
	static char hex[] = "0123456789abcdef";
	int slice, occurrence;
	minor_t	minornum;

	if (!disks.disk_valid)
		return;

	/*
	 * This code creates compatibility nodes for disk occurrences
	 * 0 and 1 - therefore we are guaranteed that these will use
	 * the first set of major numbers for SD01.
	 *
	 * It also creates nodes for /dev/[r]root and /dev/[r]swap
	 * which reference the first set of sd01 minor numbers.
	 */

	make_bnodes(NULL, disks.disk_cmajor, disks.disk_bmajor, level, contents_fp, FALSE);

	for (occurrence=0; occurrence <= 1; occurrence++) {
		if (occurrence+1 > disks.disk_count)
			break;
		obdsk[9] = hex[occurrence];
		ordsk[10] = hex[occurrence];
		minornum = disks.disk_minor[occurrence];
		for (slice=0; slice < V_NUMPAR; slice++) {
			obdsk[11] = hex[slice];
			ordsk[12] = hex[slice];
			/* check for old or incorrect Osx, 1sx nodes
			 * and unlink them.
			 */
			if (!SpecialExists(S_IFBLK,obdsk,disks.disk_bmajor,minornum)) {
				(void) unlink(obdsk);
				if (mknod(obdsk,DEV_MODE | S_IFBLK,makedev(disks.disk_bmajor,minornum)) < 0) {
					warning(":403:mknod failed for %s.\n", obdsk);
				} else {
					(void)chown(obdsk,(uid_t)DEV_UID,(gid_t)DEV_GID);
					(void)lvlfile(obdsk, MAC_SET, &level);
				}
			}
			fprintf(contents_fp,"%s b %d %d ? ? ? %d NULL NULL\n",obdsk,disks.disk_bmajor,minornum,level);
			if (!SpecialExists(S_IFCHR,ordsk,disks.disk_cmajor,minornum)) {
				(void) unlink(ordsk);
				if (mknod(ordsk,DEV_MODE | S_IFCHR,makedev(disks.disk_cmajor,minornum)) < 0) {
					warning(":403:mknod failed for %s.\n", ordsk);
				} else {
					(void)chown(ordsk,(uid_t)DEV_UID,(gid_t)DEV_GID);
					(void)lvlfile(ordsk, MAC_SET, &level);
				}
			}
			fprintf(contents_fp,"%s c %d %d ? ? ? %d NULL NULL\n",ordsk,disks.disk_cmajor,minornum,level);
			minornum++;
		}
	}
}

int
main(int argc, char **argv)
{
					
	register EDT 	*xedtptr  = NULL;	 /* Pointer to edt */
	register EDT 	*xedtptr2 = NULL;	 /* Temp pointer   */
	register int	c, t, lu, e;
	int		ntargets, scsicnt;
	struct scsi_addr	sa;
	char		tcinq[INQ_EXLEN];
	int		edtcnt, i, arg, force_to_run, ppid;
	int		ignore_old_edt, driver_index;
	extern char	*optarg;
	int		no_match;
	STAT		ostatbuf, nstatbuf;
	int		c1, c2;
	char		*label;
	dev_t	sdi_dev;
	major_t	sdi_major;
	int		unsorted;
	ulong_t	tempd, disk1, disk2;
	
/*
 *	Force the command to be pdimkdev if it was run as pdimkdtab
 *		but no arguements were given. Sigh... Can't change it.
 *									Compatability, you know.
 */
	if (argc < 1) {
		(void) strcpy(Cmdname,CMDNAME);
		Pdimkdev = TRUE;
	} else {
		(void) strcpy(Cmdname,basename(argv[0]));
		Pdimkdev = strcmp(Cmdname, CMDNAME) ? FALSE : TRUE;
	}

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxdisksetup");
	label = (char *) malloc(strlen(Cmdname)+1+3);
	sprintf(label, "UX:%s", Cmdname);
	(void) setlabel(label);

	if (lvlin(SYS_PUBLIC, &level) == 0)
		ES_attr = "mode=static state=private range=SYS_RANGE_MAX-SYS_RANGE_MIN startup=no ual_enable=yes other=\">y\"";

	disks.disk_valid = FALSE;

	/* set TCindex to default location */
	(void) strcpy(TCindex,TCINDEX);

	/* set the default MAC level for all nodes to SYS_PRIVATE */
	level = 2;

	Debug = FALSE;
	ignore_old_edt = FALSE;
	force_to_run=FALSE;
	while ((arg = getopt(argc,argv,"Sd:isfu")) != EOF)

		switch (arg) {
		case 'd' : /* alternate tc index supplied */
			(void) strcpy(TCindex,optarg);
			break;
		case 's' : /* Silent mode */
			Silent = TRUE;
			break;
		case 'f' : /* force to run regardless of run-level */
			force_to_run = TRUE;
			break;
		case 'S' : /* Turn on debug messages */
			Debug = TRUE;
			break;
		case 'i' : /* ignore configuration saved in previous boot */
			ignore_old_edt = TRUE;
			break;
		case 'u' : /* update mode  for hot insert/removal */
			UpdateMode = TRUE;
			force_to_run = TRUE;
			ignore_old_edt = TRUE;
			break;
		case '?' : /* Incorrect argument found */
			before_exit(FALSE);
			error(":410:Usage: %s [-f] [-i] [-d file].\n", Cmdname);
			/*NOTREACHED*/
			break;
		}
	
	/* if the parent is init then run this command */
	ppid = getppid();
	if (ppid == 1) {
		force_to_run = TRUE;
	}

	/* Print error unless forced to run */
	/* or running as pdimkdtab         */
	if (!force_to_run && (strcmp(Cmdname, MKDTABNAME) != 0)) {
		before_exit(FALSE);
		errno = 0;
		error(":411:This command is designed to run during the boot sequence.\nUse the -f option to force its execution.\n");
	}

	/* Ignore certain signals */
if (!Debug) {
	(void) signal(SIGHUP,SIG_IGN);
	(void) signal(SIGINT,SIG_IGN);
	(void) signal(SIGTERM,SIG_IGN);
}

	umask(0); /* use template file permission (mode) tokens */

	/* Check to see that the tc.index file exists. */
	if (!FileExists(TCindex)) {
		before_exit(FALSE);
		error(":412:index file %s does not exist.\n", TCindex);
	}

	/* Open the target controller index file. Exits on failure.  */
	if ((TCindexfp = fopen(TCindex,"r")) == NULL) {
		before_exit(FALSE);
		error(":400:Could not open %s.\n", TCindex);
	}

	/* Initialize driver info structure */
	for(i = 0; i < MAXTCTYPE; i++) {
		DRVRinfo[i].valid = FALSE;
	}

	tload(NULL);
	if ((xedtptr = readxedt(&edtcnt)) == 0) {
		tuload();
		before_exit(FALSE);
		error(":413:Unable to read equipped device table.\n");
	}
	tuload();
	if (Debug) {
		printf("edtcnt %d\n", edtcnt);
	}

	if (!ignore_old_edt) {
		if (!FileExists(LAST_BOOT_EDT)) 
			ignore_old_edt = TRUE;
		else
			if ((last_edt_fp = fopen(LAST_BOOT_EDT,"r")) == NULL) 
				ignore_old_edt = TRUE;
	}

	if ((temp_edt_fp = fopen(TEMP_EDT,"w+")) == NULL) {
		before_exit(FALSE);
		error(":400:Could not open %s.\n", TEMP_EDT);
	}
	
	/* write current edt to temp file */
	fwrite (xedtptr, sizeof(struct scsi_xedt), edtcnt, temp_edt_fp);

	/* compare the size of old and new files. no need to do byte
	 * comparison if the sizes are different */
	if (!ignore_old_edt) {
		rewind(temp_edt_fp);	/* get offset from the file beginning */
		if (stat(TEMP_EDT, &nstatbuf) < 0) {
			before_exit(FALSE);
			error(":396:stat failed for %s\n", TEMP_EDT);
		}
		if (stat(LAST_BOOT_EDT, &ostatbuf) < 0) {
			before_exit(FALSE);
			error(":396:stat failed for %s\n", LAST_BOOT_EDT);
		}
		if ( ostatbuf.st_size != nstatbuf.st_size )
			ignore_old_edt = TRUE;
	}

	if (!ignore_old_edt) {

		no_match = FALSE;

		/* byte compare previous and current edt */ 
		while ((c1=getc(temp_edt_fp))!= EOF && (c2=getc(last_edt_fp))!= EOF) {
			if (c1 != c2) {
			no_match = TRUE;
			break;
			}
		}

		fclose(temp_edt_fp);
		fclose(last_edt_fp);
		if (!no_match) {
			unlink (TEMP_EDT);
			exit(NO_CHANGES_EXIT);
		}
	} else
		fclose(temp_edt_fp);

	rename(TEMP_EDT, LAST_BOOT_EDT);
	chmod(LAST_BOOT_EDT, MASK);

	/*
	 * Now that we know we have something to do, open the input file
	 * we create for pdimkdtab so that it can update the contents file.
	 */
	if ( Pdimkdev ) {
		if ((contents_fp = fopen(INSTALLF_INPUT,"w+")) == NULL)
			error(":400:Could not open %s.\n", INSTALLF_INPUT);
	}


	/*
	 * set the owner of host adapter entries to SDI
	 *
	 * "VOID" means a device not yet claimed by a target driver.
	 * ID_PROCESOR indicates a host adapter.
	 *
	 * This allows a processor target driver to claim certain ID_PROCESOR
	 * devices and sdi will get the rest.
	 */
	/* get device for sdi */
	if (sysi86(SI86SDIDEV, &sdi_dev) == -1) {
		errno = 0;
		error(":387:ioctl(SI86SDIDEV) failed.\n");
	}

	xedtptr2 = xedtptr;
	sdi_major = major(sdi_dev) - 1;
	for (e = 0; e < edtcnt; ++e, xedtptr2++) {
	 	if (!strcmp((char *) xedtptr2->xedt_drvname,"VOID") &&
				xedtptr2->xedt_pdtype == ID_PROCESOR) {
			strcpy(xedtptr2->xedt_drvname, "SDI");
			xedtptr2->xedt_cmaj = sdi_major;
			xedtptr2->xedt_bmaj = sdi_major;
			xedtptr2->xedt_minors_per = 32;
		} 
	}

#ifdef DPRINTF
	PrintEDT(xedtptr, edtcnt);
#endif
	unsorted = edt_fix(xedtptr, edtcnt);

#ifdef DPRINTF
	PrintEDT(xedtptr, edtcnt);
#endif
	scsicnt = edt_sort(xedtptr, edtcnt, HBA, unsorted, FALSE);

#ifdef DPRINTF
	PrintEDT(xedtptr, edtcnt);
#endif

	disk1 = ORDINAL_A(MAX_EXHAS,MAX_BUS,MAX_EXTCS) + 1;
	disk2 = ORDINAL_A(MAX_EXHAS,MAX_BUS,MAX_EXTCS) + 1;
	disks.disk_count = 0;
	for ( xedtptr2 = xedtptr, e = 0; e < edtcnt; e++, xedtptr2++ ) {
		if (xedtptr2->xedt_pdtype == ID_RANDOM) {
			disks.disk_count++;
			tempd = xedtptr2->xedt_ordinal + xedtptr2->xedt_lun;
			if (tempd < disk1) {
				disk1 = tempd;
				disks.disk_cmajor = xedtptr2->xedt_cmaj;
				disks.disk_bmajor = xedtptr2->xedt_bmaj;
				disks.disk_minor[0] = xedtptr2->xedt_first_minor;
			} else if (tempd < disk2) {
				disk2 = tempd;
				disks.disk_minor[1] = xedtptr2->xedt_first_minor;
			}
		}
	}
	disks.disk_valid = TRUE;

	for ( xedtptr2 = xedtptr, e = 0; e < edtcnt; e++, xedtptr2++ ) {
		if (!strcmp((char *) xedtptr2->xedt_drvname,"VOID")) {
			continue;
		}
		for (driver_index = -1, i = 0; i < MAXTCTYPE; i++) {
			if (DRVRinfo[i].valid == TRUE) {
				if (!strcmp(DRVRinfo[i].name, xedtptr2->xedt_drvname)) {
				/* found it */
					driver_index = i;
					break;
				}
			} else {
				DRVRinfo[i].valid = TRUE;
				strcpy(DRVRinfo[i].name, xedtptr2->xedt_drvname);
				DRVRinfo[i].majors.b_maj = xedtptr2->xedt_bmaj;
				DRVRinfo[i].majors.c_maj = xedtptr2->xedt_cmaj;
				DRVRinfo[i].subdevs  = DRVRinfo[i].majors.minors_per = xedtptr2->xedt_minors_per;
	DTRACE;
				DRVRinfo[i].lu_limit = (MAXMINOR + 1)/DRVRinfo[i].subdevs;
	DTRACE;
				DRVRinfo[i].lu_occur = -1;
				DRVRinfo[i].instances = 1;
				driver_index = i;
				break;
			}
		}

		if (driver_index < 0) {
			errno = 0;
			error(":414:Too many drivers to start. Only %d drivers supported.\n", MAXTCTYPE);
		}
	}

	if ( !Pdimkdev || UpdateMode )
		ClearDeviceTable();

	if (Debug) {
		printf("driver\tha_slot\tSCSIbus\tTC\tnumlus\tPDtype\tTCinq\n");
	}

	for ( c = 0; c < scsicnt; c++ ) {

		/*
		 * The structure sa is used to pass the current values
		 *	of C,B,T,L and N out of this loop into MakeDeviceNodes
		 *
		 * We need to actually number things from 0 as we start
		 *	here.  So the controller number used to name all nodes
		 *	we make or link in this loop is simply incremented.
		 */


		/*
		 * now start in the order we want to really make the devices in.
		 *
		 * The device that we are making nodes for, as described by
		 *	the array of edt entries, may not be the first device in the
		 *	edt ( Equipped Device Table ).  This is caused by constraints
		 *	on ordering the HBA table imposed by the Loadable Driver stuff.
		 *
		 * So, lets set our starting point for each controller.  We need
		 *	a pointer into the edt that reflects actual position in the edt.
		 *
		 * Now lets make devices for all the targets and luns used by
		 *	the edt_order'th controller in the HBA table.
		 *
		 */

		xedtptr2 = HBA[HBA[c].order].edtptr;
		ntargets = HBA[HBA[c].order].ntargets;
		sa.sa_c = xedtptr2->xedt_ctl;;

		for (t = 0; t < ntargets; t++, xedtptr2++) {

			sa.sa_b = xedtptr2->xedt_bus;
			sa.sa_t = xedtptr2->xedt_target;

			strcpy(tcinq,(char *) xedtptr2->xedt_tcinquiry);

			if (Debug) {
				printf("%s\t%d\t%d\t%d\t%d\t%d\t%s\n",
					xedtptr2->xedt_drvname,
					xedtptr2->xedt_ctl,
					xedtptr2->xedt_bus,
					xedtptr2->xedt_target,
					xedtptr2->xedt_lun,
					xedtptr2->xedt_pdtype,
					xedtptr2->xedt_tcinquiry);
			}

			if (xedtptr2->xedt_drvname == NULL ||
			   (strcmp((char *) xedtptr2->xedt_drvname,"VOID") == 0)) {
				continue;
			}

			/* find the DRVRinfo entry */
			for(driver_index = -1, i = 0; i < MAXTCTYPE; i++) {
				if(DRVRinfo[i].valid == TRUE) {
					if (!strcmp(DRVRinfo[i].name, xedtptr2->xedt_drvname)) {
						driver_index = i;
						break;
					}
				} else {
					break;
				}
			}
			if (driver_index == -1) {
				before_exit(FALSE);
				errno = 0;
				error(":415:Too many drivers. Only %d drivers supported.\n", MAXTCTYPE);
			}

			for(lu = 0; lu < MAX_EXLUS; lu++) {
			    if(xedtptr2->xedt_lun == lu) {

					sa.sa_l = lu;
					sa.sa_n = DRVRinfo[driver_index].instances++;

					DRVRinfo[driver_index].lu_occur++;

					MakeDeviceNodes(&sa, driver_index, xedtptr2);
				}
			} /* end LU loop */
		} /* end TC loop */
	} /* end HA loop using boot-chain based index into EDT */

	(void) fclose (TCindexfp);

	/* make compatibility nodes and update the contents file */
	/* before_exit does nothing if this is mkdtab */
	before_exit(TRUE);

	exit(NORMEXIT);
	/* NOTREACHED */
}


/* OA&M population utility code starts here */

#define BLKSIZE	512

#define SCSI_ATTR	"scsi=true"
#define ATTR_TERM	(char *)NULL

#define GOODEXIT	0
#define BADEXIT		-1

typedef struct type_value
{
	char	type[MAXLINE];
	int	N;
	struct type_value *next;
} TYPE_VALUE_T;
struct type_value	*list;

static int		vfsnum;
static struct vfstab	*vfstab;

extern void   free();

static int getvar(uchar_t *, char *, char *, char *, char *, char *, uchar_t);
static int  get_serial_n();
static void expand();
static void add_dpart();
static int  initialize();
static int  s5part();
static char *memstr();

/*
 *  ClearDeviceTable
 *       removes all scsi device entries from the device table
 *       removes scsi device entries from the scsi device groups
 *            but not from administrator defined groups
 *
 *       returns 0 on success, -1 on failure
 */
int
ClearDeviceTable(void)
{
	char	**criteria_list;
	char	**criteria_ptr;
	char	**rm_list;	/* devices with scsi=true to be removed */
	char	**dev_ptr;
	char	*type;
	char	criteria[MAXLINE];
	char	**dgrp_list;
	char	**dgrp_ptr;
	char	system_str[2*MAXLINE];

	/* initialize list to empty (list used in get_serial_n()) */
	list = (struct type_value *)NULL;

	/* get list of devices with attribute scsi=true */
	/* build criteria list */
	criteria_list = (char **) malloc(3*sizeof(char **));
	criteria_ptr = criteria_list;
	*criteria_ptr++ = SCSI_ATTR;
	*criteria_ptr = ATTR_TERM;

	if ( (rm_list = getdev((char **)NULL, criteria_list, DTAB_ANDCRITERIA)) == (char **)NULL)
	{
		warning(":416:Unable to clear device table %s, %s failure.\n", DTAB_PATH, "getdev");
		return(BADEXIT);
	}

	for (dev_ptr = rm_list; *dev_ptr != (char *)NULL; dev_ptr++)
	{
		 /* find scsi device group that this device is in */
		 /* first find out which type scsi device we are removing */
		if ( (type = devattr(*dev_ptr, "type")) == (char *)NULL)
		{
			warning(":416:Unable to clear device table %s, %s failure.\n", DTAB_PATH, "devattr");
			continue;
		}
		/* then get a list of groups with that device type */
		(void) sprintf(criteria, "type=%s", type);
		criteria_ptr = criteria_list + 1;  /* skip over scsi=true criteria */
		*criteria_ptr++ = criteria;
		*criteria_ptr = ATTR_TERM;
		if ( (dgrp_list = getdgrp((char **)NULL, criteria_list, DTAB_ANDCRITERIA)) == (char **)NULL)
		{
			/*
			 * This will only occur if an error occurs during getdgrp.
			 * If no groups are present meeting the criteria, a list
			 * with the first element a NULL pointer is returned.
			 * This is true for all device table functions.
			 */
			warning(":417:Unable to clear device group table %s, getdgrp failure.\n", DGRP_PATH);
			continue;
		}

		/* remove the device from the SCSI device groups */
		for (dgrp_ptr = dgrp_list; *dgrp_ptr != (char *)NULL; dgrp_ptr++)
		{
			if (strncmp(*dgrp_ptr, "scsi", 4) != 0)
				continue;
			/*  attempt to remove device from group
			 *  ignore return code since we didn't
			 *       check that device is a member of dgrp
			 */
			(void) sprintf(system_str, "putdgrp -d %s %s > /dev/null 2>&1", *dgrp_ptr, *dev_ptr);
			(void) system(system_str);
		}
		free((char *)dgrp_list);

		/* remove device from device table */
		(void) sprintf(system_str, "putdev -d %s > /dev/null 2>&1", *dev_ptr);
		(void) system(system_str);
	}
	free((char *)criteria_list);
	free((char *)rm_list);

	/* these lines are a work-around for a bug in libadm
	 * close the device and device group tables so that
	 * the above removals will be written to the tables
	 */
	_enddevtab();
	_enddgrptab();

	/* initialize copy of vfstab in memory for use in adding dpart entries */
	return(initialize());
}

/*
 *  MakeDeviceTable
 *       reads the template file to get the alias, device group
 *            and attribute list
 *       create a unique alias for the device table using the alias
 *            for example, the alias is disk and disk1, disk2 exist
 *            then the alias for this device should be disk3
 *       translate variables in the attribute list using the info
 *            passed in
 *            for example, an attribute may be prtvtoc $CDEVICE$ so
 *            substitute the character device passed in for CDEVICE
 *       add the new device to the device table
 *       add the new device to the device group
 *       if type is disk, add partition entries
 *
 *       returns 0 on success, -1 on failure
 */
int
MakeDeviceTable(char *b_dev_name, char *c_dev_name, uchar_t *tc_inquiry, uchar_t tc_pdtype, char *dtab_alias)
{
	char	alias[MAXLINE], dgrp[MAXLINE], attr[2*MAXLINE], fsattr[2*MAXLINE], dpattr[2*MAXLINE];
	int	serial_n;
	char	serial_n_str[5];
	char	exp_alias[MAXLINE];
	char	exp_attr[2*MAXLINE];
	char	system_str[2*MAXLINE];
	char	dpartlist[MAXLINE];

	(void) sprintf(alias, "");
	(void) sprintf(dgrp, "");
	(void) sprintf(attr, "");
	(void) sprintf(fsattr, "");
	(void) sprintf(dpattr, "");
	(void) sprintf(dpartlist, "");
	
	/* retrieve alias, device group and attribute list */
	if (getvar(tc_inquiry, alias, dgrp, attr, fsattr, dpattr, tc_pdtype) == BADEXIT)
	{
		/* set errno to zero so that warning will not use perror */
		errno = 0;
		warning(":418:Unable to retrieve alias, device group and attribute list\n\tfrom template file specified by tc_inquiry string %s.\nNo device entry added to device table %s\n\tfor character device %s.\n", tc_inquiry, DTAB_PATH, c_dev_name);
		return(BADEXIT);
	}

	/* get number value necessary to create unique alias */
	serial_n = get_serial_n(alias);
	(void) sprintf(serial_n_str, "%d", serial_n);
	(void) sprintf(exp_alias, "%s%d", alias, serial_n);

	/* return the expanded alias string in dtab_alias */
	(void)strcpy(dtab_alias,exp_alias);

	/* expand variables in attribute list to character or block device
	 * passed in
	 */
	expand(b_dev_name, c_dev_name, serial_n_str, attr, exp_attr, tc_inquiry);

	/* add new device to device table */
	/* need more than one form of command because the block device
	 * name may be null and putdev no longer accepts null parameters
	 */
	if (b_dev_name == (char *)NULL || strlen(b_dev_name) == 0)
		(void) sprintf(system_str, "putdev -a %s %s=%s %s %s",
			exp_alias, DTAB_CDEVICE, c_dev_name, exp_attr,
			ES_attr);
	else
		(void) sprintf(system_str, "putdev -a %s %s=%s %s=%s %s %s",
			exp_alias, DTAB_BDEVICE, b_dev_name, DTAB_CDEVICE,
			c_dev_name, exp_attr, ES_attr);
	(void) system(system_str);

	/* add new device to device group table */
	(void) sprintf(system_str, "putdgrp %s %s", dgrp, exp_alias);
	(void) system(system_str);

	if (strcmp(alias, "disk") == 0)
	{
		/* add entries for partitions */
		if (strlen(fsattr) == 0 || strlen(dpattr) == 0)
		{
			errno = 0;
			warning(":419:FSATTR or DPATTR tokens not defined in template file\n\treferenced by %s inquiry string.\nNo partition entries added to device table for %s.\n", tc_inquiry, exp_alias);
			return(GOODEXIT);
		}
		add_dpart(b_dev_name, c_dev_name, serial_n, fsattr, dpattr, dpartlist);
		if (strlen(dpartlist) == 0)
			return(GOODEXIT);
		(void) sprintf(system_str, "putdev -m %s dpartlist=%s", exp_alias, dpartlist);
		(void) system(system_str);
		if (serial_n == 1)
		{
			(void) sprintf(system_str, "putdev -p dpart11 %s=/dev/root",
				DDB_BDEVLIST);
			(void) system(system_str);
			(void) sprintf(system_str, "putdev -p dpart11 %s=/dev/rroot",
				DDB_CDEVLIST);
			(void) system(system_str);
		}
	}

	return(GOODEXIT);
}

/*
 *  UpdateDeviceTable
 *
 *       add the bdevlist and cdevlist attributes to the device table
 *
 *       returns 1 on success, 0 on failure
 */
int
UpdateDeviceTable(char *bdevlist, char *cdevlist, char *dtab_alias)
{
	int		blen,clen;
	char	system_str[3*MAXLINE];

	if ( ! strlen(dtab_alias) )	/* no alias means no 'O' in template */
		return(0);

	blen = strlen(bdevlist);
	if ( bdevlist[blen-1] == '=' ) /* was anything added to bdevlist */
		blen = 0;
	else
		bdevlist[blen-1] = '\0';		/* yes, remove trailing comma */

	clen = strlen(cdevlist);
	if ( cdevlist[clen-1] == '=' )	/* was anything added to cdevlist */
		clen = 0;
	else
		cdevlist[clen-1] = '\0';		/* yes, remove trailing comma */
	
	if ( !blen && !clen )	/* nothing in either, nothing to do */
		return(0);

	if ( blen && clen )
		(void)strcat(bdevlist, " ");	/* need space when both exist */

	/* update device in device table */

	(void)sprintf(system_str, "putdev -m %s ", dtab_alias );

	if ( blen )
		(void)strcat(system_str, bdevlist);

	if ( clen )
		(void)strcat(system_str, cdevlist);

	(void)system(system_str);	/* run putdev */
		
	return(1);
}

/*
 * getvar
 *
 * open template file using tc_inquiry string
 * read template file searching for the ALIAS, DGRP, ATTR and possibly
 *      FSATTR and DPATTR tokens
 * close template file
 */
static int
getvar(uchar_t *tc_inquiry, char *alias, char *dgrp, char *attr, char *fsattr, char *dpattr, uchar_t tc_pdtype)
{
	FILE		*templatefp;
	int		data_begin;
	register int	tokenindex;

	if ((templatefp = OpenTemplate(tc_pdtype, tc_inquiry)) == NULL)
		return(BADEXIT);

	data_begin = 0;
	while (!data_begin)
	{
		tokenindex = GetToken(templatefp);
		switch(tokenindex)
		{
			case ALIAS:
				(void) fscanf(templatefp, " %[^\n]\n", alias);
				break;
			case DGRP:
				(void) fscanf(templatefp, " %[^\n]\n", dgrp);
				break;
			case ATTR:
				(void) fscanf(templatefp, " %[^\n]\n", attr);
				break;
			case FSATTR:
				(void) fscanf(templatefp, " %[^\n]\n", fsattr);
				break;
			case DPATTR:
				(void) fscanf(templatefp, " %[^\n]\n", dpattr);
				break;
			case DATA:
				data_begin = 1;
				break;
			case EOF:
				(void) fclose(templatefp);
				return(BADEXIT);
				/* NOTREACHED */
				break;
			default:
				(void) fscanf(templatefp, "%*[^\n]%*[\n]");
				break;
		}
	}
	(void) fclose(templatefp);
	if (strlen(alias) == 0 || strlen(dgrp) == 0 || strlen(attr) == 0)
		return(BADEXIT);
	return(GOODEXIT);
}

/*
 *  get_serial_n
 *
 *  get entries already in device table of this type
 *  find the current highest value of N in typeN
 *  add 1 to N and return for use in new alias
 */
static int
get_serial_n(char *alias)
{
	struct type_value	*ptr;
	struct type_value	*lastptr;
	char			criteria[MAXLINE];
	char			**criteria_list;
	char			**criteria_ptr;
	char			**dev_list;
	char			**dev_ptr;
	int 			n, id;

	/* has this type been evaluated before */
	for (ptr = list, lastptr = list; ptr != (struct type_value *)NULL; ptr = ptr->next)
	{
		/* save value for end of list in case another member must be added */
		if (ptr->next != (struct type_value *)NULL)
			lastptr = ptr->next;
		if (strcmp(alias, ptr->type) == 0)
			break;
	}

	if (ptr != (struct type_value *)NULL)
	{
		/* N value for this type has been found from table already */
		n = ptr->N;
		ptr->N = n + 1;
		return(n);
	}

	/* build criteria list, criteria is type = alias from template */
	criteria_list = (char **) malloc(2*sizeof(char **));
	criteria_ptr = criteria_list;
	(void) sprintf(criteria, "type=%s", alias);
	*criteria_ptr++ = criteria;
	*criteria_ptr = ATTR_TERM;

	/*
	 * Note that getdev returns (char **)NULL only if an error occurs.
	 * If no devices exist meeting the criteria, getdev returns a
	 * dev_list where the first element is a (char *)NULL
	 */
	if ( (dev_list = getdev((char **)NULL, criteria_list, DTAB_ANDCRITERIA)) == (char **)NULL)
	{
		warning(":420:Unable to get current list of devices of type %s.\nUsing %s0 as alias.\n", alias, alias);
		free((char *)criteria_list);
		return(0);
	}

	free((char *)criteria_list);

	/* get the highest N value from typeN */
	n = id = 0;
	for (dev_ptr = dev_list; *dev_ptr != (char *)NULL; dev_ptr++)
	{
		id = atoi(*dev_ptr + strlen(alias));
		if (id > n)
			n = id;
	}
	n = n + 1;
	free((char *)dev_list);

	/* add this type to the list */
	ptr = (struct type_value *)malloc(sizeof(struct type_value));
	(void) strcpy(ptr->type, alias);
	ptr->N = n + 1;
	ptr->next = (struct type_value *)NULL;
	if (lastptr == (struct type_value *)NULL)
		/* first member of list */
		list = ptr;
	else
		lastptr->next = ptr;

	/* return N for use in creating alias */
	return(n);
}

/*
 *  expand
 * 
 *  variables in attribute list are surrounded by $ characters
 *  find $ character and translate next token
 */
static void
expand(char *b_dev_name, char *c_dev_name, char *serial_n_str, char *attr, char *exp_attr, uchar_t *tc_inquiry)
{
	char	*attr_var;
	char	*dev_name;
	char	*dev;

	/* dev_name is device name without path (cNtNdNsN)
	 * dev is device name without path or slice (cNtNdN)
	 * create device name from character device name
	 * this assumes that device name is in form /dev/rxxx/device_name
 	 * or something similar
	 * strip the sN for dev
 	 */
	dev_name = strrchr(c_dev_name, '/') + 1;
	dev = strdup(dev_name);
	(void)strtok(dev, "s");

	attr_var = strtok(attr, "$");
	(void) strcpy(exp_attr, attr);
	while (attr_var != (char *)NULL)
	{
		attr_var = strtok((char *)NULL, "$");
		if (attr_var == NULL)
			continue;
		/* N value */
		if (strcmp(attr_var, "N") == 0)
		{
			(void) strcat(exp_attr, serial_n_str);
		}
		/* block device name */
		else if (strcmp(attr_var, "BDEVICE") == 0)
		{
			(void) strcat(exp_attr, b_dev_name);
		}
		/* character device name */
		else if (strcmp(attr_var, "CDEVICE") == 0)
		{
			(void) strcat(exp_attr, c_dev_name);
		}
		/* device name without character or block path */
		else if (strcmp(attr_var, "DEVICE") == 0)
		{
			(void) strcat(exp_attr, dev_name);
		}
		else if (strcmp(attr_var, "DEV") == 0)
		{
			(void) strcat(exp_attr, dev);
		}
		else if (strcmp(attr_var, "INQUIRY") == 0)
		{
			if (tc_inquiry)
				(void) strcat(exp_attr, (char *)tc_inquiry);
			else
				(void) strcat(exp_attr, "                        ");
		}
		else
		{
			errno = 0;
			warning(":421:Cannot expand variable %s.\n", attr_var);
		}

		attr_var = strtok((char *)NULL, "$");
		if (attr_var != NULL)
			(void) strcat(exp_attr, attr_var);
	}
	return;
}

/*
 * add_dpart() gets information about the specified hard drive from the vtoc
 * and vfstab and adds the partition entries to device.tab.
 */
static void
add_dpart(char *b_dev_name, char *c_dev_name, int serial_n, char *fsattr, char *dpattr, char *dpartlist)
{
	struct vtoc	vtoc;
	int		s, j;
	static char	hex[] = "0123456789abcdef";
	char		hex_s;
	char		serial_ns_str[5];
	char		exp_alias[MAXLINE], exp_bdevice[MAXLINE], exp_cdevice[MAXLINE], exp_attr[2*MAXLINE];
	char		*bdev_p, *cdev_p, bdev_alias[MAXLINE], cdev_alias[MAXLINE];
	char		tmpfsattr[2*MAXLINE], tmpdpattr[2*MAXLINE];
	char		system_str[2*MAXLINE];
	char		*mountpoint;
	int		first = 0;

	if (rd_vtoc(c_dev_name, &vtoc) <= 0)
		return;

	/*
	 * Build a table of disk partitions we are interested in and finish
	 * add each partition to the dpartlist.
	 */
	for (s = 0; s < (int)vtoc.v_nparts; ++s)
	{
		if (vtoc.v_part[s].p_size == 0 || ((vtoc.v_part[s].p_flag & V_VALID) != V_VALID))
			continue;
		hex_s = hex[s];
		(void) sprintf(exp_cdevice, "%.*s%c", strlen(c_dev_name) - 1, c_dev_name, hex_s);
		if (strcmp(exp_cdevice, c_dev_name) == 0)
			/* don't add a dpart entry for partition that has
			 * been added as a disk entry
			 */
			continue;
		(void) sprintf(exp_bdevice, "%.*s%c", strlen(b_dev_name) - 1, b_dev_name, hex_s);
		(void) sprintf(serial_ns_str, "%d%c", serial_n, hex_s);
		(void) sprintf(exp_alias, "dpart%s", serial_ns_str);
		cdev_p = strrchr(c_dev_name, '/') + 1;
		(void) sprintf(cdev_alias, "%.*s%ds%c", cdev_p - c_dev_name,
			c_dev_name, serial_n - 1, hex_s);
		bdev_p = strrchr(b_dev_name, '/') + 1;
		(void) sprintf(bdev_alias, "%.*s%ds%c", bdev_p - b_dev_name,
			b_dev_name, serial_n - 1, hex_s);

		/* expand destroys the attribute list so copy lists
		 * to temporary lists for use by next partition
		 */
		(void) strcpy(tmpfsattr, fsattr);
		(void) strcpy(tmpdpattr, dpattr);
		expand(exp_bdevice, exp_cdevice, serial_ns_str, tmpfsattr, exp_attr, NULL);

		/*
		 * We assemble the rest of the information about the partitions by
		 * looking in the vfstab and at the disk itself.  If vfstab says the
		 * partition contains a non-s5 file system we believe it, otherwise
		 * we call s5part() which will check for an s5 super block on the disk
		 * and do the putdev if found
		 */
		for (j = 0; j < vfsnum; j++)
		{
			if(strcmp(exp_bdevice,vfstab[j].vfs_special)==0)
				break;
		}
		if (j < vfsnum)
		{
			/*
			 * Partition found in vfstab.
			 */
			if (strncmp(vfstab[j].vfs_fstype,"s5",2) == 0)
			{
				/*
				 * If s5part() finds a file system it will create
				 * the device.tab entry.  If not, we have a
				 * conflict with what vfstab says so we leave
				 * this partition out of device.tab.
				 */
				if ((s5part(exp_alias, exp_bdevice, exp_cdevice,
					exp_attr, vtoc.v_part[s].p_size,
					vfstab[j].vfs_mountp) == GOODEXIT) &&
					(serial_n <= 2))
				{
					(void) sprintf(system_str,
						"putdev -m %s %s=%s %s=%s", exp_alias,
						DDB_BDEVLIST, bdev_alias,
						DDB_CDEVLIST, cdev_alias);
					(void) system(system_str);
				}
			}
			else
			{
				if (strcmp(vfstab[j].vfs_mountp, "-") == 0)
					mountpoint="/mnt";
				else
					mountpoint=vfstab[j].vfs_mountp;
				/* add new device to device table */
				(void) sprintf(system_str,
					"putdev -a %s %s=%s %s=%s capacity=%d mountpt=%s %s %s",
					exp_alias, DTAB_BDEVICE, exp_bdevice,
					DTAB_CDEVICE, exp_cdevice, vtoc.v_part[s].p_size,
					mountpoint, exp_attr, ES_attr);
				(void) system(system_str);
				if (serial_n <= 2)
				{
					(void) sprintf(system_str, "putdev -m %s %s=%s %s=%s",
						exp_alias, DDB_BDEVLIST, bdev_alias,
						DDB_CDEVLIST, cdev_alias);
					(void) system(system_str);
				}
			}
		}
		else
		{
			/*
			 * Partition not in vfstab.  See if it's an s5
			 * file system; if not, call it a data partition.
			 */
			if (s5part(exp_alias, exp_bdevice, exp_cdevice, exp_attr, vtoc.v_part[s].p_size, (char *)NULL) == BADEXIT)
			{
				/* add new device to device table */
				expand(exp_bdevice, exp_cdevice, serial_ns_str, tmpdpattr, exp_attr, NULL);
				(void) sprintf(system_str, "putdev -a %s %s=%s %s=%s capacity=%d %s %s",
					exp_alias, DTAB_BDEVICE, exp_bdevice,
					DTAB_CDEVICE, exp_cdevice,
					vtoc.v_part[s].p_size, exp_attr, ES_attr);
				(void) system(system_str);
			}
			if (serial_n <= 2)
			{
				(void) sprintf(system_str,
					"putdev -m %s %s=%s %s=%s", exp_alias,
					DDB_BDEVLIST, bdev_alias, DDB_CDEVLIST, cdev_alias);
				(void) system(system_str);
			}
		}
		/* add new device to device group table */
		(void) sprintf(system_str, "putdgrp scsidpart %s", exp_alias);
		(void) system(system_str);

		/* add alias to dpartlist to be returned */
		if (first == 0)
		{
			(void) sprintf(dpartlist, "%s", exp_alias);
			first++;
		}
		else
		{
			(void) strcat(dpartlist, ",");
			(void) strcat(dpartlist, exp_alias);
		}
	}
	return;
}


static int
initialize(void)
{
	FILE		*fp;
	int		i;
	struct vfstab	vfsent;

	/*
	 * Build a copy of vfstab in memory for later use.
	 */
	if ((fp = fopen("/etc/vfstab", "r")) == NULL)
	{
		warning(":422:No device entries will be added to device table %s for disk partitions.\n\tUnable to open /etc/vfstab.\n", DTAB_PATH);
		return(BADEXIT);
	}

	/*
	 * Go through the vfstab file once to get the number of entries so
	 * we can allocate the right amount of contiguous memory.
	 */
	vfsnum = 0;
	while (getvfsent(fp, &vfsent) == 0)
		vfsnum++;
	rewind(fp);

	if ((vfstab = (struct vfstab *)malloc(vfsnum * sizeof(struct vfstab))) == NULL)
	{
		warning(":423:No device entries will be added to device table %s for disk partitions.\n\tOut of memory for /etc/vfstab.\n", DTAB_PATH);
		return(BADEXIT);
	}

	/*
	 * Go through the vfstab file one more time to populate our copy in
	 * memory.  We only populate the fields we'll need.
	 */
	i = 0;
	while (getvfsent(fp, &vfsent) == 0 && i < vfsnum)
	{
		if (vfsent.vfs_special == NULL)
			vfstab[i].vfs_special = NULL;
		else
			vfstab[i].vfs_special = memstr(vfsent.vfs_special);
		if (vfsent.vfs_mountp == NULL)
			vfstab[i].vfs_mountp = NULL;
		else
			vfstab[i].vfs_mountp = memstr(vfsent.vfs_mountp);
		if (vfsent.vfs_fstype == NULL)
			vfstab[i].vfs_fstype = NULL;
		else
			vfstab[i].vfs_fstype = memstr(vfsent.vfs_fstype);
		i++;
	}
	(void)fclose(fp);
	return(GOODEXIT);
}


/*
 * s5part() reads the raw partition looking for an s5 file system.  If
 * it finds one it adds a partition entry to device.tab using the
 * information passed as arguments and additional info read from the
 * super-block.
 */
static int
s5part(char *exp_alias, char *exp_bdevice, char *exp_cdevice, char *exp_attr, long capacity, char *mountpt)
{
	int		fd;
	long		lbsize, ninodes;
	struct filsys	s5super;
	char		*mountpoint;
	struct stat	psb, rsb;
	char	system_str[2*MAXLINE];

	if ((fd = open(exp_cdevice, O_RDONLY)) == -1)
		return(BADEXIT);

	if (lseek(fd, SUPERBOFF, SEEK_SET) == -1)
	{
		(void)close(fd);
		return(BADEXIT);
	}

	if (read(fd, &s5super, sizeof(struct filsys)) < sizeof(struct filsys))
	{
		(void)close(fd);
		return(BADEXIT);
	}

	(void)close(fd);

	if (s5super.s_magic != FsMAGIC)
		return(BADEXIT);

	switch(s5super.s_type)
	{
		case Fs1b:
			lbsize = 512;
			ninodes = (s5super.s_isize - 2) * 8;
			break;
		case Fs2b:
			lbsize = 1024;	/* may be wrong for 3b15 */
			ninodes = (s5super.s_isize - 2) * 16; /* may be wrong for 3b15*/
			break;
		case Fs4b:
			lbsize = 2048;
			ninodes = (s5super.s_isize - 2) * 32;
			break;
		default:
			return(BADEXIT);
	}

	if (mountpt != NULL)
	{
		mountpoint = mountpt;	/* Use mount point passed as arg */
	}
	else
	{
		if (strcmp(s5super.s_fname, "root") == 0 &&
		    stat(exp_bdevice, &psb) == 0 &&
		    stat("/dev/root", &rsb) == 0 &&
		    psb.st_rdev ==  rsb.st_rdev)
			mountpoint = "/";
		else
			mountpoint = "/mnt";
	}
	(void) sprintf(system_str,
		"putdev -a %s %s=%s %s=%s capacity=%d fstype=s5 mountpt=%s fsname=%s volname=%s lbsize=%d nlblocks=%d ninodes=%d %s %s",
		exp_alias, DTAB_BDEVICE, exp_bdevice, DTAB_CDEVICE, exp_cdevice,
		capacity, mountpoint, s5super.s_fname, s5super.s_fpack, lbsize,
		s5super.s_fsize, ninodes, exp_attr, ES_attr);
	(void) system(system_str);
	return(GOODEXIT);
}

static char *
memstr(char *str)
{
	char	*mem;

	if ((mem = (char *)malloc((unsigned)strlen(str) + 1)) == NULL)
	{
		warning(":424:No device entries will be added to device table %s for disk partitions.\n\tOut of memory.\n", DTAB_PATH);
		return((char *)NULL);
	}
	return(strcpy(mem, str));
}
