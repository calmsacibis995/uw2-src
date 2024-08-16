/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:common/cmd/crash/inode.c	1.1.1.6"
#ident	"$Header: inode.c 1.1 91/07/23 $"

/*
 * This file contains code for the crash functions:  vnode, inode, file.
 */
#include <sys/param.h>
#include <a.out.h>
#include <stdio.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/fs/s5dir.h>
#include <sys/var.h>
#include <sys/vfs.h>
#include <sys/fstyp.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/vnode.h>
#include <sys/list.h>
#include <sys/proc.h>
#include <sys/fs/s5inode.h>
#include <sys/fs/xnamnode.h>
#include <sys/cred.h>
#include <sys/stream.h>
#include <stdlib.h>

#include "crash.h"

extern struct syment *Vnode, *Vfs; /* namelist symbol pointers */
extern struct syment *Snode, *Sndd, *Rcvd, *Nrcvd, *Ngrps;

struct syment  *S_s5_totally_free, *S_s5_partially_free;
struct syment *S_s5vnodeops, *S_ninode, *S_s5_hash;

struct vnode 		vnbuf;			/* buffer for vnode */
struct inode 		ibuf;			/* buffer for s5inode */
struct vnode 		vnode;
struct inode_marker	s5_totally_free;	/* inode free list */
struct inode_marker 	s5_partially_free;	/* inode free list */
int 			ninode;
struct hinode  		hinode[NHINO];		/* inode hash list */ 

struct listbuf {
        long    addr;
        char    state;
};

/* get arguments for vnode function */
int
getvnode()
{
	long addr = -1;
	int phys = 0;
	int c;


	optind = 1;
	while((c = getopt(argcnt,args,"pw:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 1;
					break;
			default  :	longjmp(syn,0);
					break;
		}
	}
	if(args[optind]){
		fprintf(fp,"VCNT  VFSMNTED    VFSP      STREAMP VTYPE RDEV    VDATA   VFILOCKS VFLAG   LID\n");
		do{
			if((addr = strcon(args[optind++], 'h')) == -1)
				error("\n");
			readmem(addr,1,-1,(char *)&vnbuf,sizeof vnbuf,"vnode");
			cprvnode(&vnbuf);
		}while(args[optind]);

		fprintf(fp, "\n");
	}
	else longjmp(syn,0);

}

/*
 * Crash version of prvnode(), 
 * differs from _KERNEL __STDC__ function prototype
 * in <fs/procfs/prdata.h>
 */
cprvnode(vnptr)
struct vnode *vnptr;
{
	

	fprintf(fp,"%4d %8x %8x %8x",
		vnptr->v_count,
		vnptr->v_vfsmountedhere,
		vnptr->v_vfsp,
		vnptr->v_stream);
	switch(vnptr->v_type){
		case VREG :	fprintf(fp, " f        -    "); break;
		case VDIR :	fprintf(fp, " d        -    "); break;
		case VLNK :	fprintf(fp, " l        -    "); break;
		case VCHR :
				fprintf(fp," c %4u,%-4u",
					getemajor(vnptr->v_rdev),
					geteminor(vnptr->v_rdev));
				break;
		case VBLK :
				fprintf(fp,"  b  %4u,%-5u",
					getemajor(vnptr->v_rdev),
					geteminor(vnptr->v_rdev));
				break;
		case VFIFO :	fprintf(fp, "  p       -   "); break;
		case VNON :	fprintf(fp, "  n       -   "); break;
		default :	fprintf(fp, "  -       -   "); break;
	}
	fprintf(fp,"  %8x %8x",
		vnptr->v_data,
		vnptr->v_filocks);
	fprintf(fp,"%s",
		vnptr->v_flag & VROOT ? " root" : "  -  ");
	fprintf(fp," %4x\n",
		vnptr->v_lid);
}


/* get arguments for S5 inode function */
int
getinode()
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
	int i ;
	struct inode *ipx, *ip;
	struct hinode *ih;
	struct listbuf  *listptr;
        struct listbuf  *listbuf;

	char *heading = 
	    "SLOT  MAJ/MIN  INUMB RCNT  LINK     UID     GID     SIZE TYPE  MODE   FLAGS\n";

	if (!(S_s5_totally_free = symsrch("s5_totally_free")))
		 error("s5_totally_free not found in symbol table\n");

	if (!(S_s5_partially_free = symsrch("s5_partially_free")))
		 error("s5_partially_free not found in symbol table\n");

	if (!(S_s5_hash = symsrch("hinode")))
		 error("hinode not found in symbol table\n");

	if (!(S_s5vnodeops = symsrch("s5vnodeops")))
		 error("s5vnodeops not found in symbol table\n");

	if (!(S_ninode = symsrch("ninode")))
		 error("ninode not found in symbol table\n");
	optind = 1;
	while((i = getopt(argcnt,args,"efprlw:")) !=EOF) {
		switch(i) {
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
	readmem(S_s5_totally_free->n_value,1,-1,(char *)&s5_totally_free,
		 sizeof(s5_totally_free), "S5 free inode list");

	readmem(S_s5_partially_free->n_value,1,-1,(char *)&s5_partially_free,
		 sizeof(s5_partially_free), "S5 free inode list");

	readmem(S_s5_hash->n_value,1,-1,(char *)&hinode,
		 sizeof(hinode), "S5 hash list");

	readmem(S_ninode->n_value,1,-1,(char *)&ninode, sizeof ninode,
	 "S5 inode count");

	listbuf = listptr = (struct listbuf *)malloc(sizeof(struct listbuf)*(ninode+2));
        if (listbuf == NULL) {
		fprintf(fp, "Could not allocate space for S5 inode buffer\n");
                return;
        }
	listptr = &listptr[2];
	for (i=0; i < NHINO; i++) {
		ih = &hinode[i];
		addr = (long)ih->ih_forw;
		ipx = ih->ih_forw;
		for (ip = ipx; ;ip = (inode_t *)addr, listptr++) {
			readmem((long)ip, 1,-1, (char *)&ibuf, sizeof ibuf, "S5 inode");
			if (ibuf.i_forw == ipx)
				break;
			listptr->addr = addr;
                	listptr->state = 'n';           /* unknown state */
			vnode = ibuf.i_vnode;
                	if ((long)vnode.v_op != S_s5vnodeops->n_value) {
                        	listptr->state = 'x'; /* not s5 */
				addr = (long)ibuf.i_forw;
                        	continue;
			} else if (vnode.v_count != 0)
                        	listptr->state = 'u';           /* in use */
			addr = (long)ibuf.i_forw;
        	}
	}
	ip = s5_totally_free.im_chain[2];	
	for (;;listptr++) {
		readmem((long)ip,1,-1,(char *)&ibuf,
                               sizeof ibuf, "s5 inode");
		if (ibuf.i_freef == (inode_t *)S_s5_totally_free->n_value)
			break;
		listptr->addr = (long)ip;
		/* Structure copy */
		vnode = ibuf.i_vnode;
		if ((long)vnode.v_op != S_s5vnodeops->n_value) {
			listptr->state = 'x'; /* not s5 */
       			ip = ibuf.i_freef;
			continue;
		} else if (vnode.v_count == 0)
			listptr->state = 'f';
		else 
			listptr->state = 'b';
       		ip = ibuf.i_freef;
       	}
	ip = s5_partially_free.im_chain[2];
	for (;;listptr++) {
		readmem((long)ip,1,-1,(char *)&ibuf,
                               sizeof ibuf, "s5 inode");
		if (ibuf.i_freef == (inode_t *)S_s5_partially_free->n_value)
			break;
		listptr++;
		listptr->addr = (long)ip;
		/* Structure copy */
		vnode = ibuf.i_vnode;
		if ((long)vnode.v_op != S_s5vnodeops->n_value) {
			listptr->state = 'x'; /* not  s5 */
			ip = ibuf.i_freef;
			continue;
		} else if (vnode.v_count == 0)
			listptr->state = 'f';
		else 
			listptr->state = 'b';
		ip = ibuf.i_freef;
	}
done:
	if(list)
		listinode(listbuf);
	else {
		fprintf(fp,"INODE TABLE SIZE = %d\n", ninode);
		if(!full)
			fprintf(fp,"%s",heading);
		if(free) {
			ip = s5_totally_free.im_chain[2];

			while (ip != (struct inode *)S_s5_totally_free->n_value){
				prinode(1,full,slot,phys,ip,heading);
				ip = ibuf.i_freef;
        		}
			ip = s5_partially_free.im_chain[2];
			while (ip != (struct inode *)S_s5_partially_free->n_value){
				prinode(1,full,slot,phys,ip,heading);
                		ip = ibuf.i_freef;
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
						prinode(all,full,slot,phys,addr,heading);
				} else {
					if(arg1 >= 0 && arg1 < ninode) {
						slot = arg1;
						addr = listbuf[slot].addr;
					} else {
						addr = arg1;
						slot = gets5_ipos(addr,listbuf,ninode);
					}
					prinode(all,full,slot,phys,addr,heading);
				}
				slot = addr = arg1 = arg2 = -1;
			}while(args[++optind]);
		} else {
			listptr = &listbuf[2];
			for(slot = 2; slot < ninode; slot++,listptr++) {
				prinode(all,full,slot,phys,listptr->addr,heading);
			}
		}
	}
	if (listbuf != NULL) {
                s5free((void *)listbuf);
                listbuf = listptr = NULL;
        }

}

s5free(ptr)
void *ptr;
{
	free(ptr);
}

int
listinode(listbuf)
struct listbuf listbuf[];
{
	struct listbuf  *listptr;
	int i,j, k;
	long next;
	struct inode *s5_freelist;

	if (listbuf == NULL)
                return;
	fprintf(fp,"The following S5 inodes are in use:\n");
	for(i = 0,j = 0; i < ninode; i++) {
		if(listbuf[i].state == 'u') {
			if(j && (j % 10) == 0)
				fprintf(fp,"\n");
			fprintf(fp,"%3d    ",i);
			j++;
		}
	}
	fprintf(fp,"\n\nThe following S5 inodes are on the freelist:\n");
	for(i = 0,j=0; i < ninode; i++) {
		if(listbuf[i].state == 'f') {
			if(j && (j % 10) == 0)
				fprintf(fp,"\n");
			fprintf(fp,"%3d    ",i);
			j++;
		}
	}
	fprintf(fp,"\n\nThe following S5 inodes are on the freelist but have non-zero reference counts:\n");
	for(i = 0,j=0; i < ninode; i++) {
		if(listbuf[i].state == 'b') {
			if(j && (j % 10) == 0)
				fprintf(fp,"\n");
			fprintf(fp,"%3d    ",i);
			j++;
		}
	}

	fprintf(fp,"\n\nThe following S5 inodes are in unknown states:\n");
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
prinode(all,full,slot,phys,addr,heading)
int all,full,slot,phys;
long addr;
char *heading;
{
	char ch;
	int i;
	extern long lseek();

	if (addr == -1)
		return;
	readmem(addr, 1,-1,(char *)&ibuf,sizeof ibuf,"s5 inode");
	vnode = ibuf.i_vnode;

	if(!ibuf.i_vnode.v_count && !all) 
			return ;
	if( (long)vnode.v_op != S_s5vnodeops->n_value )
		return;	/* not s5 */

	if(full)
		fprintf(fp,"%s",heading);

	if(slot == -1)
		fprintf(fp,"  - ");
	else fprintf(fp,"%4d",slot);

	fprintf(fp," %4u,%-4u %4u  %3d %5d %7d %7d  %8ld",
		getemajor(ibuf.i_dev),
		geteminor(ibuf.i_dev),
		ibuf.i_number,
		vnode.v_count,
		ibuf.i_nlink,
		ibuf.i_uid,
		ibuf.i_gid,
		ibuf.i_size);
	switch(ibuf.i_vnode.v_type) {
		case VDIR: ch = ' d'; break;
		case VCHR: ch = ' c'; break;
		case VBLK: ch = ' b'; break;
		case VREG: ch = ' f'; break;
		case VLNK: ch = ' l'; break;
		case VFIFO: ch = ' p'; break;
		case VXNAM:
                        switch(ibuf.i_rdev) {
                                case XNAM_SEM: ch = ' s'; break;
                                case XNAM_SD: ch = ' m'; break;
                                default: ch = ' -'; break;
                        };
                        break;
		default:    ch = ' -'; break;
	}
	fprintf(fp,"  %c",ch);
	fprintf(fp,"  %s%s%s%03o",
		ibuf.i_mode & ISUID ? "u" : "-",
		ibuf.i_mode & ISGID ? "g" : "-",
		ibuf.i_mode & ISVTX ? "v" : "-",
		ibuf.i_mode & 0777);
	if (vnode.v_count != 0)
	    fprintf(fp,"%s%s%s%s%s%s%s%s\n",
		ibuf.i_flag & IUPD ? "   up" : "",
		ibuf.i_flag & IACC ? "   ac" : "",
		ibuf.i_flag & ICHG ? "   ch" : "",
		ibuf.i_flag & INOACC ? "   na" : "",
		ibuf.i_flag & ISYN ? "   sy" : "",
		ibuf.i_flag & ISYNC ? "   sc" : "",
		ibuf.i_flag & IMODTIME ? "   mt" : "",
		ibuf.i_flag & IMOD ? "   md" : "");
	else
	    fprintf(fp,"%s%s\n",
		ibuf.i_flag & ITFREE ? "   tfr" : "",
		ibuf.i_flag & IPFREE ? "   pfr" : "");

	if(!full)
		return;
	fprintf(fp,"\t    FORW\t    BACK\t    AFOR\t    ABCK\n");
	fprintf(fp,"\t%8x",ibuf.i_forw);
	fprintf(fp,"\t%8x",ibuf.i_back);
	fprintf(fp,"\t%8x",ibuf.i_freef);
	fprintf(fp,"\t%8x\n",ibuf.i_freeb);

	fprintf(fp,"\t   NEXTR   I_RWLOCK   SWAPCNT  \n");

	if (ibuf.i_rwlock.rw_mode & RWS_WRITE) {
		fprintf(fp,"\t%8x   1 WRITER   %8x   ", ibuf.i_nextr,
		 	ibuf.i_swapcnt);
	} else if (ibuf.i_rwlock.rw_mode & RWS_READ) {
		fprintf(fp,"\t%8x   %6dRD   %8x   ", ibuf.i_nextr,
			ibuf.i_rwlock.rw_read, ibuf.i_swapcnt);
	} else {
		fprintf(fp,"\t%8x   AVAILBLE   %8x   ", ibuf.i_nextr,
		 	ibuf.i_swapcnt);
	}

	if((ibuf.i_vnode.v_type == VDIR) || (ibuf.i_vnode.v_type == VREG)
		|| (ibuf.i_vnode.v_type == VLNK)) {
		for(i = 0; i < NADDR; i++) {
			if(!(i & 3))
				fprintf(fp,"\n\t");
			fprintf(fp,"[%2d]: %-10x",i,ibuf.i_addr[i]);
		}
		fprintf(fp,"\n");
	}
	else
		fprintf(fp,"\n");

	/* print vnode info */
	fprintf(fp,"\nVNODE :\n");
	fprintf(fp,"VCNT VFSMNTED   VFSP    STREAMP VTYPE   RDEV    VDATA    VFILOCKS VFLAG   LID \n");
	cprvnode(&ibuf.i_vnode);
	fprintf(fp,"\n");
}

/* get arguments for file function */
int
getfile()
{
	proc_t procbuf, *procaddr;
	int all = 0, slot;
	int full = 0;
	int phys = 0;
	int  i,c, nentries;
	long filep, addr;
	char *heading = "ADDRESS  RCNT    TYPE/ADDR       OFFSET   FLAGS\n";
	long prfile();
	fd_table_t fd_table;
	file_t	*fd_file;
	fd_entry_t	fd_entry;
	extern proc_t	*slot_to_proc();

	optind = 1;
	while((c = getopt(argcnt,args,"epfw:")) !=EOF) {
		switch(c) {
			case 'e' :	all = 1;
					break;
			case 'f' :	full = 1;
					break;
			case 'p' :	phys = 1;
					break;
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
				
	if(!full)
		fprintf(fp,"%s", heading);
	if(args[optind]) {
		all = 1;
		do {
			filep = strcon(args[optind],'h');
			if(filep == -1) 
				continue;
			else
				(void) prfile(all,full,phys,filep,heading);
			filep = -1;
		} while(args[++optind]);
	} else {
		/*
	 	* go through process table to find file descriptors.
	 	*/
		for (slot = 0; slot < vbuf.v_proc; slot++) {
			procaddr = slot_to_proc(slot);
			if (!procaddr)
				return;
			readmem((long)procaddr, 1, -1, (char *)&procbuf,
					sizeof (procbuf), "proc table");
			fd_table = procbuf.p_fdtab;
			nentries = fd_table.fdt_sizeused;
			addr = (long)fd_table.fdt_entrytab;
			for (i=0; i < nentries; i++) {
				readmem((long)addr, 1, -1,(char *)&fd_entry,
					 sizeof (fd_entry), "fd table entry");
				fd_file = fd_entry.fd_file;
				if (fd_file) {
					/* print file descriptors. */
					filep = prfile(all,full,phys,fd_file,heading);
				}
				addr += sizeof(fd_entry_t);
				nentries--;
			}
		}
	}
}


/* print file table */
long
prfile(all,full,phys,addr,heading)
int all,full,phys;
long addr;
char *heading;
{
	struct file fbuf;
	struct cred *credbufp;
	int fileslot;
	int ngrpbuf;
	short i;
	char fstyp[5];
	struct vnode vno;
	extern int ufsvno();

	readmem(addr,1,-1,(char *)&fbuf,sizeof fbuf,"file table");
	if(!fbuf.f_count && !all)
		return(0);
	if(full)
		fprintf(fp,"\n%s", heading);
	fprintf(fp,"%.8x", addr);
	fprintf(fp," %3d", fbuf.f_count);


	if(fbuf.f_count && fbuf.f_vnode != 0){
		char *tname;
		/* read in vnode */
		readmem(((long)fbuf.f_vnode),1,-1,(char *)&vno,sizeof vno,"vnode");

		tname = vnotofsname(&vno);
		if(tname == NULL)
			strcpy(fstyp, " ?  ");
		else {
			int i;
			strncpy(fstyp,tname,4);
			for(i = strlen(tname); i < 4; i++)
				fstyp[i] = ' ';
			fstyp[5] = '\0';
		}

	} else
		strcpy(fstyp, " ?  ");
	fprintf(fp,"    %s/%8x",fstyp,fbuf.f_vnode);
	fprintf(fp," %8x",fbuf.f_offset);
	fprintf(fp,"  %s%s%s%s%s%s%s%s\n",
		fbuf.f_flag & FREAD ? " read" : "",
		fbuf.f_flag & FWRITE ? " write" : "",  /* print the file flag */
		fbuf.f_flag & FAPPEND ? " appen" : "",
		fbuf.f_flag & FSYNC ? " sync" : "",
		fbuf.f_flag & FCREAT ? " creat" : "",
		fbuf.f_flag & FTRUNC ? " trunc" : "",
		fbuf.f_flag & FEXCL ? " excl" : "",
		fbuf.f_flag & FNDELAY ? " ndelay" : "");
	if(!full) {
		return 0;
#if 0
		if ((long)fbuf.f_flist.flink == File_list->n_value)
			fbuf.f_flist.flink = 0;
		return((long)fbuf.f_flist.flink);
#endif
	}

	/* user credentials */
	if(!Ngrps)
		if(!(Ngrps = symsrch("ngroups_max")))
			error("ngroups_max not found in symbol table\n");
	readmem((long)Ngrps->n_value, 1, -1, (char *)&ngrpbuf,
		sizeof ngrpbuf, "max groups");

	credbufp=(struct cred *)malloc(sizeof(struct cred) + sizeof(uid_t) * (ngrpbuf-1));
	readmem((long)fbuf.f_cred,1,-1,(char *)credbufp,sizeof (struct cred) + sizeof(uid_t) * (ngrpbuf-1),"user cred");
	fprintf(fp,"User Credential : \n");
	fprintf(fp,"rcnt:%3d, uid:%-10d, gid:%-10d, ruid:%-10d, rgid:%-10d, ngroup:%4d",
		credbufp->cr_ref,
		credbufp->cr_uid,
		credbufp->cr_gid,
		credbufp->cr_ruid,
		credbufp->cr_rgid,
		credbufp->cr_ngroups);
	for(i=0; i < (short)credbufp->cr_ngroups; i++){
		if(!(i % 4))
			fprintf(fp, "\n");
		fprintf(fp,"group[%d]:%4d ", i, credbufp->cr_groups[i]);
	}
	fprintf(fp, "\n");

	return 0;
}

gets5_ipos(addr, list, max)
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

int s5lck()
{
	inode_t	*ip, *ipx;
	struct hinode	*ih;
	int i, active=0;
	extern print_lock();

	if (!(S_s5_hash = symsrch("hinode")))
                 error("hinode not found in symbol table\n");

	readmem(S_s5_hash->n_value,1,-1,(char *)&hinode,
                 sizeof(hinode), "S5 hash list");

	for (i=0; i < NHINO; i++) {
                ih = &hinode[i];
                ipx = ih->ih_forw;
                for (ip = ipx; ;) {
			readmem((long)ip,1,-1, (char *)&ibuf,sizeof ibuf,
	    				"S5 inode ");

                        if (ibuf.i_forw == ipx)
                                break;
			if(ibuf.i_vnode.v_filocks != 0) {
				active =+ print_lock(ibuf.i_vnode.v_filocks, ip, "s5");
			}
                        ip = ibuf.i_forw;
                }
        }
	return (active);
}
