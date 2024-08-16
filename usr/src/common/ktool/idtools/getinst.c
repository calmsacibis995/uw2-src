/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ktool:common/ktool/idtools/getinst.c	1.40"
#ident	"$Header: $"

/* Get an entry from and ID configuration data file.
 *
 * The entry is
 * name = RESET: reset file pointer to beginning of file.
 * name = NEXT: next entry.
 * name = next instance of device/parameter name.
 *
 * Stp points to an mdev, mtun, sdev, stun, sasn stucture.
 *
 * Return  0 if EOF or cannot locate device.
 *	   1 if success.
 *	   IERR_OPEN if cannot open file.
 *	   IERR_READ if a read error occurs.
 *	   IERR_NFLDS if wrong number of fields.
 *	   IERR_FLAGS if incorrect flags field.
 *	   IERR_MAJOR if syntax error in major number(s).
 *	   IERR_MMRANGE if invalid multiple major range.
 *	   IERR_BCTYPE if invalid block/char type.
 *	   IERR_ENTRY if invalid entry-point name.
 *	   IERR_DEPEND if invalid dependee module name.
 *	   IERR_AUTO if invalid entry in an autotune field.
 *	   IERR_TYPE if module type name too long.
 *	   IERR_MAGIC if $magic invalid.
 *
 * Calling functions can tell getinst the location of the files
 * by filling in values for the variables:
 *
 *	extern char instroot[], pathinst[];
 *
 * instroot[] is prepended to the directory name for directory forms
 *	(e.g. "mdevice.d").
 * pathinst[] is prepended to the file name for flat-file forms
 *	(e.g. "mdevice"); this is by default the "cf.d" subdirectory of
 *	the instroot directory.
 */

#include "inst.h"
#include "devconf.h"
#include "mdep.h"
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <locale.h>
#include <pfmt.h>

extern char linebuf[];	/* current input line */
char pathinst[PATH_MAX] = CFPATH;  /* directory containing Master and System files */
char instroot[PATH_MAX] = ROOT; /* configuration "root" directory (/etc/conf) */
int ignore_directives;	/* flag: ignore special directives in Master file */
char path[PATH_MAX];	/* path to the file being processed */

/* File type definitions */
ftype_t ftypes[] = {
/* MDEV */	{ "Master", "mdevice", 0, MDEV, MDEV_VER },
/* MDEV_D */	{ "Master", "mdevice.d", 1, MDEV, MDEV_VER },
/* SDEV */	{ "System", "sdevice", 0, SDEV, SDEV_VER },
/* SDEV_D */	{ "System", "sdevice.d", 1, SDEV, SDEV_VER },
/* MTUN */	{ "Mtune", "mtune", 0, MTUN, MTUNE_VER },
/* MTUN_D */	{ "Mtune", "mtune.d", 1, MTUN, MTUNE_VER },
/* STUN */	{ "Stune", "stune", 0, STUN, STUNE_VER },
/* STUN_D */	{ "Stune", "stune.d", 1, STUN, STUNE_VER },
/* SASN */	{ "Sassign", "sassign", 0, SASN, SASSIGN_VER },
/* SASN_D */	{ "Sassign", "sassign.d", 1, SASN, SASSIGN_VER },
/* NODE */	{ "Node", "node", 0, NODE, NODE_VER },
/* NODE_D */	{ "Node", "node.d", 1, NODE, NODE_VER },
/* FTAB */	{ "Ftab", "ftab", 0, FTAB, FTAB_VER },
/* FTAB_D */	{ "Ftab", "ftab.d", 1, FTAB, FTAB_VER },
/* INTFC */	{ "interface", "interface", 0, INTFC, INTFC_VER },
/* INTFC_D */	{ "interface", "interface.d", 1, INTFC, INTFC_VER },
/* ATUN */	{ "Autotune", "autotune", 0, ATUN, ATUNE_VER },
/* ATUN_D */	{ "Autotune", "autotune.d", 1, ATUN, ATUNE_VER },
/* DRVTYPE */	{ "", "type", 0, DRVTYPE, DRVTYPE_VER },
		{ NULL }
};

driver_t *driver_info;

extern int getmajors();
#if !defined(__STDC__)
extern long strtol();
#endif
static int line(), same(), legalflags(), getinstf();


int
rdinst(type, buf, stp, prev_stat, oversion)
short type;
char *buf;
char *stp;
int prev_stat;
int oversion;
{
	struct multmaj mm;	/* used for multiple majors */
	struct mdev *mdev;
	struct sdev *sdev;
	struct mtune *mtune;
	struct stune *stune;
	struct sassign *sassign;
	struct node *node;
	struct ftab *ftab;
	struct intfc *intfc;
	struct drvtype *drvtype;
	char fmt[78];
	char dummy[2];
	char tag[12];
	int n;

	switch (type) {
	case MDEV:
		mdev = (struct mdev *)stp;
		if (prev_stat != I_MORE) {
			mdev->name[0] = '\0';
			mdev->extname[0] = '\0';
			mdev->modtype[0] = '\0';
			mdev->depends = NULL;
			mdev->entries = NULL;
			mdev->magics = NULL;
			mdev->interfaces = NULL;
			mdev->over = oversion;
		}
		if (strncmp(buf, "$oversion", 9) == 0 && isspace(buf[9])) {
			mdev->over = atoi(buf + 10);
			return I_MORE;
		}
		if (strncmp(buf, "$name", 5) == 0 && isspace(buf[5])) {
			char *name;

			for (buf += 6; isspace(*buf); buf++)
				;
			name = buf;
			while (*++buf != '\0' && !isspace(*buf))
				;
			if (*buf != '\n' || buf - name > NAMESZ - 1)
				return IERR_NAME;
			strncpy(mdev->extname, name, buf - name);
			mdev->extname[buf - name] = '\0';
			return I_MORE;
		}
		if (strncmp(buf, "$entry", 6) == 0 && isspace(buf[6])) {
			char *ename, old_c;

			if (ignore_directives)
				return I_MORE;
			for (buf += 7; isspace(*buf); buf++)
				;
			while (*buf != '\0') {
				ename = buf;
				while (*++buf != '\0' && !isspace(*buf))
					;
				if ((old_c = *buf) != '\0')
					*buf++ = '\0';
				n = lookup_entry(ename, &mdev->entries, 1);
				if (old_c != '\0')
					buf[-1] = old_c;
				if (!n)
					return IERR_ENTRY;
				for(; isspace(*buf); buf++)
					;
			}
			return I_MORE;
		}
		if (strncmp(buf, "$magic", 6) == 0 && isspace(buf[6])) {
			char *magic, old_c;
			struct magic_list *mlist;
			int nmagic = 0;

			if (ignore_directives)
				return I_MORE;
			if (mdev->magics != NULL)
				return IERR_MAGIC;
			for (buf += 7; isspace(*buf); buf++)
				;
			mlist = (struct magic_list *)calloc(sizeof(struct magic_list), 1);
			while (*buf != '\0') {
				magic = buf;
				while (*++buf != '\0' && !isspace(*buf))
					;
				if ((old_c = *buf) != '\0')
					*buf++ = '\0';
				if (strcmp("wildcard", magic) == 0)
					mlist->wildcard = 1;
				else
					mlist->magics[nmagic++] =
					(unsigned short)strtol(magic, NULL, 0);
				if (old_c != '\0')
					buf[-1] = old_c;
				for (; isspace(*buf); buf++)
					;
			}
			if (nmagic || mlist->wildcard) {
				mlist->nmagic = nmagic;
				mdev->magics = mlist;
			} else {
				free(mlist);
				return IERR_MAGIC;
			}
			return I_MORE;
		}
		if (strncmp(buf, "$modtype", 8) == 0 && isspace(buf[8])) {
			int typelen;

			for (buf += 9; isspace(*buf); buf++)
				;
			if ((typelen = strlen(buf) - 1) >= MTYPESZ - 1)
				return IERR_TYPE;
			strncpy(mdev->modtype, buf, typelen - 1);
			/* drop the final newline */
			mdev->modtype[typelen] = '\0';
			return I_MORE;
		}
		if (strncmp(buf, "$depend", 7) == 0 && isspace(buf[7])) {
			return get_depend(buf, &mdev->depends);
		}
		if (strncmp(buf, "$contact", 8) == 0 && isspace(buf[8])) {
			return I_MORE;
		}
		if (strncmp(buf, "$interface", 10) == 0 && isspace(buf[10])) {
			struct interface_list *ifp, *ifp2;
			int nver, ver;

			if (ignore_directives)
				return I_MORE;
			ifp = (struct interface_list *)malloc(sizeof(struct interface_list));
			if (ifp == NULL) {
				fprintf(stderr,
					"Not enough memory for interface list.\n");
				exit(1);
			}
			buf = strdup(buf + 11);
			ifp->name = strtok(buf, " \t\n");
			if (ifp->name[0] == '\0') {
intfc_error:
				free(ifp);
				free(buf);
				return IERR_INTFC;
			}
			for (ifp2 = mdev->interfaces; ifp2; ifp2 = ifp2->next) {
				if (strcmp(ifp2->name, ifp->name) == 0) {
intfc_dup:
					free(ifp);
					free(buf);
					return IERR_INTFC_DUP;
				}
			}
			for (nver = 0; (buf = strtok(NULL, " \t\n")) != NULL;) {
				if (nver == MAX_VER)
					goto intfc_error;
				for (ver = 0; ver < nver; ver++) {
					if (strcmp(ifp->versions[ver],
						   buf) == 0)
						goto intfc_dup;
				}
				ifp->versions[nver++] = buf;
			}
			if ((strcmp(ifp->name, "base") == 0 ||
			     strcmp(ifp->name, "nonconforming") == 0) !=
							(nver == 0))
				goto intfc_error;
			ifp->versions[nver] = NULL;
			ifp->next = mdev->interfaces;
			mdev->interfaces = ifp;
			return I_MORE;
		}
		sprintf(fmt, "%%%ds %%%ds %%%ds %%hd %%%ds %%%ds %%1s",
			NAMESZ - 1, PFXSZ - 1, FLAGSZ - 1,
			RANGESZ - 1, RANGESZ - 1);
		n = sscanf(buf, fmt,
				mdev->name,
				mdev->prefix,
				mdev->mflags,
				&mdev->order,
				mm.brange,
				mm.crange,
				dummy);
		if (n != 6)
			return IERR_NFLDS;

		if (mdev->extname[0] == '\0')
			strcpy(mdev->extname, mdev->name);

		/* make sure specified flags are all legal */
		if (!legalflags(mdev->mflags, ALL_MFLAGS))
			return IERR_FLAGS;
		if (INSTRING(mdev->mflags, FCOMPAT) && mdev->over != 0)
			return IERR_FLAGS;

		/* convert the major numbers, read as a string         */
		/* into a number (if single major) or a start/end pair */
		/* if multiple majors are specified.                   */

		n = getmajors(mm.brange, &mdev->blk_start, &mdev->blk_end);
		if (n != 0)
			return n;

		n = getmajors(mm.crange, &mdev->chr_start, &mdev->chr_end);
		if (n != 0)
			return n;

		return 1;

	case SDEV:
		sdev = (struct sdev *)stp;
		if (prev_stat != I_MORE) {
			sdev->over = ftypes[type].cur_ver;
			sdev->conf_static = 0;
			sdev->bind_cpu = -1;
		}
		if (strncmp(buf, "$oversion", 9) == 0 && isspace(buf[9])) {
			sdev->over = atoi(buf + 10);
			return I_MORE;
		}
		if (strcmp(buf, "$static\n") == 0) {
			sdev->conf_static = 1;
			return I_MORE;
		}
		sprintf(fmt, "%%%ds", NAMESZ - 1);
		strcat(fmt, " %c %ld %hd %hd %hd %lx %lx %lx %lx %hd %d %1s");
		n = sscanf(buf, fmt,
				sdev->name,
				&sdev->conf,
				&sdev->unit,
				&sdev->ipl,
				&sdev->itype,
				&sdev->vector,
				&sdev->sioa,
				&sdev->eioa,
				&sdev->scma,
				&sdev->ecma,
				&sdev->dmachan,
				&sdev->bind_cpu,
				dummy);
		if (n == 11)
			sdev->bind_cpu = -1;
		else if (n != 12)
			return IERR_NFLDS;
		return 1;

	case MTUN:
		{ char s1[MAX_STR_TUNE], s2[16], s3[16];

		mtune = (struct mtune *)stp;
		sprintf(fmt, "%%%ds %%%ds", TUNESZ - 1, MAX_STR_TUNE - 1);
		strcat(fmt, " %16s %16s %11s %1s");
		n = sscanf(buf, fmt, mtune->name, s1, s2, s3, tag, dummy);
		if ((n == 5 && 
		    ((strcmp(tag, "%%INS%%") == 0) || 
			(strcmp(tag, "%%AUTO%%") == 0))) ||
		    (n == 3 && 
		    ((strcmp(s2, "%%INS%%") == 0)) || 
			(strcmp(s2, "%%AUTO%%") == 0)))
			n--;
		switch (n) {
		case 4:
			mtune->str_def = NULL;
			mtune->def = strtol(s1, NULL, 0);
			mtune->min = strtol(s2, NULL, 0);
			mtune->max = strtol(s3, NULL, 0);
			return 1;
		case 2:
			if (strcmp(s1, "\"\"") == 0)
				s1[0] = '\0';
			if ((mtune->str_def = malloc(strlen(s1) + 1)) == NULL) {
				pfmt(stderr, MM_ERROR, ":202:Not enough memory for mtune table.\n");
				exit(1);
			}
			strcpy(mtune->str_def, s1);
			return 1;
		default:
			return IERR_NFLDS;
		}
		}

	case STUN:
		{ char s1[MAX_STR_TUNE];

		stune = (struct stune *)stp;
		sprintf(fmt, "%%%ds %%%ds %%1s", TUNESZ - 1, MAX_STR_TUNE - 1);
		n = sscanf(buf, fmt, stune->name, s1, dummy);
		if (n != 2)
			return IERR_NFLDS;
		if (strcmp(s1, "\"\"") == 0)
			s1[0] = '\0';
		if ((stune->value = malloc(strlen(s1) + 1)) == NULL) {
			pfmt(stderr, MM_ERROR, ":203:Not enough memory for stune table.\n");
			exit(1);
		}
		strcpy(stune->value, s1);
		return 1;
		}

	case SASN:
		sassign = (struct sassign *)stp;
		sassign->objname[0] = '\0';
		sassign->low = sassign->blocks = 0;
		sprintf(fmt, "%%%ds %%%ds %%ld %%%ds %%ld %%ld %%1s",
			NAMESZ - 1,
			NAMESZ - 1,
			MAXOBJNAME - 1);
		n = sscanf(buf, fmt,
				sassign->device,
				sassign->major,
				&sassign->minor,
				sassign->objname,
				&sassign->low,
				&sassign->blocks,
				dummy);
		if (n < 3 || n > 6)
			return IERR_NFLDS;
		return 1;

	case NODE:
		node = (struct node *)stp;
		sprintf(fmt, "%%%ds %%%ds %%10s %%%ds %%ld %%ld %%o %%ld %%1s",
			NAMESZ - 1,
			MAXOBJNAME - 1,
			NAMESZ - 1);
		n = sscanf(buf, fmt,
				node->major,
				node->nodename,
				tag,
				node->majminor,
				&node->uid,
				&node->gid,
				&node->mode,
				&node->level,
				dummy);

		switch (n) {
		case 4:
			node->uid = node->gid = -1;
			node->mode = 0666;
		case 7:
			node->level = 0;
			break;
		case 8:
			break;
		default:
			return IERR_NFLDS;
		}

		node->type = tag[0];
		if (node->type != BLOCK && node->type != CHAR)
			return IERR_BCTYPE;
		if (tag[1] == ':')
			node->maj_off = atoi(tag + 2);
		else
			node->maj_off = 0;
		if (isdigit(node->majminor[0])) {
			node->minor = atoi(node->majminor);
			node->majminor[0] = '\0';
		}
		return 1;

	case FTAB:
		ftab = (struct ftab *)stp;
		sprintf(fmt, "%%%ds %%%ds %%%ds %%%ds %%1s",
			SYMSZ - 1,
			SYMSZ - 1,
			TYPESZ - 1,
			FLAGSZ - 1);
		n = sscanf(buf, fmt,
				ftab->entry,
				ftab->tabname,
				ftab->type,
				ftab->fflags,
				dummy);
		switch (n) {
		case 3:
			strcpy(ftab->fflags, "");
			break;
		case 4:
			break;
		default:
			return IERR_NFLDS;
		}

		return 1;

	case INTFC:
		{ struct intfc_sym *symp;

		intfc = (struct intfc *)stp;
		if (prev_stat != I_MORE) {
			intfc->repver = NULL;
			intfc->symbols = NULL;
			intfc->depends = NULL;
		}
		if (strncmp(buf, "$replace", 8) == 0 && isspace(buf[8])) {
			if (intfc->repver != NULL)
				return IERR_DUPREP;
			buf = strtok(buf + 9, " \t\n");
			if (buf == NULL || strtok(NULL, " \t\n") != NULL)
				return IERR_NFLDS;
			intfc->repver = strdup(buf);
			return I_MORE;
		}
		if (strncmp(buf, "$depend", 7) == 0 && isspace(buf[7])) {
			return get_depend(buf, &intfc->depends);
		}
		symp = (struct intfc_sym *)malloc(sizeof(struct intfc_sym));
		if (symp == NULL) {
			fprintf(stderr,
				"Not enough memory for interface symbol.\n");
			exit(1);
		}
		sprintf(fmt, "%%%ds %%%ds %%1s",
			FUNCSZ - 1,
			FUNCSZ - 1);
		n = sscanf(buf, fmt,
				symp->symname,
				symp->newname,
				dummy);
		if (n == 1)
			symp->newname[0] = '\0';
		else if (n != 2)
			return IERR_NFLDS;

		symp->next = intfc->symbols;
		intfc->symbols = symp;

		return I_MORE;
		}

	case ATUN:
		{ char s0[5], s1[16], s2[16], s3[16];
			struct atune *atune;

		atune = (struct atune *)stp;
		sprintf(fmt, "%%%ds ", TUNESZ - 1);
		strcat(fmt, " %3s %6s %11s %11s");
		n = sscanf(buf, fmt, atune->tv_name, s0, s1, s2, s3);
		if (n == -1)
			return 0;
		if (n != 5)
			return(IERR_NFLDS);

		if (strcmp(s0, "DEF") == 0)
			atune->tv_which = TV_DEF;
		else if (strcmp(s0, "MAX") == 0)
			atune->tv_which = TV_MAX;
		else if (strcmp(s0, "MIN") == 0) 
			atune->tv_which = TV_MIN;
		else {
			return(IERR_AUTO);
		}
		if (strcmp(s1, "LINEAR") == 0)
			atune->tv_linetype = TV_LINEAR;
		else if (strcmp(s1, "STEP") == 0)
			atune->tv_linetype = TV_STEP;
		else {
			pfmt(stderr, MM_WARNING, ":204:autotune linetype %s unrecognized, using STEP\n", s1);
			atune->tv_linetype = TV_STEP;
		}
		atune->tv_mempt = strtol(s2, NULL, 0);
		atune->tv_tuneval = strtol(s3, NULL, 0);
		return 1;
		}
	case DRVTYPE:
		drvtype = (struct drvtype *)stp;
		sprintf(fmt, "%%%ds",
			DRVTYPSZ - 1);
		n = sscanf(buf, fmt,
				drvtype->type_name,
				dummy);
		if (n != 1)
			return IERR_NFLDS;

		return 1;
	}

	return IERR_READ;	/* shouldn't happen */
}


int
getinst_name(type, name, stp, namep)
short type;
char *name;
char *stp;
char **namep;
{
	ftype_t	*ftp = &ftypes[type];
	int	stat;

	if (*name == *RESET) {
		if (ftp->fp != NULL) {
			if (ftp->is_dir) {
				fclose(ftp->fp);
				ftp->fp = NULL;
			} else
				fseek(ftp->fp, 0L, 0);
		}
		if (ftp->dp != NULL) {
			closedir(ftp->dp);
			ftp->dp = NULL;
		}
		return 1;
	}

	if (ftp->is_dir && ftp->dp == NULL) {
		if (*name != *NEXT) {
			sprintf(path, "%s/%s/%s", instroot, ftp->fname, name);
			if (namep)
				*namep = strdup(name);
		} else {
			sprintf(path, "%s/%s", instroot, ftp->fname);
			if ((ftp->dp = opendir(path)) == NULL)
				return IERR_OPEN;
		}
	}

	for (;;) {
		stat = getinstf(ftp, name, stp, namep);
		if (stat != 0 || !ftp->is_dir || *name != *NEXT)
			break;
		fclose(ftp->fp);
		ftp->fp = NULL;
		if (namep)
			return 1;
	}

	return stat == 2 ? 0 : stat;
}

int
getinst(type, name, stp)
short type;
char *name;
char *stp;
{
	return getinst_name(type, name, stp, NULL);
}
	

static int
getinstf(ftp, name, stp, namep)
ftype_t *ftp;
char *name;
char *stp;
char **namep;
{
	struct dirent *direntp;
	int stat;
	char *fname;

	if (ftp->fp == NULL) {
		if (!ftp->is_dir)
			sprintf(path, "%s/%s", pathinst, ftp->fname);
		else if (*name == *NEXT) {
			while ((direntp = readdir(ftp->dp)) != NULL &&
			       direntp->d_name[0] == '.')
				;
			if (direntp == NULL)
				return 2;
			sprintf(path, "%s/%s/%s", instroot, ftp->fname,
				direntp->d_name);
			if (namep)
				*namep = strdup(direntp->d_name);
		}

		if ((ftp->fp = fopen(path, "r")) == NULL) {
			if (errno == ENOENT && ftp->is_dir)
				return 2;
			return IERR_OPEN;
		}

		/* New file version defaults to 0 */
		ftp->ver = 0;
	}

	stat = 0;
	do {
next_line:
		if (line(ftp->fp) == 0)
			return ferror(ftp->fp)? IERR_READ : 0;

		/* Check if version number specified */
		if (strncmp(linebuf, "$version", 8) == 0 &&
		    isspace(linebuf[8])) {
			ftp->ver = atoi(linebuf + 9);
			/* Make sure this file is the right version */
			if (ftp->ver != ftp->cur_ver)
				return IERR_VER;

			goto next_line;
		}

		stat = rdinst(ftp->basetype, linebuf, stp, stat, ftp->cur_ver);

	} while (stat == I_MORE ||
		 (*name != *NEXT && !ftp->is_dir && !same(name)));

	return stat;
}


show_ierr(errcode, ftype, dev)
int errcode;
int ftype;
char *dev;
{
	char	*filename = ftypes[ftype].lname;

	pfmt(stderr, MM_ERROR, ":104:FILE: %s\n", path);
	switch (errcode) {
	case IERR_VER:
		pfmt(stderr, MM_ERROR, EMSG_VER, filename);
		break;

	case IERR_NFLDS:
		pfmt(stderr, MM_ERROR, EMSG_NFLDS, filename);
		break;

	case IERR_FLAGS:
		pfmt(stderr, MM_ERROR, EMSG_FLAGS, filename, dev);
		break;

	case IERR_MAJOR:
		pfmt(stderr, MM_ERROR, EMSG_MAJOR, filename, dev);
		break;

	case IERR_MMRANGE:
		pfmt(stderr, MM_ERROR, EMSG_MMRANGE, filename, dev);
		break;

	case IERR_OPEN:
		pfmt(stderr, MM_ERROR, EMSG_OPEN, filename);
		break;

	case IERR_BCTYPE:
		pfmt(stderr, MM_ERROR, EMSG_BCTYPE, filename);
		break;

	case IERR_ENTRY:
		pfmt(stderr, MM_ERROR, EMSG_ENTRY);
		break;

	case IERR_DEPEND:
		pfmt(stderr, MM_ERROR, EMSG_DEPEND);
		break;

	case IERR_AUTO:
		pfmt(stderr, MM_ERROR, EMSG_AUTO);
		break;

	case IERR_TYPE:
		pfmt(stderr, MM_ERROR, EMSG_TYPE);
		break;

	case IERR_MAGIC:
		pfmt(stderr, MM_ERROR, EMSG_MAGIC);
		break;

	case IERR_INTFC:
		pfmt(stderr, MM_ERROR, EMSG_INTFC, dev);
  		break;

	case IERR_INTFC_DUP:
		pfmt(stderr, MM_ERROR, EMSG_INTFC_DUP, dev);
		break;

	case IERR_BADINTFC:
		pfmt(stderr, MM_ERROR, EMSG_BADINTFC);
		break;

	case IERR_MISREP:
		pfmt(stderr, MM_ERROR, EMSG_MISREP, dev);
		break;

	case IERR_DUPREP:
		pfmt(stderr, MM_ERROR, EMSG_DUPREP);
		break;

	case IERR_REPLOOP:
		pfmt(stderr, MM_ERROR, EMSG_REPLOOP);
		break;

	case IERR_NAME:
		pfmt(stderr, MM_ERROR, EMSG_NAME);
		break;

	default:
		pfmt(stderr, MM_ERROR, EMSG_READ, filename);
	}
}


/* Read a line. Skip lines beginning with '*' or '#', and blank lines.
 * Return 0 if EOF. Return 1 otherwise.
 */
static int
line(fp)
FILE *fp;
{
	for (;;) {
		if (fgets(linebuf, LINESZ, fp) == NULL) {
			linebuf[0] = '\0';
			return(0);
		}
		if (*linebuf != '*' && *linebuf != '#' && *linebuf != '\n')
			return(1);
	}
}

/* Check if 'name' is the same string that begins in column 1 of 'linebuf'.
 * 'Name' must be null terminated. The first field of 'linebuf' which is being
 * compared must be followed by white space (this doesn't include '\0').
 * Return 1 if field 1 of 'linebuf' matches name, and 0 otherwise.
 */
static int
same(name)
char *name;
{
	char *b;

	for (b = linebuf; !isspace(*b) && *name != '\0'; b++, name++)
		if (*b != *name)
			return(0);
	if (isspace(*b) && *name == '\0')
		return(1);
	return(0);
}

/* Check for illegal flag characters.
 * Return 1 if all flags are OK, else 0.
 */
static int
legalflags(flags, legal)
char *flags;
char *legal;
{
	while (*flags) {
		if (!INSTRING(legal, *flags++))
			return 0;
	}
	return 1;
}


static int
get_depend(buf, deplistpp)
char *buf;
struct depend_list **deplistpp;
{
	char *dname, old_c;
	struct depend_list *dep;

	if (ignore_directives)
		return I_MORE;
	for (buf += 8; isspace(*buf); buf++)
		;
	while (*buf != '\0') {
		dname = buf;
		while (*++buf != '\0' && !isspace(*buf))
			;
		if ((old_c = *buf) != '\0')
			*buf++ = '\0';
		if (strlen(dname) >= NAMESZ)
			return IERR_DEPEND;
		dep = (struct depend_list *)malloc(sizeof(struct depend_list));
		if (dep == NULL) {
			fprintf(stderr,
				"Not enough memory for dependency list.\n");
			exit(1);
		}
		strcpy(dep->name, dname);
		dep->next = *deplistpp;
		*deplistpp = dep;
		if (old_c != '\0')
			buf[-1] = old_c;
		for(; isspace(*buf); buf++)
			;
	}
	return I_MORE;
}


/* write mdev structure to file */

wrtmdev(d, fp)
struct mdev *d;
FILE *fp;
{
	struct multmaj mm;

	if (d->blk_start == d->blk_end)
		sprintf(mm.brange, "%hd", d->blk_start);
	else
		sprintf(mm.brange, "%hd-%hd", d->blk_start, d->blk_end);
	if (d->chr_start == d->chr_end)
		sprintf(mm.crange, "%hd", d->chr_start);
	else
		sprintf(mm.crange, "%hd-%hd", d->chr_start, d->chr_end);

	fprintf(fp, "%s\t%s\t%s\t%hd\t%s\t%s\n", d->name, d->prefix,
		d->mflags, d->order, mm.brange, mm.crange);
}

/*
 * This routine is used to search through the Master table for
 * some specified device.  If the device is found, we return a pointer to
 * the device.  If the device is not found, we return a NULL.
 */
driver_t *
mfind(device)
char *device;
{
        register driver_t *drv;

        for (drv = driver_info; drv != NULL; drv = drv->next) {
                if (equal(device, drv->mdev.name))
                        return(drv);
        }
        return(NULL);
}


/*
 *  Drvmap entries have the form:
 *
 *	drvname|autconf|...
 *
 *	where drvname is the name of the driver
 *	and autoconf is Y/N
 */

static int
parse_drvmap_entry()
{
        driver_t *drv;
	char *p;

	for (p = linebuf; *p && *p != '|'; p++)
		;
	if (*p)
		*p++ = '\0';

        drv = mfind(linebuf);

	if (drv == NULL)
		return;

	if (*p == 'Y' || *p == 'y')
		drv->autoconf = 1;
}


rddrvmap()
{
	DIR *d;
	struct dirent *e;
	FILE *fp;
	char fnam[PATH_MAX];
	char buf[200];

	sprintf(fnam, "%s/%s", instroot, "drvmap.d");
	d = opendir(fnam);
	if (d == NULL)
		return;

	while ((e = readdir(d)) != NULL)
	{
		if (*(e->d_name) == '.')
			continue;

		sprintf(fnam, "%s/drvmap.d/%s", instroot, e->d_name);
		fp = fopen(fnam, "r");

		if (fp == NULL)
		{
			perror(fnam);
			continue;
		}

		if (line(fp))
			parse_drvmap_entry();

		fclose(fp);
	}

	closedir(d);
}
