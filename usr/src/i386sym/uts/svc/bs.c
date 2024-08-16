/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386sym:svc/bs.c	1.9"
#ident	"$Header: $"

/*
 * Routines for dealing with the bootstrap information.
 */

#include <io/cfg.h>
#include <io/conf.h>
#include <mem/vm_mdep.h>
#include <svc/systm.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

static void parse_device(char *, dev_t *);

/*
 * a physical device spec is  {phys_spec} :== {dev} ( {unit}, {part} )
 * eg zd(0,6)
 *    meaning: the zdc drive, unit 0 partition 6
 *
 * a logical device spec is  {logical_spec} :== {dev}{unit}|{dev}{unit}{part}
 * eg zd0
 *    meaning: the first zdc drive
 *
 * a device spec is {dev_spec} :== {phys_spec} | {logical_spec}
 */

/*
 * void
 * bootarg_parse(void)
 *
 *	Parse the bootstrap string, setting parameters like rootdev.
 *
 * Calling/Exit State:
 *
 *	Called from the initialization process when the system is
 *	still running on the boot processor.  No return value.
 */
void
bootarg_parse(void)
{
	register char *p;

	/*
	 * look for an opening paren.
	 * {dev}({unit},{part}){kern} [-r {rootdev} ... ]
	 */
	for (p = KVCD_LOC->c_boot_name; *p != '\0'; p++) {
		if (*p == '(') {
			break;
		}
	}

	/*
	 * now look for arguments.
	 */
	while (p < &KVCD_LOC->c_boot_name[BNAMESIZE]) {
		p++;
		if (*p == '-') {
			p++;
			switch (*p++) {
			case 'r':
				/*
				 * root device
				 */
				while ((*p == ' ') || (*p == '\0'))
					p++;
				parse_device(p, &rootdev);
				break;

			default:
				break;
			}
		}
	}
}


#define	P_PER_D	256	/* each disk can contain 256 partitions */

/*
 * static void parse_device(char *name, dev_t *devp)
 *	parse an option for a device specifier.
 *
 * Calling/Exit State:
 *	takes a string containing a device name in the form:
 *	xxn[nn]{sm[mm]}
 *		xx = disk device
 *		n = unit
 *		m = partition
 *	If the device is a valid one, sets *devp to its dev_t.
 */
static void
parse_device(char *name, dev_t *devp)
{
	char	device[3];	/* device prefix */
	major_t	major = (major_t)NODEV;	/* major number of device */
	int	unit;
	int	part;
	char	*p;
	int	i;

	device[0] = name[0];
	device[1] = name[1];
	device[2] = '\0';

	for (i = 0; i < bdevcnt; i++) {
		if (strcmp(bdevsw[i].d_name, device) == 0) {
			major = (major_t)i;
			break;
		}
	}
	if (major == (major_t)NODEV)
		return;

	p = &name[2];
	/*
	 * parse up to 3 decimal digits for unit
	 */
	if (*p >= '0' && *p <= '9') {
		unit = *p++ - '0';
		if (*p >= '0' && *p <= '9') {
			unit = (unit*10) + (*p++ - '0');
			if (*p >= '0' && *p <= '9') {
				unit = (unit*10) + (*p++ - '0');
			}
		}
	} else
		return;
	if (*p == 's') {
		/*
		 * xxn[nn]sm[mm] spec xx = disk device
		 *                     n = unit
		 *                     m = partition
		 */
		p++;
		/*
		 * parse up to 3 decimal digits for partition
		 */
		if (*p >= '0' && *p <= '9') {
			part = *p++ - '0';
			if (*p >= '0' && *p <= '9') {
				part = (part*10) + (*p++ - '0');
				if (*p >= '0' && *p <= '9') {
					part = (part*10) + (*p++ - '0');
				}
			}
		}
		if (*p != '\0')
			return;
		*devp = makedevice(major, (unit*P_PER_D)+part);
		return;
	}
	*devp = makedevice(major, (unit*P_PER_D)+0xff);
}
