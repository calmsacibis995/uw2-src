/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)stand:i386at/standalone/boot/at386/sip/images.c	1.2.1.7"

#include <sys/types.h>
#include <sys/bootinfo.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/cram.h>
#include <sys/inline.h>
#include <boothdr/boot.h>
#include <boothdr/bootlink.h>
#include <boothdr/initprog.h>
#include <boothdr/libfm.h>
#include <boothdr/error.h>

typedef char	byte;

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
}; 
struct pcx_header *Header;

#define PCX_DATA_OFFSET	sizeof(struct pcx_header)

#define	TEXT_MODE	0x0003
#define NUM_PCX_BUFS	3

/* to determine if we need to read the image file */
int	got_image[NUM_PCX_BUFS] = {0,0,0}; 
unsigned char	*img_buf[NUM_PCX_BUFS];
static int	bt_logo_up = 0;


unsigned char  *pcx_logo_buf = 0;
unsigned char  *pcx_wrk_buf = 0;
unsigned char  *pcx_clrwrk_buf = 0;
unsigned char	*tbuf = 0;
unsigned char	video_mode;
/*
* these routines provide the support/stubs for displaying a logo to the 
* console while the system is being booted.  A VGA support is assumed.
*/

int 
bt_logo( fc )
	pcx_img_t	fc;
{
	int	status;
	void display_image(), reset_video(), set_colorregs(), set_palette();
#ifdef IMG_DEBUG 
printf("bt_logo: 0x%x\n",fc);
goany();
#endif 
	switch( fc )
	{
	case DISPLAY_LOGO:
		/* Display the boot logo, if it exists, and if we have a
		   VGA display adapter installed */
		if( got_image [ DISPLAY_LOGO ] != 0){
			if( bt_logo_up == 0){
				set_video();
				set_palette();
				display_image( DISPLAY_LOGO );
				bt_logo_up = 1;
			}
			return( E_OK );
		}else
			return( ~E_OK );

		break;
	case DISPLAY_WORKING:
		if(( bt_logo_up == 1 ) && ( got_image[DISPLAY_WORKING] == 1 ))
			display_image( DISPLAY_WORKING );
		return( E_OK );
		break;
	case CLEAR_WORKING:
		if(( bt_logo_up == 1 ) && ( got_image[CLEAR_WORKING] == 1 ))
			display_image( CLEAR_WORKING );
		return( E_OK );
		break;
	case REMOVE_LOGO:
		if( bt_logo_up == 1 ){
			bt_logo_up = 0;
			reset_video();
		}
		return( E_OK );
		break;
	};
}

int 
read_image()
{
	int	 image_len, actual, status = E_OK;
	pcx_img_t	num_imgs, img_type;

	BL_file_read(physaddr(&num_imgs),NULL,4L,&actual,&status);
	if (status == E_OK){
		for(img_type=DISPLAY_LOGO; img_type < num_imgs; img_type++ ){
			BL_file_read(physaddr(&image_len),NULL,4L,&actual,&status);
			if (status == E_OK){
				BL_file_read(physaddr(&Header[img_type]),NULL,sizeof(struct pcx_header),&actual,&status);
				if (status == E_OK) {
					BL_file_read(physaddr(img_buf[img_type]),NULL,image_len-sizeof(struct pcx_header),&actual,&status);
					if (status == E_OK) 
						got_image[img_type] = 1;
					else
						break;
				}else
					break;
			}else
				break;
		}
	}
	return(status);
}


/*
 * Display a VGA image, 640 x 480 16 colors
 */
void 
display_image( img_type)
pcx_img_t	img_type;
{
	struct	pcx_header	*Hdrp = &Header[img_type];
	unsigned char  *vidbuf     = (unsigned char *) 0x000A0000;
	unsigned char x;
	int count, offs, xx, i, plane, plane_no;
	unsigned char	*pcx_bufp = img_buf[img_type];

	offs = 0;
	for(i = 0; i <= Hdrp->Ymax; i++) {
		plane = 0x0100;
		for(plane_no = 0; plane_no < Hdrp->Nplanes; plane_no++) {
			xx = 0;
			while(xx < Hdrp->Bytes_per_line_per_plane) {
				x = *(pcx_bufp++);
				if ((x & 0xc0) != 0xc0) {
					tbuf[xx++]=x;
				}else{
					count = x & 0x3f;
					x = *(pcx_bufp++);
					memset(tbuf+xx, x, count);
					xx+=count;
				}
			}
			/* load map mask register w/ "plane" */
			outw(0x3c4,plane+2);
			memcpy(vidbuf+offs,tbuf,Hdrp->Bytes_per_line_per_plane);
			plane <<= 1;

		}
		offs+=80;
	}
}

/*
 * Put VGA back to text mode
 */
void
reset_video()
{
	struct int_pb	ic;

	/* reset to original VGA mode */
	outw(0x3c4,0x0f02);	

	memset(&ic, 0, sizeof(struct int_pb));
	ic.intval = 0x10;
	ic.ax = (unsigned short) video_mode;
	doint(&ic);
}

int
set_video()
{
	struct int_pb	ic;
	int	yy, xx;

	/* if VGA, save original mode */
	memset(&ic, 0, sizeof(struct int_pb));
	ic.intval = 0x10;
	ic.ax = 0x0f00;
	doint(&ic); /* get video information */
	video_mode = (unsigned char)ic.ax&0x7f;

	memset(&ic, 0, sizeof(struct int_pb));
	ic.ax = 0x12;	/* VGA mode 12, 640x480 16 colors */
	ic.intval = 0x10;
	doint(&ic);
	memset(&ic, 0, sizeof(struct int_pb));
	ic.ax = 0x0f00;
	ic.intval = 0x10;
	doint(&ic); /* read current video state */
	if ((ic.ax & 0x7f) == 0x12){
		outw(0x3ce,05); /* load the mode register */
		return (1);
	}else{
		memset(&ic, 0, sizeof(struct int_pb));
		ic.intval = 0x10;
		ic.ax = (unsigned short) video_mode;
		doint(&ic);
		return(0);
	}


}

void
set_palette()
{
	struct int_pb	ic;
	int xx,yy;

	for( yy=0; yy<16; yy++){

		if (yy > 7 )
			xx = 0x30 | yy;
		else
			xx = yy;
		ic.ax = 0x1000;
		ic.bx = (xx << 8) | yy;
		ic.intval = 0x10;
		ic.ds = 0;
		doint(&ic);
	}
	/* adjust for color red */
		ic.ax = 0x1000;
		ic.bx = 0x0401;
		ic.intval = 0x10;
		ic.ds = 0;
		doint(&ic);
		ic.ax = 0x1000;
		ic.bx = 0x0409;
		ic.intval = 0x10;
		ic.ds = 0;
		doint(&ic);
}

int
bt_img_init(logo)
char	*logo;
{
	int	rtn=FAILURE,status;

	BL_file_open(logo,&status);
	if (status == E_OK){
		pcx_wrk_buf = (unsigned char *)malloc(0x2000);
		pcx_clrwrk_buf = (unsigned char *)malloc(0x2000);
		tbuf = (unsigned char *)malloc(0x1000);
		Header = (struct pcx_header *)malloc(sizeof(struct pcx_header)*3);
		pcx_logo_buf = (unsigned char *)malloc(0x9000);

		img_buf [ DISPLAY_LOGO ] = pcx_logo_buf;
		img_buf [ DISPLAY_WORKING ] = pcx_wrk_buf;
		img_buf [ CLEAR_WORKING ] = pcx_clrwrk_buf;
		if( read_image() == E_OK){
			if(set_video()){
				set_palette();
				display_image( DISPLAY_LOGO );
				display_image( DISPLAY_WORKING );
				bt_logo_up = 1;
				rtn=SUCCESS;
			}
		}

		BL_file_close(&status);
	}
#ifdef IMG_DEBUG
printf("bt_img_init: wrk: 0x%x clrwrk: 0x%x tbuf: 0x%x Header: 0x%x pcx_buf; 0x%x\n", wrk, clrwrk, tbuf, Header, pcx_buf);
printf("return status: %d\n",rtn);
goany();
#endif
	if ( rtn == SUCCESS )
		bfp->logo = bt_logo;
	return(rtn);

	
}
