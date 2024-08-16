/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pdi.cmds:edvtoc.c	1.3.2.8"

#include <stdio.h>
#include <fcntl.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/vtoc.h>
#include <sys/stat.h>
#include <sys/alttbl.h>

#include <errno.h>
#include <locale.h>
#include <pfmt.h>
#include <limits.h>
#include <langinfo.h>
#include <regex.h>

char    *devname;		/* name of device */
int	devfd = 0;		/* device file descriptor */
FILE	*vtocfile;		/* file containing new vtoc info */
struct  disk_parms   dp;        /* device parameters */
struct  pdinfo	pdinfo;		/* physical device info area */
struct  vtoc	ovtoc;		/* current virt. table of contents */
struct  vtoc	nvtoc;		/* new virt. table of contents */
char    *buf;			/* buffer used to read in disk structs. */
struct  absio absio;

void
main(argc, argv)
int	argc;
char	*argv[];
{
	static char     options[] = "pf:";
	extern int	optind;
	extern char	*optarg;
	struct stat statbuf;
	int c, i;
	int pflag = 0; /* flag to resynch the pdinfo struct only */

        (void) setlocale(LC_ALL, "");
	(void) setcat("uxedvtoc");
	(void) setlabel("UX:edvtoc");

	if (argc < 3) {
		giveusage();
		exit(1);
	}
	while ( (c=getopt(argc, argv, options)) != EOF ) {
		switch (c) {
		case 'f':
			if ((vtocfile = fopen(optarg, "r")) == NULL) {
				(void) pfmt(stderr, MM_ERROR, ":1:Unable to open %s file\n", optarg);
				exit(40);
			}
			break;
		case 'p':
			pflag = 1;
			break;
		default:
			(void) pfmt(stderr, MM_ERROR, ":2:Invalid option '%s'\n", argv[optind]);
			giveusage();
			exit(1);
		}
	}

		/* get the last argument -- device stanza */
	if (argc != optind+1) {
		fclose(vtocfile);
		(void) pfmt(stderr, MM_ERROR, ":3:Missing disk device name\n");
		giveusage();
		exit(1);
	}
	devname = argv[optind];
	if (stat(devname, &statbuf)) {
		fclose(vtocfile);
		(void) pfmt(stderr, MM_ERROR, ":4:Stat of %s failed: %s\n", devname, strerror(errno));
		exit(1);
	}
	if ((statbuf.st_mode & S_IFMT) != S_IFCHR) {
		fclose(vtocfile);
		(void) pfmt(stderr, MM_ERROR, ":5:Device %s is not character special.\n", devname);
		giveusage();
		exit(1);
	}
	if ((devfd=open(devname, O_RDWR)) == -1) {
		(void) pfmt(stderr, MM_ERROR, ":6:Open of %s failed.\n", devname);
		syserror(2);
	}

	if (ioctl(devfd, V_GETPARMS, &dp) == -1) {
		(void) pfmt(stderr, MM_ERROR, ":7:GETPARMS on %s failed.\n", devname);
		syserror(2);
	}

	if ((buf=(char *)malloc(dp.dp_secsiz)) == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":8:Cannot malloc space\n");
		syserror(3);
	}

	absio.abs_sec = dp.dp_pstartsec + VTOC_SEC;
	absio.abs_buf = buf;
	if (ioctl(devfd, V_RDABS, &absio) == -1) {
		(void) pfmt(stderr, MM_ERROR, ":9:Reading pdinfo failed.\n");
		syserror(4);
	}
	memcpy((char *)&pdinfo, absio.abs_buf, sizeof(pdinfo));
	/* check for valid pdinfo - valid version id's are 
	 * V_VERSION_1  V_VERSION_2, V_VERSION_2, and V_VERSION.
	 */
	if ((pdinfo.sanity != VALID_PD) || (pdinfo.version < V_VERSION_1 &&
		pdinfo.version > V_VERSION)) {
		(void) pfmt(stderr, MM_ERROR, ":10:Invalid pdinfo block found.\n");
		fclose(vtocfile);
		close(devfd);
		exit(5);
	}
	memcpy((char *)&ovtoc,&buf[pdinfo.vtoc_ptr%dp.dp_secsiz],sizeof(ovtoc));
	/* check for valid vtoc - valid version id's are 
	 * V_VERSION_1 V_VERSION_2, V_VERSION_2, and V_VERSION.
	 */
	if ((ovtoc.v_sanity != VTOC_SANE) || (ovtoc.v_version < V_VERSION_1 &&
		ovtoc.v_version > V_VERSION)) {
		(void) pfmt(stderr, MM_ERROR, ":11:Invalid VTOC found.\n");
		fclose(vtocfile);
		close(devfd);
		exit(6);
	}
	if (pflag) {
		/* update pdinfo struct to match GETPARMS info */
		pdinfo.cyls = dp.dp_cyls;
		pdinfo.tracks = dp.dp_heads;
		pdinfo.sectors = dp.dp_sectors;
		pdinfo.bytes = dp.dp_secsiz;
		pdinfo.logicalst = dp.dp_pstartsec;
		pdinfo.vtoc_ptr = dp.dp_secsiz * VTOC_SEC + sizeof(pdinfo);
		pdinfo.vtoc_len =  sizeof(struct vtoc);
		pdinfo.alt_ptr = dp.dp_secsiz * (VTOC_SEC + 1);
		pdinfo.alt_len = sizeof(struct alt_info);
 		*((struct pdinfo *)buf) = pdinfo;
 		*((struct vtoc *)&buf[pdinfo.vtoc_ptr%dp.dp_secsiz]) = ovtoc;
		absio.abs_sec = dp.dp_pstartsec + VTOC_SEC;
		absio.abs_buf = buf;
		if (ioctl(devfd, V_WRABS, &absio) == -1) {
			(void) pfmt(stderr, MM_ERROR, ":12:Writing vtoc failed: %s\n", strerror(errno));
			close(devfd);
			exit(1);
		}
		close(devfd);
		exit(0);
	}

	memcpy((char *)&nvtoc, (char *)&ovtoc, sizeof(ovtoc));
	readvtoc();
	if (chose_new_vtoc()) {
		set_timestamps();
 		*((struct pdinfo *)buf) = pdinfo;
 		*((struct vtoc *)&buf[pdinfo.vtoc_ptr%dp.dp_secsiz]) = nvtoc;
		absio.abs_sec = dp.dp_pstartsec + VTOC_SEC;
		absio.abs_buf = buf;
		if (ioctl(devfd, V_WRABS, &absio) == -1) {
			(void) pfmt(stderr, MM_ERROR, ":12:Writing vtoc failed: %s\n", strerror(errno));
		}
	}
	else
		(void) pfmt(stdout, MM_INFO, ":13:New VTOC will not be written to the disk.\n");

	close(devfd);
	exit(0);
}

syserror(exitval)
int exitval;
{
	fclose(vtocfile);
	if (devfd > 0)
		close(devfd);
	(void) pfmt(stderr, MM_ERROR, ":14:System call error is: %s\n", strerror(errno));
	exit();
}

/* set timestamps of slices which have changed to 0, so if mirror driver present */
/* the mirror slice will be updated.						 */
set_timestamps()
{
	int i;

	for (i = 0; i < V_NUMPAR; i++) 
		if ((nvtoc.v_part[i].p_size != ovtoc.v_part[i].p_size) ||
		   (nvtoc.v_part[i].p_start != ovtoc.v_part[i].p_start))
			nvtoc.timestamp[i] = 0;
}

/*  Printslice prints a single slice entry.  */
printslice(v_p)
struct partition *v_p;
{
	(void) pfmt(stdout, MM_NOSTD, ":15:tag: ");
	switch(v_p->p_tag) {
	case V_BOOT:
		(void) pfmt(stdout, MM_NOSTD, ":16:BOOT ");
		break;
	case V_ROOT:
		(void) pfmt(stdout, MM_NOSTD, ":17:ROOT  ");
		break;
	case V_SWAP:
		(void) pfmt(stdout, MM_NOSTD, ":18:SWAP  ");
		break;
	case V_USR:
		(void) pfmt(stdout, MM_NOSTD, ":19:USER  ");
		break;
	case V_BACKUP:
		(void) pfmt(stdout, MM_NOSTD, ":20:DISK  ");
		break;
	case V_STAND:
		(void) pfmt(stdout, MM_NOSTD, ":21:STAND  ");
		break;
	case V_HOME:
		(void) pfmt(stdout, MM_NOSTD, ":22:HOME  ");
		break;
	case V_DUMP:
		(void) pfmt(stdout, MM_NOSTD, ":23:DUMP  ");
		break;
	case V_VAR:
		(void) pfmt(stdout, MM_NOSTD, ":24:VAR  ");
		break;
	case V_ALTS:
		(void) pfmt(stdout, MM_NOSTD, ":25:ALTSECTS  ");
		break;
	case V_ALTTRK:
		(void) pfmt(stdout, MM_NOSTD, ":26:ALTTRKS  ");
		break;
	case V_ALTSCTR:
		(void) pfmt(stdout, MM_NOSTD, ":27:ALT SEC/TRK  ");
		break;
	case V_MANAGED_1:
		(void) pfmt(stdout, MM_NOSTD, ":28:VOLPUBLIC  ");
		break;
	case V_MANAGED_2:
		(void) pfmt(stdout, MM_NOSTD, ":29:VOLPRIVATE  ");
		break;
	case V_OTHER:
		(void) pfmt(stdout, MM_NOSTD, ":30:DOS  ");
		break;
	default:
		if (v_p->p_tag == 0)
			(void) pfmt(stdout, MM_NOSTD, ":31:EMPTY ");
		else
			(void) pfmt(stdout, MM_NOSTD, ":32:other 0x%x  ", v_p->p_tag);	
		break;
	}

	(void) pfmt(stdout, MM_NOSTD, ":33:perms: ");
	if (v_p->p_flag & V_VALID)
		(void) pfmt(stdout, MM_NOSTD, ":34:VALID ");
	if (v_p->p_flag & V_UNMNT)
		(void) pfmt(stdout, MM_NOSTD, ":35:UNMOUNTABLE ");
	if (v_p->p_flag & V_RONLY)
		(void) pfmt(stdout, MM_NOSTD, ":36:READ ONLY ");
	if (v_p->p_flag & V_OPEN)
		(void) pfmt(stdout, MM_NOSTD, ":37:(driver open) ");
	if (v_p->p_flag & ~(V_VALID|V_OPEN|V_RONLY|V_UNMNT))
		(void) pfmt(stdout, MM_NOSTD, ":38:flag: 0x%x", v_p->p_flag);
	(void) pfmt(stdout, MM_NOSTD, ":39:  start: %ld  length: %ld\n", v_p->p_start, v_p->p_size);
}

giveusage()
{
	(void) pfmt(stderr, MM_ACTION, ":40:Usage: edvtoc [-p] [-f vtoc-file] raw-device\n");
}

int 
yes_response()
{
	static regex_t yesre, nore;
	static int first=1;
	char resp[MAX_INPUT];
	int err;

	if (first) {
		first = 0;
		err = regcomp(&yesre, nl_langinfo(YESEXPR), REG_EXTENDED|REG_NOSUB);
		if (err != 0) {
			regerror(err, &yesre, resp, MAX_INPUT);
			pfmt(stderr, MM_ERROR, ":41:Regular expression failure: %s\n", resp);
			exit(6);
		}
		err = regcomp(&nore, nl_langinfo(NOEXPR), REG_EXTENDED|REG_NOSUB);
		if (err != 0) {
			regerror(err, &nore, resp, MAX_INPUT);
			pfmt(stderr, MM_ERROR, ":41:Regular expression failure: %s\n", resp);
			exit(6);
		}
	}
	for (;;) {
		fgets(resp, MAX_INPUT, stdin);
		if (regexec(&yesre, resp, (size_t) 0, (regmatch_t *) 0, 0) == 0)
			return(1);
		if (regexec(&nore, resp, (size_t) 0, (regmatch_t *) 0, 0) == 0)
			return(0);
		(void) pfmt(stdout, MM_INFO, ":42:Invalid response - please answer with y or n. ");
	}
}

			
readvtoc()
{
	int i, n, slice, tag, flag;
	daddr_t size, start;
	daddr_t numsects, availsects, availcyls;

	for (i=1; (fgets(&buf[0],512,vtocfile) != NULL); i++) {
		if (buf[0] == '#')
			continue;
		n = sscanf(&buf[0],"%d 0x%x 0x%x %ld %ld",&slice,&tag,&flag,
			&start, &size); 
		if ((n < 5) || (slice < 0) || (slice >= V_NUMPAR)) {
			(void) pfmt(stderr, MM_ERROR, ":43:Vtocfile line %d is invalid.\n", i);
			fclose(vtocfile);
			close(devfd);
			exit(1);
		}
		if (size > 0)
			nvtoc.v_nparts = slice + 1;
		nvtoc.v_part[slice].p_start = start;
		nvtoc.v_part[slice].p_size = size;
		nvtoc.v_part[slice].p_flag = flag;
		nvtoc.v_part[slice].p_tag = tag;
	}
	fclose(vtocfile);
}

int 
chose_new_vtoc()
{
	int i;

	(void) pfmt(stdout, MM_INFO, ":44:The following slices are the new disk configuration\nyou have created.");
	(void) pfmt(stdout, MM_NOSTD, ":45:  NO ERROR or VALIDITY checking has been done on it.\n\n");
	for(i=0; i < V_NUMPAR; i++) {
		if (nvtoc.v_part[i].p_size > 0) {
			(void) pfmt(stdout, MM_NOSTD, ":46:slice %d: ", i);
			printslice(&nvtoc.v_part[i]);
		}
	}
	(void) pfmt(stdout, MM_NOSTD, ":47:\nIs this configuration the VTOC you want written to %s?\n(y/n) ", devname);
	if (yes_response())
		return(1);
	else
		return(0);
}
