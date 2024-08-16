/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ktool:common/ktool/idtools/idtarg/idresadd.c	1.18"
#ident	"$Header: $"

#include <stdio.h>
#include <malloc.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
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


/*
 *  For each Sdev entry being added, do:
 *
 *	scan /dev/resmgr for a key with no MODNAME
 *	and values for the following which match exactly:
 *
 *	Check these for a match:
 *
 *		IRQ DMAC IOADDR MEMADDR
 *
 *	Then (destructively) add:
 *
 *		IPL ITYPE UNIT MODNAME [BINDCPU]
 *
 *	Otherwise, a new key will be created, and the above
 *	fields plus MODNAME will be added.
 */


char *rmdb_file = "/dev/resmgr";

int debug;
int rmfd;
int valbufsz;
struct rm_args rma;

extern char pathinst[];
extern char instroot[];

void *malloc_fe();
char *str_save();

ctlr_t *head_ctlr = NULL;

int nflag = 0;

main(argc, argv)
int argc;
char **argv;
{
	extern  char *optarg;
	int c;
	int errflag = 0;
	int dflag = 0;
	int fflag = 0;

	umask(022);

        (void) setlocale(LC_ALL, "");
        (void) setcat("uxidtools");
        (void) setlabel("UX:idresadd");

        while ((c = getopt(argc, argv, "ndfr:?")) != EOF)
                switch (c) {
		case 'd':
			dflag++;
			break;

		case 'f':
			fflag++;	/* inhibit check for existing entry */
			break;

		case 'n':
			nflag++;	/* don't flag new entries as default */
			break;

		case 'r':
			strcpy(instroot, optarg);
			break;

                case '?':
		default:
			errflag++;
                }

	if (optind >= argc)
		errflag++;

	if (errflag)
	{
		pfmt(stderr, MM_ACTION, ":130:Usage: %s [options] module_name\n\t-r dir\tspecify an alternate /etc/conf directory\n\t-d\tdelete entries from resmgr instead of adding\n\t-f\tadd even if there are already entries for this driver in the\n\t\tRM database\n\t-n\tdon't supply CM_ENTRY_DEFAULT on new entries; remove other\n\t\tdefault entries for this driver from the RM database\n", argv[0]);
                        exit(1);
	}

	rm_init();
	if (dflag)
		delete_resmgr(argv[optind]);
	else
	{
		if (fflag || check_existing_entries(argv[optind]) == 0)
			add_resmgr(argv[optind]);
		if (nflag)
			remove_default_entries(argv[optind]);
	}
	rm_done();

	system("/etc/conf/bin/idconfupdate -f");

	exit(0);
}


rm_init()
{

	rmfd = open(rmdb_file, O_RDWR);
	if (rmfd < 0)
	{
		perror(rmdb_file);
		exit(1);
	}

	valbufsz = 1;
	rma.rm_val = malloc_fe(valbufsz);
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


/*
 *  Make sure rma.rm_val is large enough to hold our data
 */

static
growbuf()
{

	if (valbufsz < rma.rm_vallen)
	{
		valbufsz = rma.rm_vallen;
		free(rma.rm_val);
		rma.rm_val = malloc_fe(valbufsz);
	}
}


rm_del_param(param)
char *param;
{

	memset(rma.rm_param, 0, RM_MAXPARAMLEN);
	strcpy(rma.rm_param, param);

	if (ioctl(rmfd, RMIOC_DELVAL, &rma) != 0)
	{
		perror("RMIOC_DELVAL");
		exit(1);
	}
}


rm_put_param(param, name)
char *param;
char *name;
{

	memset(rma.rm_param, 0, RM_MAXPARAMLEN);
	strcpy(rma.rm_param, param);

	rma.rm_vallen = strlen(name) + 1;
	growbuf();
	strcpy(rma.rm_val, name);

	if (ioctl(rmfd, RMIOC_DELVAL, &rma) != 0)
	{
		perror("RMIOC_DELVAL");
		exit(1);
	}

	if (ioctl(rmfd, RMIOC_ADDVAL, &rma))
	{
		perror("RMIOC_ADDVAL");
		exit(1);
	}
}


rm_put_num(param, data)
char *param;
cm_num_t data;
{

	memset(rma.rm_param, 0, RM_MAXPARAMLEN);
	strcpy(rma.rm_param, param);

	rma.rm_vallen = sizeof(cm_num_t);
	growbuf();
	*((cm_num_t *) rma.rm_val) = data;

	if (ioctl(rmfd, RMIOC_DELVAL, &rma) != 0)
	{
		perror("RMIOC_DELVAL");
		exit(1);
	}

	if (ioctl(rmfd, RMIOC_ADDVAL, &rma))
	{
		perror("RMIOC_ADDVAL");
		exit(1);
	}
}


rm_put_rng(param, start, end)
char *param;
long start;
long end;
{
	struct cm_addr_rng data;

	memset(rma.rm_param, 0, RM_MAXPARAMLEN);
	strcpy(rma.rm_param, param);

	rma.rm_vallen = sizeof(struct cm_addr_rng);
	growbuf();
	data.startaddr = start;
	data.endaddr = end;
	*((struct cm_addr_rng *) rma.rm_val) = data;

	if (ioctl(rmfd, RMIOC_DELVAL, &rma) != 0)
	{
		perror("RMIOC_DELVAL");
		exit(1);
	}

	if (ioctl(rmfd, RMIOC_ADDVAL, &rma))
	{
		perror("RMIOC_ADDVAL");
		exit(1);
	}
}


read_sdev(device)
char *device;
{
	int stat;
	struct sdev sdev;

	(void) getinst(SDEV_D, RESET, NULL);

	while ((stat = getinst(SDEV_D, device, &sdev)) == 1)
	    add_sdev(&sdev);

	if (stat != 0)
	    insterror(stat, SDEV, device);
}


add_sdev(sdp)
struct sdev *sdp;
{
	ctlr_t *ctlr;
	ctlr_t *last;

	ctlr = malloc_fe(sizeof(ctlr_t));

	for (last = head_ctlr; last && last->drv_link; last = last->drv_link)
		;

	if (last)
		last->drv_link = ctlr;
	else
		head_ctlr = ctlr;

	ctlr->drv_link = NULL;

        ctlr->sdev = *sdp;
}


nextkey(rmfd, t)
int rmfd;
struct rm_args *t;
{

	if (ioctl(rmfd, RMIOC_NEXTKEY, t))
	{
		if (errno == ENOENT)
		{
			t->rm_key = RM_NULL_KEY;
			return 0;
		}

		perror("RMIOC_NEXTKEY");
		exit(1);
	}

	return 1;
}


/*
 *  Remove any entries for the named device if they have CM_ENTRY_DEFAULT
 *  set
 */

remove_default_entries(modname)
char *modname;
{
	int ret;
	struct rm_args next;

	next.rm_key = RM_NULL_KEY;
	nextkey(rmfd, &next);

	while (next.rm_key != RM_NULL_KEY)
	{
		rma.rm_key = next.rm_key;
		nextkey(rmfd, &next);

		if (rm_getval(CM_MODNAME, 0) &&
		    strcmp(rma.rm_val, modname) == 0 &&
		    rm_getval(CM_ENTRYTYPE, 0) &&
		    *((cm_num_t *) rma.rm_val) == CM_ENTRY_DEFAULT)
		{
			if (ioctl(rmfd, RMIOC_DELKEY, &rma))
			{
				perror("RMIOC_DELKEY");
				exit(1);
			}
		}
	}
}


/*
 *  See if there is already an entry in the resource manager for
 *  the named device.  Return 0 if not, 1 if there is.
 */

check_existing_entries(modname)
char *modname;
{
	int ret;

	rma.rm_key = RM_NULL_KEY;

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

		if (strcmp(rma.rm_val, modname) == 0)
			return 1;	/* there is an existing entry */
	}
}


get_candidate(sp, modname)
struct sdev *sp;
char *modname;
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

		if (rm_getval(CM_MODNAME, 0))
		{
			if (*(char *)rma.rm_val != '\0' &&
					strcmp(rma.rm_val, "unk") != 0 &&
					strcmp(rma.rm_val, modname) != 0)
				continue;
		}

		*sp->name = '\0';
		sp->conf = 'Y';
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


/*
 *	Check these for a match:
 *
 *		IRQ DMAC IOADDR MEMADDR
 *
 *	Additionally, at least one value must be set (one must be non-default)
 */

static int
entries_match(a, b)
struct sdev *a;
struct sdev *b;
{

	if (a->vector || a->dmachan != -1 || a->eioa || a->ecma)
	{
		if (a->vector == b->vector &&
		    a->dmachan == b->dmachan &&
		    a->sioa == b->sioa &&
		    a->eioa == b->eioa &&
		    a->scma == b->scma &&
		    a->ecma == b->ecma)
			return 1;
	}

	return 0;
}


stuff_entries(sp)
struct sdev *sp;
{

	rm_put_param(CM_MODNAME, sp->name);
	rm_put_num(CM_UNIT, sp->unit);

	if (sp->itype)
		rm_put_num(CM_ITYPE, sp->itype);
	else
		rm_del_param(CM_ITYPE);

	if (sp->ipl)
		rm_put_num(CM_IPL, sp->ipl);
	else
		rm_del_param(CM_IPL);

	if (sp->vector)
		rm_put_num(CM_IRQ, sp->vector);
	else
		rm_del_param(CM_IRQ);

	if (sp->dmachan != -1)
		rm_put_num(CM_DMAC, sp->dmachan);
	else
		rm_del_param(CM_DMAC);

	if (sp->eioa)
		rm_put_rng(CM_IOADDR, sp->sioa, sp->eioa);
	else
		rm_del_param(CM_IOADDR);

	if (sp->ecma)
		rm_put_rng(CM_MEMADDR, sp->scma, sp->ecma);
	else
		rm_del_param(CM_MEMADDR);

	if (sp->bind_cpu != -1)
		rm_put_num(CM_BINDCPU, sp->bind_cpu);
	else
		rm_del_param(CM_BINDCPU);

	if (nflag)
		rm_del_param(CM_ENTRYTYPE, CM_ENTRY_DEFAULT);
}


static
create_new_key()
{

	if (ioctl(rmfd, RMIOC_NEWKEY, &rma) != 0)
	{
		perror("RMIOC_NEWKEY");
		exit(1);
	}

	/*
	 *  Flag this as a default entry unless -n is given
	 */
	if (nflag == 0)
		rm_put_num(CM_ENTRYTYPE, CM_ENTRY_DEFAULT);
}


add_resmgr(device)
char *device;
{
	struct sdev sdev;
	int found;
	ctlr_t *ctlr;

	read_sdev(device);

	for (ctlr = head_ctlr; ctlr; ctlr = ctlr->drv_link)
	{
		if (ctlr->sdev.conf != 'Y' && ctlr->sdev.conf != 'y')
			continue;

		found = 0;
		rma.rm_key = RM_NULL_KEY;
		while (get_candidate(&sdev, device))
			if (entries_match(&sdev, &ctlr->sdev))
			{
				found = 1;
				break;
			}

		if (!found)
			create_new_key();

		stuff_entries(&ctlr->sdev);
	}
}


delete_resmgr(device)
char *device;
{

	while (1)
	{
		if (ioctl(rmfd, RMIOC_NEXTKEY, &rma))
		{
			if (errno == ENOENT)
				return;
			perror("RMIOC_NEXTKEY");
			exit(1);
		}

		if (rm_getval(CM_MODNAME, 0) &&
		    strcmp(rma.rm_val, device) == 0)
		{

			if (!rm_getval(CM_BRDBUSTYPE, 0) ||
			    *((cm_num_t *) rma.rm_val) == CM_BUS_UNK ||
			    *((cm_num_t *) rma.rm_val) == CM_BUS_ISA)
			{
				if (ioctl(rmfd, RMIOC_DELKEY, &rma) != 0)
				{
					perror("RMIOC_DELKEY");
					exit(1);
				}
			}
			else
			{
				rm_del_param(CM_MODNAME);
				rm_del_param(CM_IPL);
				rm_del_param(CM_UNIT);
				rm_del_param(CM_BINDCPU);
			}
		}
	}
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


char *
str_save(s)
char *s;
{
	char *p;

	p = malloc_fe(strlen(s)+1);
	strcpy(p, s);

	return p;
}
