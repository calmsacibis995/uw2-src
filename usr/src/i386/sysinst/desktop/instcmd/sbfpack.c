/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)proto:desktop/instcmd/sbfpack.c	1.8"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/vnode.h>
#include <sys/vfs.h>
#include <sys/fs/bfs.h>
#include <sys/sysmacros.h>
#include <sys/bootinfo.h>
#include <sys/stat.h>
#include <sys/vtoc.h>

typedef struct sbfs_d {
	char	fname[B_PATHSIZ];
	daddr_t	s_addr;
	daddr_t	e_addr;
}	sbfsdir_t;

sbfsdir_t	sbfs_dir[10] = {
	"",0,0,
	"",0,0,
	"",0,0,
	"",0,0,
	"",0,0,
	"",0,0,
	"",0,0,
	"",0,0,
	"",0,0,
	"",0,0,
};
char	cpy_buf[36*512];

main(int argc, char *argv[])
{
	int cp_fd[10], out_fd, cnt;
	long	count;
	int	blkcnt = 0;
	struct stat	statbuf;
	int	i,j,seek,offset = 512;
	int avail, outdev_size, track_size;
	struct disk_parms parms;

	if ((argc < 2) || (argc > 11)) {
		fprintf(stderr,"Usage: sbfpack source_file(s) output_device\n");
		exit(1);
	} 
	
	if ((out_fd = open(argv[argc-1],O_RDWR | O_CREAT)) == -1) {
		fprintf(stderr,"sbfpack: ERROR: Failed on open of %s\n",argv[argc-1]);
		exit(1);
	}
	if (fstat(out_fd, &statbuf) == -1) {
		fprintf(stderr,"sbfpack: ERROR: Failed on stat of %s\n", argv[argc-1]);
		exit(1);
	}
	if (S_ISCHR(statbuf.st_mode)) {
		if (ioctl(out_fd, V_GETPARMS, &parms) == -1) {
			fprintf(stderr,"sbfpack: ERROR: Cannot get output device parameters.\n");
			exit(1);
		}
		outdev_size = parms.dp_pnumsec;
	} else if (S_ISREG(statbuf.st_mode)) {
		outdev_size = 2880; /* use the size of a 1.44MB floppy by default */
	} else {
		fprintf(stderr,"sbfpack: ERROR: Must specify a character device or a regular file.\n");
		exit(1);
	}
	switch (outdev_size) {
	case 2400:
		track_size = 30 * 512;
		break;
	case 2880:
		track_size = 36 * 512;
		break;
	default:
		fprintf(stderr,"sbfpack: ERROR: output device is neither 1.2MB nor 1.44MB.\n");
		exit(1);
		/* NOTREACHED */
	}
	outdev_size--; /* reserve last block for serial number */
	cp_fd[0] = open(argv[1],O_RDONLY);
	for(cnt=1; cnt <= argc-3; cnt++){
		if ((cp_fd[cnt] = open(argv[cnt+1],O_RDONLY)) == -1) {
			fprintf(stderr,"sbfpack: ERROR: Failed on open of %s\n",argv[cnt+1]);
			exit(1);
		}
		if (stat(argv[cnt+1],&statbuf) != 0){
			fprintf(stderr,"sbfpack: ERROR: Failed on stat of %s\n",argv[cnt]);
			exit(1);
		}
		strcpy((char *)&sbfs_dir[cnt-1].fname,argv[cnt+1]);
		sbfs_dir[cnt-1].s_addr = offset;
		sbfs_dir[cnt-1].e_addr = offset + statbuf.st_size - 1;
		i = statbuf.st_size/512;
		i = ( (i * 512) == statbuf.st_size) ? 0 : ((i+1)*512 - statbuf.st_size );
		offset += statbuf.st_size + i;
		
	}
	lseek(out_fd,0,SEEK_SET);
	count = read(cp_fd[0],cpy_buf, track_size);
	if ( (j=count % 512) != 0 )
		count += 512-j;
	write(out_fd,cpy_buf,count);
	blkcnt+=count/512;
	memcpy(cpy_buf, sbfs_dir, sizeof(sbfs_dir));
	write(out_fd,cpy_buf,512);
	blkcnt++;
/*	DEBUG */
	write(out_fd,cpy_buf,512);
	blkcnt++;
	avail = outdev_size - (blkcnt + (offset/512)-1);
	if (avail < 0) {
		fprintf(stderr,
			"sbfpack: ERROR: output device is %d blocks smaller than data.\n",
			-avail);
		exit(1);
	}
	printf("Available blocks: %d\n", avail);
	printf("Blocks done: %d",blkcnt);
	fflush(stdout);
	count=0;
	for(cnt=2; cnt <= argc-2; cnt++){
		while(j = read(cp_fd[cnt-1],cpy_buf+count, track_size - count)){
			count += j;
			if ((j=count % 512) != 0 )
				count += 512-j;
			if (count == track_size){
				write(out_fd,cpy_buf,count);
				blkcnt+=count/512;
				printf("\rBlocks done: %d   ",blkcnt);
				fflush(stdout);
				count=0;
			}
		}
	}
	if ( count != 0 ){
		write(out_fd,cpy_buf,count);
		blkcnt+=count/512;
		printf("\rBlocks done: %d   ",blkcnt);
		fflush(stdout);
	}
	printf("\n");
	printf("Available blocks: %d\n", outdev_size - blkcnt);
	for(cnt=1; cnt <= argc-2; cnt++)
		close(cp_fd[cnt-1]);
	close(out_fd);
}
