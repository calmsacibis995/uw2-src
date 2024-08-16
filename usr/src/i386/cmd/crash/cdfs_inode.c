/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:i386/cmd/crash/cdfs_inode.c	1.3"
#ident	"$Header: cdfs_inode.c 1.1 91/07/23 $"

/*
 * This file contains code for the crash functions:  cdfs_inode.
 */
#include <sys/param.h>
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
#include <sys/fs/cdfs_inode.h>
#include <sys/fs/xnamnode.h>
#include <sys/cred.h>
#include <sys/stream.h>
#include <stdlib.h>

#include "crash.h"

struct syment *S_cdfsvnodeops;
struct syment *S_cdfs_ifree, *cdfs_Inode, *cdfs_Inode_cnt;

struct cdfs_inode	cdfs_ibuf;		/* buffer for cdfs_inodes */
struct cdfs_inode	*cdfs_FreeInode;	/* buffer for free inodes */
struct vnode 		vnode;
long 			cdfs_ifree;		/* inode free list */
uint 			ninode;
int 			freeinodes;		/* free inodes */
long 			iptr;

struct listbuf {
        long    addr;
        char    state;
};

/* get arguments for cdfs inode function */
int
get_cdfs_inode()
{
	int slot = -1;
	int full = 0;
	int all = 0;
	int phys = 0;
	long addr = -1;
	long arg1 = -1;
	long arg2 = -1;
	int free = 0;
	long next;
	int list = 0;
	int i, c;
	struct inode *freelist;
	struct listbuf  *listptr;
        struct listbuf  *listbuf;

	char *heading = 
	    "SLOT  Sect#/Offset  RCNT  LINK     UID     GID     SIZE TYPE  MODE   FLAGS\n";

	if (!(S_cdfs_ifree = symsrch("cdfs_InodeFree")))
		 error("cdfs_InodeFree not found in symbol table\n");
	if (!(S_cdfsvnodeops = symsrch("cdfs_vnodeops")))
		 error("cdfs_vnodeops not found in symbol table\n");
	if (!(cdfs_Inode = symsrch("cdfs_InodeCache")))
		 error("cdfs_inode not found in symbol table\n");
	if (!(cdfs_Inode_cnt = symsrch("cdfs_InodeCnt")))
		 error("cdfs_inode not found in symbol table\n");

	optind = 1;
	while((c = getopt(argcnt,args,"efprlw:")) !=EOF) {
		switch(c) {
			case 'e' :	all = 1;
					break;
			case 'f' :	full = 1;
					break;
			case 'p' :	phys = 1;
					break;
			case 'r' :	free = 1;
					break;
			case 'l' :	list = 1;
					break;
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}

	readmem((long)cdfs_Inode_cnt->n_value,1,-1, (char *)&ninode,
                 sizeof (ninode), "CDFS Inode");
	readmem((long)S_cdfs_ifree->n_value,1,-1, (char *)&cdfs_ifree,
                 sizeof (cdfs_ifree), "CDFS Free Inode");

	readmem((long)(cdfs_Inode->n_value),1,-1,(char *)&iptr, sizeof iptr,
	 "CDFS inode");

	listbuf = listptr = (struct listbuf *)malloc(sizeof(struct listbuf)*ninode);
        if (listbuf == NULL) {
		fprintf(fp, "Could not allocate space for CDFS inode buffer\n");
                return;
        }
	addr = iptr;
	for (i = 0; i < ninode; i++, listptr++) {
		readmem((long)(iptr + i * sizeof cdfs_ibuf), 1,-1,
			 (char *)&cdfs_ibuf, sizeof cdfs_ibuf, "CDFS inode table");
		listptr->addr = iptr;
                listptr->state = 'n';           	/* unknown state */
		listptr->addr += i * (sizeof cdfs_ibuf);
		vnode = cdfs_ibuf.i_VnodeStorage;
                if ((long)vnode.v_op != S_cdfsvnodeops->n_value) {
                        listptr->state = 'x'; 		/* not cdfs */
                        continue;
                }
                if (cdfs_ibuf.i_VnodeStorage.v_count != 0)
                        listptr->state = 'u';           /* in use */
        }
	if(list)
		list_cdfs_inode(listbuf);
	else {
		fprintf(fp,"INODE TABLE SIZE = %d\n", ninode);
		if(!full)
			fprintf(fp,"%s",heading);
		if(free) {
			freelist = (struct inode *)cdfs_ifree;
			next = (long)freelist;
			for (i=0; i< freeinodes; i++) {	
				pr_cdfs_inode(1,full,slot,phys,next,heading);
				next = (long)cdfs_ibuf.i_FreeFwd;
			}
		} else if(args[optind]) {
			all = 1;
			do {
				getargs(ninode,&arg1,&arg2);
				if(arg1 == -1) 
					continue;
				if(arg2 != -1)
					for(slot = arg1; slot <= arg2; slot++) {
						addr = listbuf[slot].addr;
						pr_cdfs_inode(all,full,slot,phys,addr,heading);
				} else {
					if(arg1 >= 0 && arg1 < ninode) {
						slot = arg1;
					} else {
						addr = arg1;
						slot = get_cdfs_ipos(addr,listbuf,ninode);
					}
					pr_cdfs_inode(all,full,slot,phys,addr,heading);
				}
				slot = addr = arg1 = arg2 = -1;
			}while(args[++optind]);
		} else {
			listptr = listbuf;
			for(slot = 1; slot < ninode; slot++,listptr++)
				pr_cdfs_inode(all,full,slot,phys,listptr->addr,heading);
		}
	}
	if (listbuf != NULL) {
                cdfs_free((void *)listbuf);
                listbuf = listptr = NULL;
        }

}

cdfs_free(ptr)
void *ptr;
{
	free(ptr);
}

int
list_cdfs_inode(listbuf)
struct listbuf listbuf[];
{
	struct listbuf  *listptr;
	int i,j;
	long next;
	struct cdfs_inode *cdfs_freelist;

	if (listbuf == NULL)
                return;

	cdfs_freelist = (struct cdfs_inode *)cdfs_ifree;
        next = (long)cdfs_freelist;
        for (i = 0; i < freeinodes; i++) {
                i = get_cdfs_ipos((long)next,listbuf,ninode);
                readmem((long)next,1,-1,(char *)&cdfs_ibuf,sizeof cdfs_ibuf,"CDFS inode");
                if( listbuf[i].state == 'u' )
                        listbuf[i].state = 'b';
                else
                        if( listbuf[i].state  != 'x' )
                                listbuf[i].state = 'f';
                next = (long)cdfs_ibuf.i_FreeFwd;
        }
	fprintf(fp,"The following cdfs inodes are in use:\n");
	for(i = 0,j = 0; i < ninode; i++) {
		if(listbuf[i].state == 'u') {
			if(j && (j % 10) == 0)
				fprintf(fp,"\n");
			fprintf(fp,"%3d    ",i);
			j++;
		}
	}
	fprintf(fp,"\n\nThe following cdfs inodes are on the freelist:\n");
	for(i = 0,j=0; i < ninode; i++) {
		if(listbuf[i].state == 'f') {
			if(j && (j % 10) == 0)
				fprintf(fp,"\n");
			fprintf(fp,"%3d    ",i);
			j++;
		}
	}
	fprintf(fp,"\n\nThe following cdfs inodes are on the freelist but have non-zero reference counts:\n");
	for(i = 0,j=0; i < ninode; i++) {
		if(listbuf[i].state == 'b') {
			if(j && (j % 10) == 0)
				fprintf(fp,"\n");
			fprintf(fp,"%3d    ",i);
			j++;
		}
	}

	fprintf(fp,"\n\nThe following cdfs inodes are in unknown states:\n");
	for(i = 0,j = 0; i < ninode; i++) {
		if(listbuf[i].state == 'n') {
			if(j && (j % 10) == 0)
				fprintf(fp,"\n");
			fprintf(fp,"%3d    ",i);
			j++;
		}
	}
	fprintf(fp,"\n");
}


/* print inode table */
int
pr_cdfs_inode(all,full,slot,phys,addr,heading)
int all,full,slot,phys;
long addr;
char *heading;
{
	char ch;
	int i;
	extern long lseek();
	cdfs_drec_t	drec;

	if (addr == -1)
		return;
	readmem(addr, 1,-1,(char *)&cdfs_ibuf,sizeof cdfs_ibuf,"cdfs inode table");
	vnode = cdfs_ibuf.i_VnodeStorage;

	if (!vnode.v_count && !all) 
			return ;
	if ((long)vnode.v_op != S_cdfsvnodeops->n_value )
		return;				/* not cdfs */

	if (full)
		fprintf(fp,"%s",heading);

	if (slot == -1)
		fprintf(fp,"  - ");
	else fprintf(fp,"%4d",slot);

	fprintf(fp," %4u,%-4u   %4u      %3d   %5d %7d  %7d",
		cdfs_ibuf.i_Fid.fid_SectNum,
		cdfs_ibuf.i_Fid.fid_Offset,
		cdfs_ibuf.i_VnodeStorage.v_count,
		cdfs_ibuf.i_LinkCnt,
		cdfs_ibuf.i_UserID,
		cdfs_ibuf.i_GroupID,
		cdfs_ibuf.i_Size);
	switch(vnode.v_type) {
		case VDIR: ch = ' d'; break;
		case VCHR: ch = ' c'; break;
		case VBLK: ch = ' b'; break;
		case VREG: ch = ' f'; break;
		case VLNK: ch = ' l'; break;
		case VFIFO: ch = ' p'; break;
		case VXNAM:
			 switch(cdfs_ibuf.i_DevNum) {
				case XNAM_SEM: ch = ' s'; break;
				case XNAM_SD: ch = ' m'; break;
				default: ch = ' -'; break;
			}
		default:    ch = ' -'; break;
	}
	fprintf(fp,"   %c  ",ch);
	fprintf(fp,"  %s%s%s",
		cdfs_ibuf.i_Mode & ISUID ? "u" : "-",
		cdfs_ibuf.i_Mode & ISGID ? "g" : "-",
		cdfs_ibuf.i_Mode & ISVTX ? "v" : "-");

	fprintf(fp,"  %s%s%s%s%s%s%s%s%s\n",
		cdfs_ibuf.i_Flags & IUPD ? "   up" : "",
		cdfs_ibuf.i_Flags & IACC ? "   ac" : "",
		cdfs_ibuf.i_Flags & ICHG ? "   ch" : "",
		cdfs_ibuf.i_Flags & IFREE ? "   fr" : "",
		cdfs_ibuf.i_Flags & INOACC ? "   na" : "",
		cdfs_ibuf.i_Flags & ISYNC ? "   sc" : "",
		cdfs_ibuf.i_Flags & IMODTIME ? "   mt" : "",
		cdfs_ibuf.i_Flags & IMOD ? "   md" : "",
		cdfs_ibuf.i_Flags & CDFS_INODE_HIDDEN ? "   hd" : "",
		cdfs_ibuf.i_Flags & CDFS_INODE_ASSOC ? "   as" : "",
		cdfs_ibuf.i_Flags & CDFS_INODE_RRIP_REL ? "   rr" : "");
	if(!full)
		return;

	drec = cdfs_ibuf.i_DirRecStorage;
	fprintf(fp,"\t    FREEFORW\t    FREEBCK\t    HASHFORW\t    HASHBCK\n");
	fprintf(fp,"\t%8x",cdfs_ibuf.i_FreeFwd);
	fprintf(fp,"\t   %8x",cdfs_ibuf.i_FreeBack);
	fprintf(fp,"\t   %8x",cdfs_ibuf.i_HashFwd);
	fprintf(fp,"\t   %8x\n",cdfs_ibuf.i_HashBack);

	fprintf(fp,"\t   NEXTBYTE   I_SLEEPLOCK   I_SPINLOCK MAPSZ  \n");
	fprintf(fp,"\t%8x   %8x   %8s   %8x\n", cdfs_ibuf.i_NextByte,
		 cdfs_ibuf.i_splock.sl_avail, cdfs_ibuf.i_mutex,
		 cdfs_ibuf.i_mapsz);

	if((vnode.v_type == VDIR) || (vnode.v_type == VREG)
		|| (vnode.v_type == VLNK)) {
		fprintf(fp, "\t NextDrec   PrevDrec   Loc    Offset\n");
		fprintf(fp,"\t%8x    %8x%8ld%8ld%8ld\n",drec.drec_NextDR,
	 drec.drec_PrevDR, drec.drec_Loc, drec.drec_Offset, drec.drec_Len); 
		fprintf(fp, "\t  Xarlen    ExtLoc    Datalen   UnitSz  Interleave \n");
		fprintf(fp,"\t%8d  %8d  %8d  %8d  %8d\n",
		drec.drec_XarLen, drec.drec_ExtLoc, drec.drec_DataLen,
		drec.drec_UnitSz, drec.drec_Interleave);
	}
	else
		fprintf(fp,"\n");

	/* print vnode info */
	fprintf(fp,"\nVNODE :\n");
	fprintf(fp,"VCNT VFSMNTED   VFSP    STREAMP VTYPE   RDEV    VDATA    VFILOCKS VFLAG   LID \n");
	cprvnode(&cdfs_ibuf.i_VnodeStorage);
	fprintf(fp,"\n");
}


get_cdfs_ipos(addr, list, max)
long    addr;
struct listbuf *list;
int     max;
{

        int     i;
        int     pos;
        struct listbuf *listptr;

        listptr = list;
        pos = -1;
        for(i = 0; i < max; i++, listptr++) {
                if (listptr->addr == addr) {
                        pos = i;
                        break;
                }
        }
        return(pos);
}
