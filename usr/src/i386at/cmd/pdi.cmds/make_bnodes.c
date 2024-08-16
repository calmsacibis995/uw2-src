/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pdi.cmds:make_bnodes.c	1.3"

#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/mkdev.h>
#include	<sys/stat.h>
#include	<mac.h>
#include	<fcntl.h>

/*
 *  Make the nodes /dev/[r]root and /dev/[r]swap
 *	add records to the passed file pointer if it is valid
 */

void
make_bnodes(char *root, major_t char_maj, major_t blk_maj, level_t level, FILE *fp, char bmkdev)
{
	dev_t dev;
	int	offset;
	char dev_name[256];

	if ( root )
		sprintf(dev_name, "%s/dev", root);
	else
		sprintf(dev_name, "/dev");

	CreateDirectory("/dev");

	offset = strlen(dev_name);

	sprintf(&dev_name[offset], "/root");
	if ( bmkdev )
		(void)unlink(dev_name);
	if (!mknod(dev_name, S_IRUSR | S_IWUSR | S_IFBLK, makedev(blk_maj,1))) {
		(void)chown(dev_name,(uid_t)0,(gid_t)3);
		(void)lvlfile(dev_name, MAC_SET, &level);
		if ( fp )
			fprintf(fp,"%s b %d %d ? ? ? %d NULL NULL\n",dev_name,blk_maj,1,level);
	} else if (fp && SpecialExists(S_IFBLK, dev_name, blk_maj, 1)) {
		fprintf(fp,"%s b %d %d ? ? ? %d NULL NULL\n",dev_name,blk_maj,1,level);
	}

	sprintf(&dev_name[offset], "/rroot");
	if ( bmkdev )
		(void)unlink(dev_name);
	if (!mknod(dev_name, S_IRUSR | S_IWUSR | S_IFCHR, makedev(char_maj,1))) {
		(void)chown(dev_name,(uid_t)0,(gid_t)3);
		(void)lvlfile(dev_name, MAC_SET, &level);
		if ( fp )
			fprintf(fp,"%s c %d %d ? ? ? %d NULL NULL\n",dev_name,char_maj,1,level);
	} else if (fp && SpecialExists(S_IFCHR, dev_name, char_maj, 1)) {
		fprintf(fp,"%s c %d %d ? ? ? %d NULL NULL\n",dev_name,char_maj,1,level);
	}

	sprintf(&dev_name[offset], "/swap");
	if ( bmkdev )
		(void)unlink(dev_name);
	if (!mknod(dev_name, S_IRUSR | S_IWUSR | S_IFBLK, makedev(blk_maj,2))) {
		(void)chown(dev_name,(uid_t)0,(gid_t)3);
		(void)lvlfile(dev_name, MAC_SET, &level);
		if ( fp )
			fprintf(fp,"%s b %d %d ? ? ? %d NULL NULL\n",dev_name,blk_maj,2,level);
	} else if (fp && SpecialExists(S_IFBLK, dev_name, blk_maj, 2)) {
		fprintf(fp,"%s b %d %d ? ? ? %d NULL NULL\n",dev_name,blk_maj,2,level);
	}

	sprintf(&dev_name[offset], "/rswap");
	if ( bmkdev )
		(void)unlink(dev_name);
	if (!mknod(dev_name, S_IRUSR | S_IWUSR | S_IFCHR, makedev(char_maj,2))) {
		(void)chown(dev_name,(uid_t)0,(gid_t)3);
		(void)lvlfile(dev_name, MAC_SET, &level);
		if ( fp )
			fprintf(fp,"%s c %d %d ? ? ? %d NULL NULL\n",dev_name,char_maj,2,level);
	} else if (fp && SpecialExists(S_IFCHR, dev_name, char_maj, 2)) {
		fprintf(fp,"%s c %d %d ? ? ? %d NULL NULL\n",dev_name,char_maj,2,level);
	}

}
