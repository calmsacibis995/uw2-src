/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)pdi.cmds:diskinit.c	1.3.1.8"


/* The diskinit.c file contains AT specific routines used by disksetup to */
/* initialize the disk. 						  */

#include <sys/types.h>
#include <sys/fdisk.h>
#include <malloc.h>
#include <stdio.h>
#include <sys/alttbl.h>
#include <sys/vtoc.h>
#include <sys/sysi86.h>
#include <pfmt.h>
#include <errno.h>

#define RESERVED        34	/* reserved sectors at start of drive */
#define ROOTSLICE	1
#define SWAPSLICE	2
#define DOSSLICE	5
#define BOOTSLICE	7
#define ALTSSLICE	8
#define TALTSLICE	9
#define ALTDOSSL_1	14	/* secondary dos slice */
#define ALTDOSSL_2	15	/* secondary dos slice */
#define TRUE  1
#define FALSE  0

extern  int     diskfd;        	/* file descriptor for raw wini disk */
extern  struct  disk_parms dp;     /* Disk parameters returned from driver */
extern  struct	vtoc		vtoc;	/* struct containing slice info */
extern  struct  pdinfo		pdinfo; /* struct containing disk param info */
extern  struct  alt_info	alttbls; /* struct containing alternates info */
extern  char    *devname;	/* pointer to device name */
extern  int     cylsecs;        /* number of sectors per cylinder */
extern  long	cylbytes;        /* number of bytes per cylinder */
extern  daddr_t	unix_base;	/* first sector of UNIX System partition */
extern  daddr_t	unix_size;	/* # sectors in UNIX System partition */
extern  int	pstart;		/* next slice start location */
extern  struct absio	absio;
struct mboot	mboot;
struct ipart	*fdp, *unix_part;

/* get_unix_partition will read in partition table from sector 0 of the disk  */
/* it will verify its sane and then search for an active unix partition. If   */
/* found it will save the start and size of the partition. */
get_unix_partition()
{
	int i;

	cylsecs = (int)dp.dp_heads * dp.dp_sectors;
	cylbytes = (long)cylsecs * dp.dp_secsiz;
	absio.abs_sec = 0;
	absio.abs_buf = (char *)&mboot;
	if (ioctl(diskfd, V_RDABS, &absio) < 0) {
		(void) pfmt(stderr, MM_ERROR,
		  ":24:Disksetup unable to read partition table from %s: %s\n",
			devname, strerror(errno));
		exit(70);
	}

	/* find an active UNIX System partition */
	unix_part = NULL;
	fdp = (struct ipart *)mboot.parts;
	for (i = FD_NUMPART; i-- > 0; ++fdp) {
		if ((fdp->systid == UNIXOS) && (fdp->bootid == ACTIVE))
				unix_part = fdp;
	}
	if (unix_part == NULL) {
		(void) pfmt(stderr, MM_ERROR,
		  ":25:No active UNIX System partition in partition table!\n");
		exit(71);
	}
	unix_base = unix_part->relsect;
	unix_size = unix_part->numsect;
	/* Initialize vtoc */
	memset((char *)&vtoc, 0, sizeof(vtoc));

}
/* this routines assigns the DOS partitions into the UNIX vtoc for use
 * by VPIX. The first DOS partition is assigned to slot 5 with secondary
 * partitions assigned to slots 14 (ALTDOSSL_1) and 15 (ALTDOSSl_2) if
 * they aren't already assigned to a UNIX slice.
 */

assign_dos(){

	/*
	 * This method of accessing DOS partitions should no longer be used
	 * therefore, let's force this routine to not create the vtoc entries,
	 * but leave the code here in case DOS Merge needs this change backed
	 * out
	 */
	return 0;

#if	0

int i;
int first = TRUE;

/* see if a DOS partition was allocated */
	fdp = (struct ipart *)mboot.parts;
	for (i = 0; i < FD_NUMPART; i++, ++fdp) {
		if (fdp->systid == DOSOS12 ||
		    fdp->systid == DOSOS16 ||
		    fdp->systid == DOSHUGE ||
		    fdp->systid == EXTDOS)
			if (first == TRUE) {
				vtoc.v_part[DOSSLICE].p_tag = V_OTHER;
				vtoc.v_part[DOSSLICE].p_flag = V_UNMNT | V_VALID;
				vtoc.v_part[DOSSLICE].p_start = fdp->relsect;
				vtoc.v_part[DOSSLICE].p_size = fdp->numsect;
				first = FALSE;

			/* vtoc slots 14 and 15 are secondary DOS
			 * partions although UNIX has precedence
			 * over these slots
			 */
			} else if (!vtoc.v_part[ALTDOSSL_1].p_tag) {
					vtoc.v_part[ALTDOSSL_1].p_tag = V_OTHER;
					vtoc.v_part[ALTDOSSL_1].p_flag = V_UNMNT | V_VALID;
					vtoc.v_part[ALTDOSSL_1].p_start = fdp->relsect;
					vtoc.v_part[ALTDOSSL_1].p_size = fdp->numsect;
				} else if (!vtoc.v_part[ALTDOSSL_2].p_tag) {
					vtoc.v_part[ALTDOSSL_2].p_tag = V_OTHER;
					vtoc.v_part[ALTDOSSL_2].p_flag = V_UNMNT | V_VALID;
					vtoc.v_part[ALTDOSSL_2].p_start = fdp->relsect;
					vtoc.v_part[ALTDOSSL_2].p_size = fdp->numsect;

				} else {
					(void) putc('\n', stderr);
					(void) pfmt(stderr, MM_ERROR,
						":26:No slots available in vtoc for DOS partition \"%d\" on drive %s\n", i+1, devname);
					return(-1);
				}

	}
	return(0);
#endif
}

init_structs()
{
	/* Initialize alt_info struct alttbls */
	memset((char *)&alttbls, 0, sizeof(alttbls)); 
	alttbls.alt_sanity = ALT_SANITY; 
	alttbls.alt_version = ALT_VERSION; 

	/* Initialize pdinfo structure */
	pdinfo.driveid = 0;		/* reasonable default value	*/
	pdinfo.sanity = VALID_PD;
	pdinfo.version = V_VERSION;
	strncpy(pdinfo.serial, "            ", sizeof(pdinfo.serial));
	pdinfo.cyls = dp.dp_cyls;
	pdinfo.tracks = dp.dp_heads;
	pdinfo.sectors = dp.dp_sectors;
	pdinfo.bytes = dp.dp_secsiz;
	pdinfo.logicalst = dp.dp_pstartsec;
	pdinfo.vtoc_ptr = dp.dp_secsiz * VTOC_SEC + sizeof(pdinfo);
	pdinfo.vtoc_len =  sizeof(vtoc);
	pdinfo.alt_ptr = dp.dp_secsiz * (VTOC_SEC + 1);
	pdinfo.alt_len = sizeof(struct alt_info);

	/* Initialize vtoc */
	vtoc.v_sanity = VTOC_SANE;
	vtoc.v_version = V_VERSION;
	strncpy(vtoc.v_volume, devname, sizeof(vtoc.v_volume));
	vtoc.v_nparts = 1;
	vtoc.v_part[0].p_tag = V_BACKUP;
	vtoc.v_part[0].p_flag = V_UNMNT | V_VALID;
	vtoc.v_part[0].p_start = unix_base;
	vtoc.v_part[0].p_size = unix_size;
	vtoc.v_part[BOOTSLICE].p_tag = V_BOOT;
	vtoc.v_part[BOOTSLICE].p_flag = V_UNMNT | V_VALID;
	vtoc.v_part[BOOTSLICE].p_start = unix_base;
	vtoc.v_part[BOOTSLICE].p_size = RESERVED;
	pstart = unix_base + RESERVED;
}

/* fs_gap returns the filesystem gap value needed for a mkfs on this device. */
int
fsgap()
{

        switch (dp.dp_sectors) {
	case 17:
                return(2);
	case 34:
                if (sysi86(SI86RDID) == C2)
                        return(8);
		else
                    	return(6);
	case 51:
		return(8);
	case 64:
		return(8);
	default:
		return(8);
	}
}

