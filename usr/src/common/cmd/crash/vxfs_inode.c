/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:common/cmd/crash/vxfs_inode.c	1.2"
#ident	"$Header: vxfs_inode.c 1.1 91/07/23 $"

/*
 * Copyright (c) 1994 VERITAS Software Corporation.  ALL RIGHTS RESERVED.
 * UNPUBLISHED -- RIGHTS RESERVED UNDER THE COPYRIGHT
 * LAWS OF THE UNITED STATES.  USE OF A COPYRIGHT NOTICE
 * IS PRECAUTIONARY ONLY AND DOES NOT IMPLY PUBLICATION
 * OR DISCLOSURE.
 *
 * THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 * TRADE SECRETS OF VERITAS SOFTWARE.  USE, DISCLOSURE,
 * OR REPRODUCTION IS PROHIBITED WITHOUT THE PRIOR
 * EXPRESS WRITTEN PERMISSION OF VERITAS SOFTWARE.
 *
 *	       RESTRICTED RIGHTS LEGEND
 * USE, DUPLICATION, OR DISCLOSURE BY THE GOVERNMENT IS
 * SUBJECT TO RESTRICTIONS AS SET FORTH IN SUBPARAGRAPH
 * (C) (1) (ii) OF THE RIGHTS IN TECHNICAL DATA AND
 * COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013.
 *	       VERITAS SOFTWARE
 * 4800 GREAT AMERICA PARKWAY, SANTA CLARA, CA 95054
 */

#include <sys/param.h>
#include <sys/sysmacros.h>
#include <a.out.h>
#include <stdio.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/var.h>
#include <sys/vfs.h>
#include <sys/fstyp.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/vnode.h>
#include <sys/list.h>
#include <sys/acl.h>
#include <sys/fs/vx_inode.h>
#include <sys/fs/vx_fs.h>
#include <sys/privilege.h>
#include <sys/cred.h>
#include <sys/stream.h>
#include <stdlib.h>
#include "crash.h"

#define ATTRFMT  0xf0000000      		/* bits used for attr */

extern struct syment *Vfs, *Vfssw;		/* namelist symbol pointers */
struct syment *S_ifreelist, *S_iinactivelist, *S_attr_ifreelist, 
		*S_idirtypagelist, *S_vxfs_fshead, *S_vxfs_attr_fshead;
struct syment *S_vx_vnodeops, *S_vx_attrvnodeops;

struct vfssw 		vfssw[10];
struct vx_inode		vxfs_ibuf;
struct vx_inode		**vxfs_iptrs;
struct vx_inode		**vxfs_attr_iptrs;
struct vnode		vnode;
struct vx_ifreehead	ifreelist;
struct vx_ifreehead	iinactivelist;
struct vx_ifreehead	idirtypagelist;
struct vx_ifreehead	attr_ifreelist;
struct vx_fshead	vxfs_fshead;
struct vx_fshead	vxfs_attr_fshead;
ulong_t			vxfs_ninode, vxfs_attr_ninode, ninode;
long			iptr;
void			vxflagprt();
void			print_vxfs_inode();

struct iflag	{
	char	*name;
	int	value;
};

/*
 * flags in i_flag
 */

struct iflag	iflags[] = {
	"IUPD", 		IUPD,
	"IACC", 		IACC,
	"IMOD", 		IMOD,
	"ICHG", 		ICHG,
	"IATIMEMOD", 		IATIMEMOD,
	"IMTIMEMOD", 		IMTIMEMOD,
	"ITRANLAZYMOD", 	ITRANLAZYMOD,
	"IFLUSHPAGES", 		IFLUSHPAGES,
	"IDIRTYPAGES", 		IDIRTYPAGES,
	"ICLOSED", 		ICLOSED,
	"IFLUSHED", 		IFLUSHED,
	"ISHORTENED", 		ISHORTENED,
	"ISYNCWRITES", 		ISYNCWRITES,
	"IDELBUF", 		IDELBUF,
	"IGHOST", 		IGHOST,
	0, 			0
};

/*
 * flags in i_intrflag
 */

struct iflag intrflags[] = {
	"IDELXWRI", 		IDELXWRI,
	"IDELXWRIERR", 		IDELXWRIERR,
	"ILOGWRITE", 		ILOGWRITE,
	"IDELAYEDERR", 		IDELAYEDERR,
	"IPUTERROR", 		IPUTERROR,
	"ILOGWRIFLUSH", 	ILOGWRIFLUSH,
	"IASYNCUPD", 		IASYNCUPD,
	0, 			0
};

/*
 * flags in i_sflags
 */

struct iflag sflags[] = {
	"IADDRVALID", 		IADDRVALID,
	"IBAD", 		IBAD,
	"IUEREAD", 		IUEREAD,
	"INOBMAPCACHE", 	INOBMAPCACHE,
	"IBADUPD", 		IBADUPD,
	"IPUTTIME", 		IPUTTIME,
	"IATTRREM", 		IATTRREM,
	"INOBMAPCLUSTER", 	INOBMAPCLUSTER,
	0, 			0
};

static char	*heading1 = 
    "SLOT  MAJ/MIN     INUMB  RCNT  LINK    UID    GID     SIZE   TYPE       MODE\n";
static char	*heading2 = 
    "ACLS DACL  ACLENTRIES\n"; 

/*
 * Get arguments for vxfs inode.
 */
int
get_vxfs_inode()
{
	static int	loptind; 
	struct vx_fshead *fshp;
	struct vx_inode **iptrs;
	int		slot = -1;
	int		attrtoo = 0;
	int		attronly = 0;
	int		full = 0;
	int		list = 0;
	int		all = 0;
	int		phys = 0;
	long		addr = -1;
	long		arg1 = -1;
	long		arg2 = -1;
	int		lfree = 0;
	int		c;

	if (!Vfs) {
		if (!(Vfs = symsrch("rootvfs"))) {
			error("vfs list not found in symbol table\n");
		}
	}
	if (!Vfssw)  {
		if (!(Vfssw = symsrch("vfssw"))) {
			error("vfssw table not found in symbol table\n");
		}
	}
	if (!(S_vx_vnodeops = symsrch("vx_vnodeops"))) {
		error("vx_vnodeops not found in symbol table\n");
	}
	if (!(S_vx_attrvnodeops = symsrch("vx_attrvnodeops"))) {
		error("vx_attrvnodeops not found in symbol table\n");
	}
	if (!(S_vxfs_fshead = symsrch("vxfs_fshead"))) {
		error("vxfs_fshead not found in symbol table\n");
	}
	if (!(S_vxfs_attr_fshead = symsrch("vxfs_attr_fshead"))) {
		error("vxfs_attr_fshead not found in symbol table\n");
	}

	optind = 1;
	while((c = getopt(argcnt, args, "Aaefprlw:")) != EOF) {
		switch(c) {

		case 'a':	attrtoo = 1;  /* print attribute inodes too */
				break;

		case 'A':	attronly = 1;  /* print only attribute inodes */
				break;

		case 'e':	all = 1;
				break;

		case 'f':	full = 1;
				break;

		case 'l':	list = 1;
				break;

		case 'p':	phys = 1;
				break;

		case 'r':	lfree = 1;
				break;

		case 'w':	redirect();
				break;

		default:	longjmp(syn, 0);
		}
	}

	if (attrtoo && attronly) {
		fprintf(fp, "Options -A and -a are mutually exclusive.\n");
		return 1;
	}

	readmem(Vfssw->n_value, 1, -1, (char *)&vfssw,
		sizeof(vfssw), "Vfssw table");

	readmem(S_vxfs_fshead->n_value, 1, -1, (char *)&vxfs_fshead,
		 sizeof(vxfs_fshead), "vxfs inode table head");

	readmem(S_vxfs_attr_fshead->n_value, 1, -1, (char *)&vxfs_attr_fshead,
		 sizeof(vxfs_attr_fshead), "vxfs attribute inode table head");

	vxfs_ninode = vxfs_fshead.f_max;
	vxfs_attr_ninode = vxfs_attr_fshead.f_max;
	vxfs_iptrs = (struct vx_inode **)
			malloc(sizeof (struct vx_inode *) * vxfs_ninode);
	if (attrtoo || attronly) {
		vxfs_attr_iptrs = (struct vx_inode **)
			malloc(sizeof (struct vx_inode *) * vxfs_attr_ninode);
	}
	if (vxfs_iptrs == NULL ||
	    ((attrtoo || attronly) && vxfs_attr_iptrs == NULL)) {
		fprintf(fp, "Could not allocate space for vxfs inode pointers.\n");
		return 1;
	}

	readmem((long)(vxfs_fshead.f_iptrs), 1, -1, (char *)vxfs_iptrs,
		sizeof (struct vx_inode *) * vxfs_ninode, "vxfs inode pointers");

	if (attrtoo || attronly) {
		readmem((long)(vxfs_attr_fshead.f_iptrs), 1, -1,
			(char *)vxfs_attr_iptrs,
			sizeof (struct vx_inode *) * vxfs_attr_ninode,
			"vxfs attribute inode pointers");
	}

	if (attronly) {
		attrtoo = 2;
	}

	/*
	 * Save the optind so we can interate over the selected
	 * inodes again for the attrtoo case.
	 */

	loptind = optind;

attrloop:
	optind = loptind;
	if (attrtoo > 1) {
		fshp = &vxfs_attr_fshead;
		iptrs = vxfs_attr_iptrs;
	} else {
		fshp = &vxfs_fshead;
		iptrs = vxfs_iptrs;
	}
	if (list) {
		list_vxfs_inode(iptrs, fshp->f_curr,
				(fshp == &vxfs_attr_fshead) ? "attribute " : "");
	} else {
		fprintf(fp, "%sINODE TABLE SIZE = %d\n",
			(fshp == &vxfs_attr_fshead) ? "ATTRIBUTE " : "",
			fshp->f_curr);
		fprintf(fp, "INODE SIZE = %d\n", fshp->f_isize);
		if (!full) {
			(void) fprintf(fp, "%s", heading1);
		}
		if (lfree) {
			for (slot = 0; slot < fshp->f_curr; slot++) {
				print_vxfs_inode (1, full, slot, phys, lfree,
					*(iptrs + slot), heading1);
			}
		} else if (args[optind]) {
			all = 1;
			do {
				getargs(fshp->f_curr, &arg1, &arg2);
				if (arg1 == -1) {
					continue;
				}
				if (arg2 != -1) {
					for (slot = arg1; slot <= arg2;
								slot++) {
						print_vxfs_inode(all, full,
							slot, phys, lfree,
							*(iptrs + slot),
							heading1);
					}
				} else {
					if (arg1 >=0 && arg1 < fshp->f_curr) {
						slot = arg1;
					} else {
						addr = arg1;
						slot = getvxfs_ipos(addr,
							    iptrs,
							    fshp->f_curr);
					}
					print_vxfs_inode(all, full, slot,
						phys, lfree,
						*(iptrs + slot),
						heading1);
				}
				slot = addr = arg1 = arg2 = -1;
			} while(args[++optind]);
		} else {
			for (slot = 0; slot < fshp->f_curr; slot++) {
				print_vxfs_inode (all, full, slot, phys, lfree,
					*(iptrs + slot), heading1);
			}
		}
	}

	if (attrtoo++ == 1) {
		goto attrloop;
	}

	free(vxfs_iptrs);
	if (attronly || attrtoo) {
		free(vxfs_attr_iptrs);
	}

	return 0;
}

int
list_vxfs_inode(vxfs_iptrs, vxfs_ninode, typestr)
	struct vx_inode	**vxfs_iptrs;
	int		vxfs_ninode;
	char		*typestr;
{
	int		j;
	struct vx_inode	*addr;
	int		slot;

	fprintf(fp, "\nThe following vxfs %sinodes are in use:\n", typestr);
	for (j = 0, slot = 0; slot < vxfs_ninode; slot++) {
		addr = *(vxfs_iptrs + slot);
		readmem((caddr_t)addr, 1, -1, (char *)&vxfs_ibuf, sizeof vxfs_ibuf,
			"vxfs inode");
		if (vxfs_ibuf.av_forw == NULL) {
			if (j && (j % 5) == 0) {
				fprintf(fp, "\n");
			}
			fprintf(fp, "%4d: %8x  ", slot, addr);
			j++;
		}
	}
	fprintf(fp, "\n\nThe following %svxfs inodes are on the freelist:\n",
		typestr);
	for (j = 0, slot = 0; slot < vxfs_ninode; slot++) {
		addr = *(vxfs_iptrs + slot);
		readmem((caddr_t)addr, 1, -1, (char *)&vxfs_ibuf, sizeof vxfs_ibuf,
			"vxfs inode");
		if (vxfs_ibuf.av_forw != NULL) {
			if (j && (j % 5) == 0) {
				fprintf(fp, "\n");
			}
			fprintf(fp, "%4d: %8x  ", slot, addr);
			j++;
		}
	}
	fprintf(fp, "\n");

	return 0;
}

/*
 * Print vxfs inode table.
 */

void
print_vxfs_inode (all, full, slot, phys, free, addr, heading)
	int	all, full, slot, phys, free;
	long	addr;
	char	*heading;
{
	struct vnode 		*vp = &vxfs_ibuf.i_vnode;
	char			extbuf[50];
	char			ch;
	char			typechar;
	int			i;
	extern long		lseek();
	int			attr_type;
	int			defflag = 0;	/* default ACLs not found yet */
	struct acl		*aclp, *acl_end, *acl_start = NULL;
	char			*aclbuf;
	struct vx_iattr		*ap;
	struct vx_attr_immed	*iap;
	int			aoff;
	struct vx_aclhd		*vxahd;
	int			nacl;

	if (addr == -1) {
		return;
	}

	readmem(addr, !phys, -1, (char *)&vxfs_ibuf, sizeof (struct vx_inode),
		"vxfs inode");
	if ((long)vp->v_op != S_vx_vnodeops->n_value &&
	   (long)vp->v_op != S_vx_attrvnodeops->n_value) {

		/*
		 * Not a vxfs inode.
		 */

		return;
	}

	if (!vp->v_count && !all) {
		return;
	}
	if (free && !vxfs_ibuf.av_forw) {
		return;
	}
	if (full) {
		fprintf(fp, "%s", heading);
	}
	if (slot == -1) {
		fprintf(fp, "  - ");
	} else {
		fprintf(fp, "%4d", slot);
	}

	fprintf(fp, " %4u, %-5u%7u   %3d %5d% 7d%7d %8ld",
		getemajor(vxfs_ibuf.i_dev),
		geteminor(vxfs_ibuf.i_dev),
		vxfs_ibuf.i_number,
		vp->v_count,
		vxfs_ibuf.i_nlink,
		vxfs_ibuf.i_uid,
		vxfs_ibuf.i_gid,
		vxfs_ibuf.i_size);

	attr_type = vxfs_ibuf.i_mode & ATTRFMT;
	if (attr_type) {

		/*
		 * For attribute inodes we dispense with the 
		 * keyletter presentation of the file type.
		 */

		switch(attr_type) {
			case IFFSH:
				fprintf(fp, "   %s", "IFFSH");
				break;
			case IFILT:
				fprintf(fp, "   %s", "IFILT");
				break;
			case IFIAU:
				fprintf(fp, "   %s", "IFIAU");
				break;
			case IFCUT:
				fprintf(fp, "   %s", "IFCUT");
				break;
			case IFATT:
				fprintf(fp, "   %s", "IFATT");
				break;
			case IFLCT:
				fprintf(fp, "   %s", "IFLCT");
				break;
			case IFIAT:
				fprintf(fp, "   %s", "IFIAT");
				break;
			case IFEMR:
				fprintf(fp, "   %s", "IFEMR");
				break;
			default:
				fprintf(fp, "       ");
				break;
		}
	} else {
		switch(vp->v_type) {
			case VDIR: ch = 'd'; break;
			case VCHR: ch = 'c'; break;
			case VBLK: ch = 'b'; break;
			case VREG: ch = 'f'; break;
			case VLNK: ch = 'l'; break;
			case VFIFO: ch = 'p'; break;
			case VXNAM: ch = 'x'; break;
			default:    ch = '-'; break;
		}
		fprintf(fp, "   %c", ch);
		fprintf(fp, "%s%s%s",
			vxfs_ibuf.i_mode & ISUID ? "u" : "-",
			vxfs_ibuf.i_mode & ISGID ? "g" : "-",
			vxfs_ibuf.i_mode & ISVTX ? "v" : "-");
	}

	fprintf(fp, "  %s%s%s%s%s%s%s%s%s",
		vxfs_ibuf.i_mode & IREAD         ? "r" : "-",
		vxfs_ibuf.i_mode & IWRITE        ? "w" : "-",
		vxfs_ibuf.i_mode & IEXEC         ? "x" : "-",
		vxfs_ibuf.i_mode & (IREAD >>  3) ? "r" : "-",
		vxfs_ibuf.i_mode & (IWRITE >> 3) ? "w" : "-",
		vxfs_ibuf.i_mode & (IEXEC >>  3) ? "x" : "-",
		vxfs_ibuf.i_mode & (IREAD >>  6) ? "r" : "-",
		vxfs_ibuf.i_mode & (IWRITE >> 6) ? "w" : "-",
		vxfs_ibuf.i_mode & (IEXEC >>  6) ? "x" : "-");

	fprintf(fp, "\n");

	if (!full) {
		return;
	}

	/*
	 * ACL stuff borrowed from sfs.  Incomplete in that
	 * we can only deal with attributes in the inode.
	 */

	if (!attr_type && vxfs_ibuf.i_aclcnt != -1) {
		fprintf(fp, "%s", heading2);
		fprintf(fp, "%4d %4d     ",
			vxfs_ibuf.i_aclcnt, vxfs_ibuf.i_daclcnt);

		/*
		 * print USER_OBJ ACL entry from permission bits.
		 */

		fprintf(fp, " u::%c%c%c\n", 
			vxfs_ibuf.i_mode & IREAD ? 'r' : '-',
			vxfs_ibuf.i_mode & IWRITE ? 'w' : '-',
			vxfs_ibuf.i_mode & IEXEC ? 'x' : '-');
	
		if (vxfs_ibuf.i_aclcnt == vxfs_ibuf.i_daclcnt) {

			/*
			 * No non-default ACL entries.  Print GROUP_OBJ entry
			 * from permission bits.
			 */

			fprintf(fp, "%14s g::%c%c%c\n", "", 
				vxfs_ibuf.i_mode & (IREAD >> 3) ? 'r' : '-',
				vxfs_ibuf.i_mode & (IWRITE >> 3) ? 'w' : '-',
				vxfs_ibuf.i_mode & (IEXEC >> 3) ? 'x' : '-');
		} 

		if (vxfs_ibuf.i_aclcnt == 0 || vxfs_ibuf.i_attr == NULL) {
			goto done_acl;
		}

		/*
		 * Paw through the immediate attribute area looking
		 * for any svr4 acls.
		 */

		aclbuf = (char *)malloc(vxfs_ibuf.i_attrlen);
		if (aclbuf == NULL) {
			fprintf(fp, "Cannot allocate acl buffer.\n");
			return;
		}

		readmem((long)vxfs_ibuf.i_attr, 1, -1, aclbuf,
			vxfs_ibuf.i_attrlen, "vxfs acls ");

		ap = (struct vx_iattr *)aclbuf;
		aoff = 0;
		while (aoff < vxfs_ibuf.i_attrlen && ap->a_format) {
			switch (ap->a_format) {
	
			case VX_ATTR_IMMED:
				iap = (struct vx_attr_immed *)ap->a_data;
				if (iap->aim_class == VX_ACL_CLASS &&
				    iap->aim_subclass == VX_ACL_SVR4_SUBCLASS) {
					acl_start = (struct acl *)iap->aim_data;
					acl_end =
					   (struct acl *)((long)acl_start +
						ap->a_length -
						VX_ATTROVERHEAD -
						VX_ATTR_IMMEDOVER);
				}
				break;
	
			case VX_ATTR_DIRECT:
				break;
			}
			aoff += (ap->a_length + 0x3) & ~0x3;
			ap = (struct vx_iattr *)((int)aclbuf + aoff);
		}

		if (acl_start == NULL) {
			goto done_acl;
		}
		vxahd = (struct vx_aclhd *)acl_start;
		nacl = vxahd->aclcnt;
		aclp = (struct acl *)((int)acl_start + sizeof (struct vx_aclhd));
		for (i = 0; aclp < acl_end && i < nacl; aclp++, i++) {
			if (aclp->a_type & ACL_DEFAULT) {
				if (defflag == 0) {

					/*
					 * 1st default ACL entry.  Print
					 * CLASS_OBJ & OTHER_OBJ entries 
					 * from permission bits before 
					 * default entry.
					 */

					fprintf(fp, "%14s c:%c%c%c\n", "", 
					vxfs_ibuf.i_mode & (IREAD >> 3)
						? 'r' : '-',
					vxfs_ibuf.i_mode & (IWRITE >> 3)
						? 'w' : '-',
					vxfs_ibuf.i_mode & (IEXEC >> 3)
						? 'x' : '-');
					fprintf(fp, "%14s o:%c%c%c\n", "", 
					vxfs_ibuf.i_mode & (IREAD >> 6)
						? 'r' : '-',
					vxfs_ibuf.i_mode & (IWRITE >> 6)
						? 'w' : '-',
					vxfs_ibuf.i_mode & (IEXEC >> 6)
						? 'x' : '-');
					defflag++;
				}
			}

			fprintf(fp, "%14s %s", "",
				aclp->a_type & ACL_DEFAULT ? "d:" : "");
			switch (aclp->a_type & ~ACL_DEFAULT) {
				case GROUP:
				case GROUP_OBJ:
					typechar = 'g';
					break;
				case USER:
				case USER_OBJ:
					typechar = 'u';
					break;
				case CLASS_OBJ:
					typechar = 'c';
					break;
				case OTHER_OBJ:
					typechar = 'o';
					break;
				default:
					typechar = '?';
					break;
			}
			if ((aclp->a_type & GROUP) ||
			    (aclp->a_type & USER)) {
				fprintf(fp, "%c:%d:%c%c%c\n",
				typechar,
				aclp->a_id,
				aclp->a_perm & (IREAD >> 6)
					? 'r' : '-',
				aclp->a_perm & (IWRITE >> 6)
					? 'w' : '-',
				aclp->a_perm & (IEXEC >> 6)
					? 'x' : '-');
			} else if ((aclp->a_type & USER_OBJ) ||
				(aclp->a_type & GROUP_OBJ)) {
				fprintf(fp, "%c::%c%c%c\n", 
				typechar,
				aclp->a_perm & (IREAD >> 6)
					? 'r' : '-',
				aclp->a_perm & (IWRITE >> 6)
					? 'w' : '-',
				aclp->a_perm & (IEXEC >> 6)
					? 'x' : '-');
			} else {
				fprintf(fp, "%c:%c%c%c\n", 
				typechar,
				aclp->a_perm & (IREAD >> 6)
					? 'r' : '-',
				aclp->a_perm & (IWRITE >> 6)
					? 'w' : '-',
				aclp->a_perm & (IEXEC >> 6)
					? 'x' : '-');
			}
		}
done_acl:
		if (defflag == 0) {

			/*
			 * No default ACL entries.  Print CLASS_OBJ & 
			 * OTHER_OBJ entries from permission bits now.
			 */

			fprintf(fp, "%14s c:%c%c%c\n", "", 
				vxfs_ibuf.i_mode & (IREAD >> 3) ? 'r' : '-',
				vxfs_ibuf.i_mode & (IWRITE >> 3) ? 'w' : '-',
				vxfs_ibuf.i_mode & (IEXEC >> 3) ? 'x' : '-');
			fprintf(fp, "%14s o:%c%c%c\n", "", 
				vxfs_ibuf.i_mode & (IREAD >> 6)
					? 'r' : '-',
				vxfs_ibuf.i_mode & (IWRITE >> 6)
					? 'w' : '-',
				vxfs_ibuf.i_mode & (IEXEC >> 6)
					? 'x' : '-');
		}
	}

	vxflagprt(&vxfs_ibuf);
	fprintf(fp, "\t    FORW\t    BACK\t    AFOR\t    ABCK\n");
	fprintf(fp, "\t%8x" , vxfs_ibuf.i_forw);
	fprintf(fp, "\t%8x", vxfs_ibuf.i_back);
	fprintf(fp, "\t%8x", vxfs_ibuf.av_forw);
	fprintf(fp, "\t%8x\n", vxfs_ibuf.av_back);

	if (vxfs_ibuf.i_orgtype == IORG_IMMED) {
		fprintf(fp, "\n    immediate\n");
	} else if (vxfs_ibuf.i_orgtype == IORG_EXT4) {
		for (i = 0; i < NDADDR_N; i++) {
			if (!(i % 3)) {
				fprintf(fp, "\n    ");
			} else {
				fprintf(fp, "  ");
			}
			sprintf(extbuf, "[%d, %d]", vxfs_ibuf.i_dext[i].ic_de,
				vxfs_ibuf.i_dext[i].ic_des);
			fprintf(fp, "e%d: %-19s", i, extbuf);
		}
		fprintf(fp, "\n    ie0: %-8d      ie1: %-8d      ies: %-8d\n",
			vxfs_ibuf.i_ie[0], vxfs_ibuf.i_ie[1],
			vxfs_ibuf.i_ies);
	}

	/*
	 * Print vnode info.
	 */

	fprintf(fp, "VNODE :\n");
	fprintf(fp, "VCNT VFSMNTED   VFSP    STREAMP VTYPE   RDEV    VDATA    VFILOCKS VFLAG \n");
	cprvnode(&vxfs_ibuf.i_vnode);
	fprintf(fp, "\n");
}

int
getvxfs_ipos(addr, vxfs_iptrs, vxfs_ninode)
	struct vx_inode	*addr;
	struct vx_inode	**vxfs_iptrs;
	int		vxfs_ninode;
{
	int 		slot;

	for (slot = 0; slot < vxfs_ninode; slot++) {
		if (*(vxfs_iptrs + slot) == addr) {
			return slot;
		}
	}
	return -1;
}

void
vxflagprt(ip)
	struct vx_inode	*ip;
{
	int		i;

	fprintf(fp, "FLAGS:");
	for (i = 0; iflags[i].value; i++) {
		if (ip->i_flag & iflags[i].value) {
			fprintf(fp, " %s", iflags[i].name);
		}
	}
	for (i = 0; intrflags[i].value; i++) {
		if (ip->i_intrflag & intrflags[i].value) {
			fprintf(fp, " %s", intrflags[i].name);
		}
	}
	for (i = 0; sflags[i].value; i++) {
		if (ip->i_sflag & sflags[i].value) {
			fprintf(fp, " %s", sflags[i].name);
		}
	}
	fprintf(fp, "\n");
	return;
}

int
vxfs_lck()
{
	int		slot, active = 0;
	extern		print_lock();
	struct vx_inode	*addr;
	struct vnode	*vp;

	if (!(S_vx_vnodeops = symsrch("vx_vnodeops"))) {
		error("vx_vnodeops not found in symbol table\n");
	}
	if (!(S_vxfs_fshead = symsrch("vxfs_fshead"))) {
		error("vxfs_fshead not found in symbol table\n");
	}

	readmem(S_vxfs_fshead->n_value, 1, -1, (char *)&vxfs_fshead,
		 sizeof(vxfs_fshead), "vxfs inode table head");

	vxfs_ninode = vxfs_fshead.f_max;
	vxfs_iptrs = (struct vx_inode **)
			malloc(sizeof (struct vx_inode *) * vxfs_ninode);
	if (vxfs_iptrs == NULL) {
		fprintf(fp, "Could not allocate space for vxfs inode pointers.\n");
		return 0;
	}

	readmem((long)(vxfs_fshead.f_iptrs), 1, -1, (char *)vxfs_iptrs,
		sizeof (struct vx_inode *) * vxfs_ninode, "vxfs inode pointers");

	for (slot = 0; slot < vxfs_fshead.f_curr; slot++) {
		addr = *(vxfs_iptrs + slot);
		readmem((caddr_t)addr, 1, -1, (char *)&vxfs_ibuf,
			sizeof (struct vx_inode), "vxfs inode");

		vp = &vxfs_ibuf.i_vnode;
		if ((long)vp->v_op != S_vx_vnodeops->n_value) {
	
			/*
			 * Not a vxfs inode - or at least not an inode
			 * that could have locks.
			 */
	
			continue;
		}
	
		if (vxfs_ibuf.i_mode == 0 || !vp->v_count ||
		    vxfs_ibuf.av_forw) {
			continue;
		}

		if (vp->v_filocks) {
			active += print_lock(vp->v_filocks, slot, "vxfs");
		}
	}
	free(vxfs_iptrs);
	return (active);
}
