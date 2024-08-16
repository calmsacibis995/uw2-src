/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ktool:common/ktool/idtools/idconfupdate.c	1.24"
#ident	"$Header: $"

#ifdef CROSS
typedef unsigned int uint_t;
#endif

#include <stdio.h>
#include <malloc.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/confmgr.h>
#include <sys/cm_i386at.h>
#include "inst.h"
#include "devconf.h"
#include <locale.h>
#include <pfmt.h>


int debug;

char *rmdb_file = "/dev/resmgr";
char *save_file = "/stand/resmgr";

size_t	endofvals = RM_ENDOFVALS;
char	endofparams[RM_MAXPARAMLEN] = RM_ENDOFPARAMS;

int rmfd;
int valbufsz;
struct rm_args rma;

extern char pathinst[];
extern char instroot[];

void *malloc_fe();
void *realloc_fe();
char *str_save();
time_t get_rm_time();


main(argc, argv)
int argc;
char **argv;
{
	extern  char *optarg;
	int c;
	int force_update = 0;
	int oflag = 0;
	int sflag = 0;

	umask(022);

        (void) setlocale(LC_ALL, "");
        (void) setcat("uxidtools");
        (void) setlabel("UX:idconfupdate");

        while ((c = getopt(argc, argv, "sfo:r:?")) != EOF)
                switch (c) {
		case 'f':	/* update regardless of timestamp */
			force_update = 1;
			break;

		case 'o':
			save_file = str_save(optarg);
			oflag++;
			break;

		case 's':
			sflag++;
			break;

		case 'r':
			strcpy(instroot, optarg);
			break;

                case '?':
		default:
			pfmt(stderr, MM_ACTION, ":205:Usage: %s [options]\n\t-f          force update\n\t-o file     set output file (default is /stand/resmgr)\n\t-s          create a database from the sdevice files\n\t-r confdir  specify an alternate /etc/conf directory\n", argv[0]);
                        exit(1);
                }

	if (sflag)
	{
		if (!oflag)
		{
			pfmt(stderr, MM_ERROR, ":206:must specify -o when using -s\n");
			exit(1);
		}

		initialize_db();
		exit(0);
	}

	rm_init();

	if (force_update || compare_timestamps())
	{
		save_db();
		update_sdevice_files();
	}

	rm_done();

	exit(0);
}


rm_init()
{

	rmfd = open(rmdb_file, O_RDONLY);
	if (rmfd < 0)
	{
		perror(rmdb_file);
		exit(1);
	}

	valbufsz = 1024;
	rma.rm_val = malloc_fe(valbufsz);
}


time_t
get_rm_time()
{
	time_t ret;

	rma.rm_key = RM_KEY;
	if (!rm_getval(RM_TIMESTAMP, 0))
		return 0;

	assert(rma.rm_vallen == sizeof(ret));
	ret = *((time_t *) rma.rm_val);

	return ret;
}


compare_timestamps()
{
	time_t rm_time;
	struct stat buf;

	if (stat(save_file, &buf) != 0)
		return 1;

	rm_time = get_rm_time();

	if (rm_time && rm_time > buf.st_mtime)
		return 1;

	return 0;
}


rm_done()
{

	close(rmfd);
	free(rma.rm_val);
	valbufsz = 0;
	rma.rm_val = NULL;
}


rm_getval(param, n)
char *param;
int n;
{
	int ret;
	int first = 1;			/* for assertion checking */

	rma.rm_n = n;
	if (param)
		strcpy(rma.rm_param, param);

try_again:
	rma.rm_vallen = valbufsz;
	ret = ioctl(rmfd, RMIOC_GETVAL, &rma);

	if (ret != 0)
	{
		if (errno == ENOSPC)
		{
			/*
			 *  Buffer was too small, malloc one big enough
			 */

			assert(valbufsz > 0);
			assert(rma.rm_vallen > valbufsz);	/* paranoia */
			assert(first);
			first = 0;

			valbufsz = rma.rm_vallen;
			free(rma.rm_val);
			rma.rm_val = malloc_fe(valbufsz);
			goto try_again;
		}

		if (errno == ENOENT)
			return 0;	/* out of values for this param */

		perror("RMIOC_GETVAL");
		exit(1);
	}

	return 1;
}


read_mdev()
{
	struct mdev mdev;
        driver_t *drv;
        driver_t **drv_p;
	int stat;

	/*
	 * Tell rdinst() to ignore special directives, such as $entry,
	 * since we haven't set up all the conditions for accurate error
	 * checking, and since we don't care about them anyway.
	 * We haven't defined the legal entry-point types, and we don't
	 * care about them anyway, so just tell getinst() to ignore
	 * $entry lines.
	 */
	ignore_directives = 1;

        (void) getinst(MDEV_D, RESET, NULL);

	while ((stat = getinst(MDEV_D, NEXT, &mdev)) == 1) {
		/* We only care about hardware modules. */
		if (!INSTRING(mdev.mflags, HARDMOD))
			continue;

                if ((drv = (driver_t *)calloc(sizeof(driver_t), 1)) == NULL)
		{
			pfmt(stderr, MM_ERROR, ":208:out of memory\n");
			exit(1);
		}

                drv->mdev = mdev;
		drv->mdev.over = -1;	/* see comment in add_sdev() */

		drv->next = driver_info;
		driver_info = drv;
	}

	if (stat != 0)
		insterror(stat, MDEV_D, mdev.name);
}


read_sdev()
{
	int stat;
	struct sdev sdev;
        driver_t *drv;

	for (drv = driver_info; drv; drv = drv->next)
	{
		(void) getinst(SDEV_D, RESET, NULL);

		while ((stat = getinst(SDEV_D, drv->mdev.name, &sdev)) == 1)
		    add_sdev(&sdev, drv);

		if (stat != 0)
		    insterror(stat, SDEV, drv->mdev.name);
	}
}


add_sdev(sdp, drv)
struct sdev *sdp;
driver_t *drv;
{
	ctlr_t *ctlr;
	ctlr_t *last;

	ctlr = malloc_fe(sizeof(ctlr_t));

	for (last = drv->ctlrs; last && last->drv_link; last = last->drv_link)
		;

	if (last)
		last->drv_link = ctlr;
	else
		drv->ctlrs = ctlr;

	ctlr->drv_link = NULL;

        ctlr->driver = drv;
        ctlr->sdev = *sdp;

        ctlr->num = (drv->n_ctlr)++;
        drv->tot_units += sdp->unit;

	if (sdp->conf_static)
		drv->conf_static = 1;
	if (sdp->over != ftypes[SDEV].cur_ver) {
		/*
		 * mdev.over is technically the old version # of the mdevice
		 * entry, but we don't care about that here and we need a place
		 * to cumulate the sdev old version #, so we use mdev.over
		 */
		drv->mdev.over = sdp->over;
	}
}


throw_away_sdevs(drv)
driver_t *drv;
{
	ctlr_t *ctlr, *next;

	ctlr = drv->ctlrs;
	while (ctlr)
	{
		next = ctlr->drv_link;
		free(ctlr);
		ctlr = next;
	}

	drv->ctlrs = NULL;
	drv->tot_units = 0;
	drv->n_ctlr = 0;
}


turn_off_sdevs(drv)
driver_t *drv;
{
	ctlr_t *ctlr;

	for (ctlr = drv->ctlrs; ctlr; ctlr = ctlr->drv_link)
		ctlr->sdev.conf = 'N';
}


save_original(fnam)
char *fnam;
{
	char dir[PATH_MAX];
	char save[PATH_MAX];
	char cmd[PATH_MAX];

	sprintf(dir, "%s/.sdevice.d", instroot);
	sprintf(save, "%s/%s", dir, fnam);

	if (access(save, F_OK) == 0)
		return;			/* save file already exists */

	if (access(dir, F_OK) != 0)
	{
		mkdir(dir, 0755);
		setlevel(dir, 2);
	}

	sprintf(cmd, "cp %s/sdevice.d/%s %s", instroot, fnam, save);

	if (system(cmd) != 0)
		pfmt(stderr, MM_ERROR, ":209:can't save original sdevice file %s", fnam);
}


write_sdev(drv)
driver_t *drv;
{
	FILE *fp;
	char nam[PATH_MAX];
	ctlr_t *ctlr;

	save_original(drv->mdev.name);

	sprintf(nam, "%s/sdevice.d/%s", instroot, drv->mdev.name);
	fp = fopen(nam, "w");

	if (fp == NULL)
	{
		perror(nam);
		exit(1);
	}

	fprintf(fp, "# DO NOT EDIT THIS FILE MANUALLY!\n");
	fprintf(fp, "# Use /sbin/dcu to cause the values in sdevice files\n");
	fprintf(fp, "# to be changed.\n");
	fprintf(fp, "#\n");
	fprintf(fp, "# automatically generated by idconfupdate\n");
	fprintf(fp, "# use /etc/conf/bin/idinstall -gs %s\n", drv->mdev.name);
	fprintf(fp, "# to view the original contents of this file\n");

	fprintf(fp, "$version %d\n", ftypes[SDEV].cur_ver);
	if (drv->mdev.over != -1)
		fprintf(fp, "$oversion %d\n", drv->mdev.over);

	if (drv->conf_static)
		fprintf(fp, "$static\n");

	for (ctlr = drv->ctlrs; ctlr; ctlr = ctlr->drv_link)
	{
		fprintf(fp,
		"%s\t%c\t%ld\t%hd\t%hd\t%hd\t%lX\t%lX\t%lX\t%lX\t%hd",
			ctlr->sdev.name,
			ctlr->sdev.conf,
			ctlr->sdev.unit,
			ctlr->sdev.ipl,
			ctlr->sdev.itype,
			ctlr->sdev.vector,
			ctlr->sdev.sioa,
			ctlr->sdev.eioa,
			ctlr->sdev.scma,
			ctlr->sdev.ecma,
			ctlr->sdev.dmachan);
		if (ctlr->sdev.bind_cpu != -1)
			fprintf(fp, "\t%d\n", ctlr->sdev.bind_cpu);
		else
			fprintf(fp, "\n");
	}

	fclose(fp);
}


get_resmgr_entry(sp)
struct sdev *sp;
{
	int ret;

	while (1)
	{
		ret = ioctl(rmfd, RMIOC_NEXTKEY, &rma);

		if (ret != 0)
		{
			if (errno == ENOENT)
				return 0;	/* no more keys */

			perror("RMIOC_NEXTKEY");
			exit(1);
		}

		if (!rm_getval(CM_MODNAME, 0))
			continue;

		strcpy(sp->name, rma.rm_val);

		sp->over = ftypes[SDEV].cur_ver;
		sp->conf = 'Y';
		sp->conf_static = 0;
		sp->unit = 0;
		sp->ipl = 0;
		sp->itype = 0;
		sp->vector = 0;
		sp->sioa = 0;
		sp->eioa = 0;
		sp->scma = 0;
		sp->ecma = 0;
		sp->dmachan = -1;
		sp->bind_cpu = -1;

		if (rm_getval(CM_UNIT, 0))
			sp->unit = *((cm_num_t *) rma.rm_val);

		if (rm_getval(CM_IPL, 0))
			sp->ipl = *((cm_num_t *) rma.rm_val);

		if (rm_getval(CM_ITYPE, 0))
			sp->itype = *((cm_num_t *) rma.rm_val);

		if (rm_getval(CM_IRQ, 0))
			sp->vector = *((cm_num_t *) rma.rm_val);

		if (rm_getval(CM_DMAC, 0))
			sp->dmachan = *((cm_num_t *) rma.rm_val);

		if (rm_getval(CM_IOADDR, 0))
		{
			struct cm_addr_rng *p = rma.rm_val;

			sp->sioa = p->startaddr;
			sp->eioa = p->endaddr;
		}

		if (rm_getval(CM_MEMADDR, 0))
		{
			struct cm_addr_rng *p = rma.rm_val;

			sp->scma = p->startaddr;
			sp->ecma = p->endaddr;
		}

		if (rm_getval(CM_BINDCPU, 0))
			sp->bind_cpu = *((cm_num_t *) rma.rm_val);

		return 1;
	}
}


update_sdevice_files()
{
	struct sdev sdev;
	driver_t *drv;

	read_mdev();
	rddrvmap();
	read_sdev();

	rma.rm_key = NULL;

	while (get_resmgr_entry(&sdev))	   /* for every entry of interest */
	{
		drv = mfind(sdev.name);
		if (drv == NULL)
			continue;

		if (drv->conf_upd == 0)
		{
			drv->conf_upd = 1;	/* "processed" */
			throw_away_sdevs(drv);
		}

		add_sdev(&sdev, drv);
	}

	/* for every mdev entry of interest */

	for (drv = driver_info; drv; drv = drv->next)
	{
		if (drv->conf_upd == 0)		/* not processed */
		{
			turn_off_sdevs(drv);
		}

		write_sdev(drv);
	}
}


write_db_param(param, name)
char *param;
char *name;
{
	size_t len;

	memset(rma.rm_param, 0, RM_MAXPARAMLEN);
	strcpy(rma.rm_param, param);
	write_mem(rma.rm_param, RM_MAXPARAMLEN);

	len = strlen(name) + 1;

	write_mem(&len, sizeof(size_t));
	write_mem(name, len);

	write_mem(&endofvals, sizeof(size_t));
}


write_db_num(param, data)
char *param;
cm_num_t data;
{
	size_t len;

	memset(rma.rm_param, 0, RM_MAXPARAMLEN);
	strcpy(rma.rm_param, param);
	write_mem(rma.rm_param, RM_MAXPARAMLEN);

	len = sizeof(cm_num_t);

	write_mem(&len, sizeof(size_t));
	write_mem(&data, len);

	write_mem(&endofvals, sizeof(size_t));
}


write_db_rng(param, start, end)
char *param;
long start;
long end;
{
	size_t len;
	struct cm_addr_rng data;

	memset(rma.rm_param, 0, RM_MAXPARAMLEN);
	strcpy(rma.rm_param, param);
	write_mem(rma.rm_param, RM_MAXPARAMLEN);

	data.startaddr = start;
	data.endaddr = end;

	len = sizeof(struct cm_addr_rng);

	write_mem(&len, sizeof(size_t));
	write_mem(&data, len);

	write_mem(&endofvals, sizeof(size_t));
}


initialize_db()
{
	driver_t *drv;
	ctlr_t *ctlr;
	struct sdev *sp;
	rm_key_t key;
	int n = 1;

	read_mdev();
	read_sdev();

	for (drv = driver_info; drv; drv = drv->next)
	{
		for (ctlr = drv->ctlrs; ctlr; ctlr = ctlr->drv_link)
		{
		    if (ctlr->sdev.conf == 'N')
			continue;

		    sp = &ctlr->sdev;

		    key = (rm_key_t) n++;
		    write_mem(&key, sizeof(rm_key_t));

		    write_db_param(CM_MODNAME, sp->name);
		    write_db_num(CM_UNIT, sp->unit);

		    if (sp->itype)
			write_db_num(CM_ITYPE, sp->itype);

		    if (sp->ipl)
			write_db_num(CM_IPL, sp->ipl);

		    if (sp->vector)
			write_db_num(CM_IRQ, sp->vector);

		    if (sp->dmachan != -1)
			write_db_num(CM_DMAC, sp->dmachan);
		    if (sp->eioa)
			write_db_rng(CM_IOADDR, sp->sioa, sp->eioa);
		    if (sp->ecma)
			write_db_rng(CM_MEMADDR, sp->scma, sp->ecma);
		    if (sp->bind_cpu != -1)
			write_db_num(CM_BINDCPU, sp->bind_cpu);

/*
 *  Flag this as a default entry
 */
		    write_db_num(CM_ENTRYTYPE, CM_ENTRY_DEFAULT);

		    write_mem(endofparams, RM_MAXPARAMLEN);
		}
	}

	key = (rm_key_t) 0;
	write_mem(&key, sizeof(rm_key_t));

	dump_mem(save_file);
}


void *
malloc_fe(len)
size_t len;
{
	void *p;

	p = malloc(len);

	if (p == NULL)
	{
		pfmt(stderr, MM_ERROR, ":131:can't malloc %d bytes\n", len);
		exit(1);
	}

	return p;
}


void *
realloc_fe(p, len)
void *p;
size_t len;
{

	if (p == NULL)
		p = malloc(len);
	else
		p = realloc(p, len);

	if (p == NULL)
	{
		pfmt(stderr, MM_ERROR, ":131:can't malloc %d bytes\n", len);
		exit(1);
	}

	return p;
}


char *
str_save(s)
char *s;
{
	char *p;

	p = malloc_fe(strlen(s)+1);
	strcpy(p, s);

	return p;
}


static char *write_ptr = NULL;
static size_t write_ptr_len = 0;
static size_t write_ptr_alloc = 0;


write_mem(ptr, len)
char *ptr;
size_t len;
{

	while (len + write_ptr_len > write_ptr_alloc)
	{
		if (write_ptr_alloc == 0)
			write_ptr_alloc = 16384;
		else
			write_ptr_alloc *= 2;

		write_ptr = realloc_fe(write_ptr, write_ptr_alloc);
	}

	memcpy(write_ptr + write_ptr_len, ptr, len);
	write_ptr_len += len;
}


dump_mem(fnam)
char *fnam;
{
	int fd;
	int n;

	fd = open(fnam, O_WRONLY | O_CREAT | O_TRUNC, 0644);

	if (fd < 0)
	{
		perror(fnam);
		exit(1);
	}

	n = write(fd, write_ptr, write_ptr_len);

	if (n != write_ptr_len)
	{
		pfmt(stderr, MM_ERROR, "::write of %s failed: %s\n",
				strerror(errno));
		exit(1);
	}

	close(fd);
}


/*
 *  Read all data from /dev/resmgr and save in /stand/resmgr
 */

save_db()
{
	int n_parm, n_val;
	int ret;

	rma.rm_key = NULL;

	while (1)
	{
		ret = ioctl(rmfd, RMIOC_NEXTKEY, &rma);
		if (ret != 0)
		{
			if (errno == ENOENT)	/* all done */
			{
				rma.rm_key = 0;
				write_mem(&rma.rm_key, sizeof(rm_key_t));

				break;
			}

			perror("RMIOC_NEXTKEY");
			exit(1);
		}

		write_mem(&rma.rm_key, sizeof(rm_key_t));

		if (rma.rm_key == NULL)		/* all done */
			break;

/*
 *  Step through all the params for this key
 */

		rma.rm_n = n_parm = 0;

		while (ioctl(rmfd, RMIOC_NEXTPARAM, &rma) == 0)
		{
			write_mem(rma.rm_param, RM_MAXPARAMLEN);

/*
 *  Get all the values for this param.
 *
 *  REM: rm_n is used for both NEXTPARAM and GETVAL so
 *  reset within each loop.
 */

			n_val = 0;

			while (rm_getval(NULL, n_val++))
			{
			    write_mem(&rma.rm_vallen, sizeof(size_t));
			    write_mem(rma.rm_val, rma.rm_vallen);
			}

			write_mem(&endofvals, sizeof(size_t));
			rma.rm_n = ++n_parm;
		}

		if (errno != ENOENT)
		{
			perror("RMIOC_NEXTPARAM");
			exit(1);
		}

		write_mem(endofparams, RM_MAXPARAMLEN);
	}

	dump_mem(save_file);
}
