/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ktool:i386at/ktool/idtools/devconf.h	1.11"
#ident	"$Header:"

/* Device-driver configuration information for ID tools. */


typedef struct per_driver driver_t;
typedef struct per_ctlr ctlr_t;


/* Per-driver table;  corresponds one-to-one to mdevice */
extern struct per_driver {
	struct mdev	mdev;		/* config info from mdevice */
	char		*fname;		/* filename for Driver.o */
	short		n_ctlr;		/* number of controllers configured */
	short		intr_decl;	/* an intr routine has been declared */
	ctlr_t		*ctlrs;		/* list of controllers configured */
	long		tot_units;	/* total units for all controllers */
	driver_t	*bdev_link;	/* link for block device list */
	driver_t	*cdev_link;	/* link for character device list */
	driver_t	*next;		/* next driver in driver_info list */
					/* sorted by mdev.order */
	unsigned short	modtype;	/* dynamic loadable module type */
	char		loadable;	/* dynamic loadable module or not */
	char		autoconf;	/* is driver autoconfigurable? */
	char		conf_upd;	/* flag for idconfupdate */
	char		conf_static;	/* flag for idconfupdate */
	char		skip;		/* skip this driver?  (-M) */
	int		bind_cpu;	/* cpu binding for this driver */
} *driver_info;

/* Block and character device lists */
extern driver_t *bdevices, *cdevices;


/* Per-controller table; corresponds one-to-one to configured sdevice lines */
extern struct per_ctlr {
	struct sdev	sdev;		/* config info from sdevice */
	driver_t	*driver;	/* link to this ctlr's driver */
	short		num;		/* ctlr # per driver (0 to n_ctlr-1) */
	ctlr_t		*drv_link;	/* link to other ctlrs w/same driver */
	ctlr_t		*vec_link;	/* link to other ctlrs sharing the
						same interrupt vector */
	ctlr_t		*next;		/* next ctlr in ctlr_info list */
} *ctlr_info;


int getdevconf();
driver_t *mfind();
ctlr_t *sfind();
