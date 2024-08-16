/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:common/cmd/crash/lidcache.c	1.2"
#ident	"$Header: lidcache.c 1.1 91/07/23 $"


#include <sys/param.h>
#include <sys/sysmacros.h>
#include <a.out.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/var.h>
#include "crash.h"
#include <sys/time.h>
#include <mac.h>

struct syment *Lidvp	    = NULL;	/* lid.internel vnode pointer */
struct syment *Lvls_hdr_blk = NULL;	/* beiginning of lid cache    */
struct syment *Mac_cachel   = NULL;	/* cache length		      */

char   *heading1 = "             LAST REFERENCE TIME FL\n";
char   *heading2 = "ROW       LID     (SEC)   (NSEC) AG CLASS CATEGORIES\n\n";

/*
 * Get and print the lidvp and lid cache
 */

int
getlidcache()
{
	int	row;			/* for looping through lid cache  */
	int	clength;		/* the lid cache length		  */
	struct	vnode	     *lidvp;   	/* lid.internal vnode pointer	  */
	struct	mac_cachent  *searchp;	/* initial search pointer	  */
	struct	mac_cachent  *cachentp;	/* subsequent searches		  */
	struct	mac_cachent  cachent;	/* subsequent searche structure   */
	struct	lvls_hdr_blk lvls_hdr_blk;	/* initial search entry   */
	struct	lvls_hdr_blk *lvls_hdr_blkp;	/* subsequent search hash */

	/* get the MAC LID translation file vnode pointer */
	if (!Lidvp)
		if (!(Lidvp = symsrch("mac_lidvp")))
			error("lid.internal vnode pointer not found in symbol table\n");
	readmem(Lidvp->n_value, 1, -1, (char *)&lidvp, sizeof(struct vnode *),
		"lid.internal vnode pointer");


	/* get the lenght of the lid cache */
	if (!Mac_cachel)
		if (!(Mac_cachel = symsrch("mac_cachel")))
			error("lid cache length not found in symbol table\n");
	readmem(Mac_cachel->n_value, 1, -1, (char *)&clength, sizeof clength,
		"lid cache length");


	/* get the initial search point of lid cache */
	if (!Lvls_hdr_blk)
		if (!(Lvls_hdr_blk = symsrch("lvls_hdr_blk")))
			error("begining of cache not found in symbol table\n");
	readmem(Lvls_hdr_blk->n_value, 1, -1, (char *)&lvls_hdr_blk,
		sizeof(struct lvls_hdr_blk), "begining of lid cache");
	if ((lvls_hdr_blkp = (struct lvls_hdr_blk *)malloc(clength *
	     sizeof(struct lvls_hdr_blk))) == NULL)
		error("unable to allocate space for reading lidcache\n");
	readmem(lvls_hdr_blk.lvls_start, 1, -1, (char *)lvls_hdr_blkp,
		sizeof(struct lvls_hdr_blk) * clength, "lid cache hash list");

	fprintf(fp, "LID.INTERNAL VNODE POINTER = 0x%x\n", lidvp);
	fprintf(fp, "LID CACHE LENGTH = %d\n", clength);
	fprintf(fp, heading1);
	fprintf(fp, heading2);

	for (row = 0; row < clength; row++) {
		if ((searchp = lvls_hdr_blkp[row].lvls_start) == NULL)
			 continue;
		readmem(lvls_hdr_blkp[row].lvls_start, 1,-1, (char *)&cachent,
			sizeof(struct mac_cachent),"initial cache row entry");
		searchp = cachent.ca_next;
		cachentp = cachent.ca_next;
		do {
			readmem(cachentp, 1, -1, (char *)&cachent,
				sizeof(struct mac_cachent), "cache entry");
			prt_cachent(row, &cachent);
			cachentp = cachent.ca_next;
		} while (cachentp != searchp);
	}
}

/*
 * Print out a single lid cache entry.
 */

prt_cachent(row, entryp)
int row;
struct mac_cachent *entryp;
{
	ushort	*catsigp;	/* ptr to category significance array entry */
	ulong	*catp, i;	/* ptr to category array entry */
	int	firstcat = 1;

	if (entryp->ca_lid == 0)
		return 0;
	fprintf(fp,"[%2d] %8d %9d %9d %c  %3d  ", row, entryp->ca_lid,
		entryp->ca_lastref.tv_sec, entryp->ca_lastref.tv_nsec,
		entryp->ca_level.lvl_valid, entryp->ca_level.lvl_class);

	/*
	 * Print out the category numbers in effect.
	 */
	for (catsigp = &entryp->ca_level.lvl_catsig[0],
		catp = &entryp->ca_level.lvl_cat[0];
	     *catsigp != 0; catsigp++, catp++) {
		for (i = 0; i < NB_LONG; i++) {
			if (*catp & (((ulong)1 << (NB_LONG - 1)) >> i)) {
				if (!firstcat)
					fprintf(fp,",");
				/* print the category # */
				fprintf(fp,"%d",
					((ulong) (*catsigp-1) << CAT_SHIFT)
					+ i + 1);
				firstcat = 0;
			}
		}
	}
	fprintf(fp,"\n");
}
