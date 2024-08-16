/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ktool:common/ktool/idtools/devconf.c	1.16"
#ident	"$Header:"

#include "inst.h"
#include "defines.h"
#include "devconf.h"
#include <unistd.h>
#include <locale.h>
#include <pfmt.h>

extern char instroot[];
extern driver_t *driver_info;
driver_t *bdevices, *cdevices;
ctlr_t *ctlr_info;
char Driver_name[48];	/* Driver_<type>.o */
int noloadable; /* Don't allow any loadable modules (used for idbuild -S) */


/* read "type" file */

static void
rdtype()
{
	struct drvtype drvtype;
	int stat;

        (void)getinst(DRVTYPE, RESET, NULL);

	strcpy(Driver_name, "Driver.o");
	if ((stat = getinst(DRVTYPE, NEXT, &drvtype)) == 1)
		sprintf(Driver_name, "Driver_%s.o", drvtype.type_name);
	else if (stat != 0 && stat != IERR_OPEN)
		insterror(stat, DRVTYPE, "");
}


/* This routine is used to search the System table for some
 * specified device.  If the device is found we return a pointer to
 * that device.  If the device is not found, we return a NULL.
 */
ctlr_t *
sfind(device)
char *device;
{
        register ctlr_t *ctlr;

	for (ctlr = ctlr_info; ctlr != NULL; ctlr = ctlr->next) {
                if (equal(device, ctlr->sdev.name))
                        return(ctlr);
        }
        return(NULL);
}



/* Enter device in block device table, bdevices, and/or character device
 * table, cdevices.
 */
static int
enter(drv)
driver_t *drv;
{
        register driver_t **drv_p;
	int d_start, d_end;

	/* Link into block and char device lists, if appropriate */
	if (INSTRING(drv->mdev.mflags, BLOCK)) {
		d_start = drv->mdev.blk_start;
		d_end = drv->mdev.blk_end;
		if (d_start < BLOW || d_end > BHIGH) {
			pfmt(stderr, MM_ERROR, IBDM, d_start, d_end, BLOW, BHIGH);
			error(1);
			return(0);
		}
		drv_p = &bdevices;
		while (*drv_p && (*drv_p)->mdev.blk_start <= d_start) {
			if (d_start <= (*drv_p)->mdev.blk_end) {
				pfmt(stderr, MM_ERROR, DBDM, d_start, d_end,
					drv->mdev.name, (*drv_p)->mdev.name);
				error(1);
				return(0);
			}
			drv_p = &(*drv_p)->bdev_link;
		}
		drv->bdev_link = *drv_p;
		*drv_p = drv;
	}
	if (INSTRING(drv->mdev.mflags, CHAR)) {
		d_start = drv->mdev.chr_start;
		d_end = drv->mdev.chr_end;
		if (d_start < CLOW || d_end > CHIGH) {
			pfmt(stderr, MM_ERROR, ICDM, d_start, d_end, CLOW, CHIGH);
			error(1);
			return(0);
		}
		drv_p = &cdevices;
		while (*drv_p && (*drv_p)->mdev.chr_start <= d_start) {
			if (d_start <= (*drv_p)->mdev.chr_end) {
				pfmt(stderr, MM_ERROR, DCDM, d_start, d_end,
					drv->mdev.name, (*drv_p)->mdev.name);
				error(0);
				return(0);
			}
			drv_p = &(*drv_p)->cdev_link;
		}
		drv->cdev_link = *drv_p;
		*drv_p = drv;
	}

        return(1);
}



/*
 *  Get device configuration info; i.e. read Master and System files.
 */

int
getdevconf(chkdev_f)
int (*chkdev_f)();
{
	struct mdev mdev;
        register driver_t *drv, **drv_p;
	struct sdev sdev;
	char fname[512], *name;
	int stat;

	static int process_sdev();

	rdtype();

        (void)getinst(MDEV_D, RESET, NULL);

	while ((stat = getinst(MDEV_D, NEXT, &mdev)) == 1) {
                if ((drv = (driver_t *)calloc(sizeof(driver_t), 1)) == NULL) {
			pfmt(stderr, MM_ERROR, TABMEM, "mdevice");
                        fatal(1);
                }

                drv->mdev = mdev;
		drv->bind_cpu = -1;

		/* Insert in proper place, according to mdev.order */
		drv_p = &driver_info;
		while (*drv_p && (*drv_p)->mdev.order >= drv->mdev.order)
			drv_p = &(*drv_p)->next;
		drv->next = *drv_p;
		*drv_p = drv;

		/*
		 * Find the appropriate Driver.o variant.
		 * Check first for the type-specific Driver_name.
		 * If not present, fall back to the default Driver.o
		 * If neither present, deconfigure the module.
		 */
		sprintf(fname, "%s/pack.d/%s/%s",
				instroot, drv->mdev.name, name = Driver_name);
		if (access(fname, F_OK) != 0) {
			sprintf(fname, "%s/pack.d/%s/%s",
					instroot, drv->mdev.name,
					name = "Driver.o");
			if (access(fname, F_OK) != 0)
				name = "";
		}
		drv->fname = name;

		/*
		 * Determine if driver should be loadable.
		 */
		drv->loadable = (INSTRING(mdev.mflags, LOADMOD) &&
				 !noloadable);
	}

	if (stat != 0)
		insterror(stat, MDEV_D, mdev.name);

	(void)getinst(SDEV, RESET, NULL);

	while ((stat = getinst(SDEV, NEXT, &sdev)) == 1)
		process_sdev(&sdev, chkdev_f);

	if (stat != 0)
		insterror(stat, SDEV, sdev.name);

	rddrvmap();

	return 0;
}


/* process an entry from sdevice */

static int
process_sdev(sdp, chkdev_f)
struct sdev *sdp;
int (*chkdev_f)();
{
        register driver_t *drv;
	register ctlr_t *ctlr;

        if ((drv = mfind(sdp->name)) == NULL) {
                pfmt(stderr, MM_ERROR, SUCH, sdp->name);
                error(1);
                return(0);
        }

	if (drv->fname[0] == '\0') {
		/* Driver not available; don't configure controller */
		return(0);
	}

	/* force module to be static if requested */
	if (sdp->conf_static)
	{
		drv->loadable = 0;
		drv->conf_static = 1;
	}

        /* check if device instance is to be configured into Kernel */
        if (sdp->conf == 'N')
                return(0);

        if (sdp->conf != 'Y') {
                pfmt(stderr, MM_ERROR, CONFD, sdp->name);
                error(1);
                return(0);
        }

	if (chkdev_f && (*chkdev_f)(drv, sdp) == 0)
		return(0);

	if ((ctlr = (ctlr_t *)malloc(sizeof(ctlr_t))) == NULL) {
		pfmt(stderr, MM_ERROR, TABMEM, "sdevice");
		fatal(1);
	}

        /* enter name in System table and update Master table */
	ctlr->next = ctlr_info;
	ctlr_info = ctlr;
        ctlr->driver = drv;
        ctlr->sdev = *sdp;
        ctlr->num = (drv->n_ctlr)++;
	ctlr->drv_link = drv->ctlrs;
	drv->ctlrs = ctlr;
        drv->tot_units += sdp->unit;
	mdep_ctlr_init(ctlr);

        /* enter name in block or character name tables */
        if (drv->n_ctlr == 1 && !enter(drv))
                return(0);

	/* bind driver if requested */
	if (sdp->bind_cpu != -1)
		drv->bind_cpu = sdp->bind_cpu;

        return(1);
}
