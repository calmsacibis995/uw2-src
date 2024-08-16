/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ktool:common/ktool/idtools/idtarg/idmodload.c	1.12"
#ident	"$Header:"

#include "inst.h"
#include "defines.h"
#include "devconf.h"
#include <stdio.h>
#include <sys/param.h>
#include <sys/mod.h>
#include <errno.h>
#include <locale.h>
#include <pfmt.h>

#define USAGE	":116:Usage:\tidmodload [-r root] [-f modlist] [-#]\n"
#define MODLIST	"/etc/loadmods"
#define	SYNTAX	"%d:%hu:%[^:]:%s"
#define ENTLEN	128

#define	VALID_MOD(drv)	(((drv)->n_ctlr > 0 && (drv)->loadable) ? 1 : 0)

/* descriptions of module types used in error message */
char *type_name[] = {
	"None",
	"Character Device Driver",
	"Block Device Driver",
	"STREAMS Module",
	"File System Type",
	"STREAMS Device Driver",
	"Miscellaneous Module",
	"Exec Module"
};

#define TYPENAME(d)	((d)->mdev.modtype[0] == '\0' ? \
				type_name[(d)->modtype] : (d)->mdev.modtype)

extern char *optarg;
extern char instroot[];
extern char pathinst[];
extern char current[];
int	debug = 0;

driver_t *driver_info_tail;

main(argc, argv)
int argc;
char *argv[];
{
	int c;
	driver_t *drv;
	char modname[NAMESZ];
	char buf[ENTLEN];
	char modlist[LINESZ] = MODLIST, root[LINESZ];
	char typed[20];
	unsigned int cmd;
	unsigned short type;
	int nfield;
	FILE *fp;
	int error, lerror;
	short rflag;

	umask(022);

        (void) setlocale(LC_ALL, "");
        (void) setcat("uxidtools");
        (void) setlabel("UX:idmodload");

	rflag = 0;

	while ((c = getopt(argc, argv, "f:r:?#")) != EOF)
		switch (c) {
		case 'f':
			strcpy(modlist, optarg);
			break;
		case 'r':
			rflag++;
			strcpy(root, optarg);
			break;
		case '#':
			debug++;
			break;
		default:
			pfmt(stderr, MM_ACTION, "%s", USAGE);
			exit(1);
		}

	getcwd(current, LINESZ);
	getpath(rflag, root, ROOT);
	strcpy(instroot, root);
	sprintf(pathinst, "%s/%s", root, CFDIR);

	ignore_directives = 1;

	if ((fp = fopen(modlist, "r")) == NULL) {
		pfmt(stderr, MM_ERROR, ":272:Cannot open %s for read.\n",
					modlist);
		exit(1);
	}

	lerror = error = 0;

	while (fgets(buf, LINESZ, fp) != NULL) {
		if (buf[0] == '#')
			continue;
		nfield = sscanf(buf, SYNTAX, &type, &cmd, &modname, &typed);
		if (nfield != 4) {
			fprintf(stderr, "%s\n", buf);
			pfmt(stderr, MM_WARNING, ":273:WARNING: number of fields is incorrect, ignored.\n");
			continue;
		}
		if (strlen(modname) >= NAMESZ) {
			pfmt(stderr, MM_ERROR, ":274:%s: module name too long.\n",
				modname);
			error++;
			continue;
		}
		add_drv(modname, type);
	}
		
	fclose(fp);

	get_sdev();

	for (drv = driver_info; drv != NULL; drv = drv->next) {
		if (!VALID_MOD(drv)) {
			if (debug)
				fprintf(stderr,
"%s: invalid module name, not loadable, or turned off in configuration file\n",
					drv->mdev.name);
			continue;
		}

		if (drv->conf_static)
		{
			if (debug)
				fprintf(stderr, "%s: module is static, not attempting load\n", drv->mdev.name);
			continue;
		}

		if (modload(drv->mdev.name) < 0) {
			pfmt(stderr, MM_ERROR, ":270:%s \"%s\" failed to load: %s\n",
				TYPENAME(drv),
				drv->mdev.name,
				strerror(errno));
			lerror++;
		}
	}

	if (lerror)
		pfmt(stderr, MM_ERROR,
":271:WARNING: Any application(s) or commands(s) that uses one or more of these\n\tmodules may not work correctly.\nTo fix: See the Troubleshooting section of the documentation.\n");

	if (error || lerror)
		exit(1);

	exit(0);
}

get_sdev()
{
	driver_t *drv;
	struct sdev sdev;
	int stat;

	(void)getinst(SDEV, RESET, NULL);

	while ((stat = getinst(SDEV, NEXT, &sdev)) == 1) {
		if ((drv = mfind(sdev.name)) != NULL && sdev.conf == 'Y')
		{
			drv->n_ctlr++;
			if (sdev.conf_static)
				drv->conf_static = 1;
		}
	}
	if (stat != 0)
		insterror(stat, SDEV, sdev.name);
}

add_drv(dev, modtype)
char *dev;
unsigned short modtype;
{
	int stat;
	struct mdev mdev;
	driver_t *drv;

	(void)getinst(MDEV_D, RESET, NULL);

	if ((stat = getinst(MDEV_D, dev, &mdev)) != 1) {
		if (stat == 0) {
			pfmt(stderr, MM_ERROR, ":269:Fail to read the Master file for module %s\n", dev);
			exit(1);
		} else
			insterror(stat, MDEV_D, dev);
	}

	if (debug)
		fprintf(stderr, "MDEV: %d: %s %s %s\n", stat, mdev.name, mdev.mflags, mdev.modtype[0] == '\0' ? "NULL" : mdev.modtype);

	if ((drv = (driver_t *)calloc(sizeof(driver_t), 1)) == NULL) {
		pfmt(stderr, MM_ERROR, TABMEM, "mdevice");
		fatal(1);
	}

	drv->mdev = mdev;
	drv->modtype = modtype;
	drv->loadable = INSTRING(mdev.mflags, LOADMOD);

	if (driver_info) {
		driver_info_tail->next = drv;
		driver_info_tail = drv;
	} else {
		driver_info = drv;
		driver_info_tail = drv;
	}
}
