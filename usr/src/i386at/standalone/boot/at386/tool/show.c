/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)stand:i386at/standalone/boot/at386/tool/show.c	1.2"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/at_ansi.h>
#include <sys/kd.h>
#include <sys/inline.h>


/* 
 * This program is used to convert a 640X480 16 color pcx file to
 * a compressed format suitable for boot.
 *
 * The dcomp command can be used to display the compressed file at the 
 * video console.
 */

int compress(unsigned char *, int , int );

#define	byte	char

struct pcx_header {
        byte Manufacturer;
        byte Version;
        byte Encoding;
        byte Bits_per_pixel;
        short Xmin;
        short Ymin;
        short Xmax;
        short Ymax;
        short Hdpi;
        short Vdpi;
	byte ColorMap[16][3];
        byte Reserved;
        byte Nplanes;
        short Bytes_per_line_per_plane;
        short PaletteInfo;
        short HscreenSize;
        short VscreenSize;
        byte Filler[128-74];
} Header;

char video_buf[0x40000 + 4096];

main( int argc, char *argv[],char  *envp[])
{
	struct kd_disparam  v_parms;
	struct kd_memloc map;
	char *vidbuf;
	unsigned char *tbuf, x, *pcx_buf;
	int pcx_fd, video_fd;
	int count, xx, i, plane_no, offs, plane;

	if (argc != 2) {
		fprintf(stderr,"Usage: show pcx_file \n");
		exit(1);
	} 
        if ((pcx_fd = open(argv[1],O_RDONLY)) <0){
                fprintf(stderr,"show: Failed on open of %s\n",argv[1]);
                exit(1);
        }
        if ((video_fd = open("/dev/console",O_RDWR)) <0){
                fprintf(stderr,"show: Failed on open of video\n");
                exit(1);
        }

	read(pcx_fd,&Header,sizeof(Header));

#ifdef DEBUG
printf("PCX file header:\n");
printf("Header.Manufacturer) : %x\n",Header.Manufacturer);
printf("Header.Version) : %x\n",Header.Version);
printf("Header.Encoding) : %x\n",Header.Encoding);
printf("Header.Bits_per_pixel) : %x\n",Header.Bits_per_pixel);
printf("Header.Xmin) : %x\n",Header.Xmin);
printf("Header.Ymin) : %x\n",Header.Ymin);
printf("Header.Xmax) : %x\n",Header.Xmax);
printf("Header.Ymax) : %x\n",Header.Ymax);
printf("Header.Hdpi) : %x\n",Header.Hdpi);
printf("Header.Vdpi) : %x\n",Header.Vdpi);
printf("Header.ColorMap[16][3]) : %x\n",Header.ColorMap[16][3]);
printf("Header.Reserved) : %x\n",Header.Reserved);
printf("Header.Nplanes) : %x\n",Header.Nplanes);
printf("Header.Bytes_per_line_per_plane) : %x\n",Header.Bytes_per_line_per_plane);
printf("Header.PaletteInfo) : %x\n",Header.PaletteInfo);
printf("Header.HscreenSize) : %x\n",Header.HscreenSize);
printf("Header.VscreenSize) : %x\n",Header.VscreenSize);
#endif

	pcx_buf = malloc((count=lseek(pcx_fd,0L,2)-128));
	tbuf = malloc(Header.Bytes_per_line_per_plane);
	lseek(pcx_fd,128L,0);
	read(pcx_fd,pcx_buf,count);

	/* setup video access */
	vidbuf = video_buf;
	vidbuf = (char *)((int)(vidbuf + 4095) & ~4095);

	/* put vga in graphics mode 0x12 */
	if(ioctl(video_fd,SW_VGA640x480E,0) < 0){
		fprintf(stderr,"show: Failed on mode set of video\n");
		exit(1);
	}

	ioctl(video_fd,KDDISPTYPE,&v_parms);
	map.physaddr = v_parms.addr;
	map.vaddr = vidbuf;
	map.length=(64*1024);
	map.ioflg = 1;

	if(ioctl(video_fd, KDMAPDISP, &map) < 0){
		fprintf(stderr,"show: Failed on KDMAPDISP of video\n");
		exit(1);
	}

	offs = 0;
	outw(0x3ce,05); /* load the mode register */
	for(i = 0; i <= Header.Ymax; i++) {
                plane = 0x0100;
                offs+=80;
		for(plane_no = 0; plane_no < Header.Nplanes; plane_no++) {
			xx = 0;
			while(xx < Header.Bytes_per_line_per_plane) {
				x = *pcx_buf++;
				if ((x & 0xc0) != 0xc0) {
					tbuf[xx++]=x;
				}else{
					count = x & 0x3f;
					x = *pcx_buf++;
					memset(tbuf+xx, x, count);
					xx+=count;
				}
			}
                        /* load map mask register w/ "plane" */
                        outw(0x3c4,plane+2);
                        memcpy(vidbuf+offs,tbuf,Header.Bytes_per_line_per_plane);
                        plane <<= 1;

		}
	}
        if(ioctl(video_fd, KDUNMAPDISP) < 0){
                fprintf(stderr,"show: Failed on KDUNMAPDISP of video\n");
                exit(1);
        }
	
}
