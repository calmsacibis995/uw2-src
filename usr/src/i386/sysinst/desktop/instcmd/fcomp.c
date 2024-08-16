/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)proto:desktop/instcmd/fcomp.c	1.1"

#include <stdio.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include "kb_remap.h"
#include "fcomp.h"

/*
 * The fcomp (font compiler) command takes two arguments: the name of a font,
 * and the name of an output file.  It reads the font tables found in
 * $1.8x1[46] and the bitmaps found in grbm.8x1[46] (to use /bin/sh naming
 * conventions).  It overlays the bitmaps on top of the font tables starting at
 * character 128 (octal 200).  fcomp then writes the result to the output file,
 * which is in the format that kb_remap expects.
 * 
 * This program is similar in purpose to langsup:i386/ls/cmd/fcomp/fcomp.c,
 * but it produces output in a different format.
 */

int
main(int argc, char **argv)
{
	int ega_fd;		/*  EGA font input file descriptor  */
	int vga_fd;		/*  VGA font input file descriptor  */
	int ega_x_fd;
	int vga_x_fd;
	int ofd;

	char *ega_ptr;	/*  Pointer to EGA source memory mapped file  */
	char *vga_ptr;	/*  Pointer to VGA source memory mapped file  */
	char ega_src[STR_LEN];		/*  EGA file name  */
	char vga_src[STR_LEN];		/*  EGA file name  */

	CS_HDR cs_hdr;	/*  Code set structure  */

	/*  Check that we have at least two arguments, the source file and 
	 *  the output file
	 */
	if (argc < 3)
	{
		printf("Usage: fcomp <src_file> <output_file>\n");
		exit(1);
	}

	/*  Open the Output file for writing  */
	if ((ofd = open(argv[2], O_WRONLY | O_CREAT, 0644)) < 0)
	{
		printf("Unable to open the output file for writing\n");
		exit(1);
	}

	/*  The input name needs to be converted to the name of two files,
	 *  with 8x14 and 8x16 extensions.
	 */
	strcpy(ega_src, argv[1]);
	strcat(ega_src, EGA_EXT);
	strcpy(vga_src, argv[1]);
	strcat(vga_src, VGA_EXT);

	/*  Now try and open these files for reading  */
	if ((ega_fd = open(ega_src, O_RDONLY)) < 0)
	{
		printf("Unable to open the EGA source file, %s\n", ega_src);
		exit(1);
	}
	
	if ((vga_fd = open(vga_src, O_RDONLY)) < 0)
	{
		printf("Unable to open the VGA source file, %s\n", vga_src);
		exit(1);
	}

	/*  Now we memory map the files for improved access performance
	 */
	if ((ega_ptr = mmap((caddr_t)0, EGA_F_SIZE, PROT_READ,
		MAP_PRIVATE, ega_fd, 0)) == NULL)
	{
		printf("Unable to memory map EGA fie, corrupted?\n");
		exit(1);
	}

	if ((vga_ptr = mmap((caddr_t)0, VGA_F_SIZE, PROT_READ,
		MAP_PRIVATE, vga_fd, 0)) == NULL)
	{
		printf("Unable to memory map VGA fie, corrupted?\n");
		exit(1);
	}

	/*  Don't need file descriptors any more  */
	close(ega_fd);
	close(vga_fd);

	if ((ega_x_fd = open("grbm.8x14", O_RDONLY)) < 0)
	{
		printf("Unable to access bit-map overlay ega\n");
		exit(1);
	}

	if ((vga_x_fd = open("grbm.8x16", O_RDONLY)) < 0)
	{
		printf("Unable to access bit-map overlay vga\n");
		exit(1);
	}

	/*  Set up the header structure  */
	strcpy((char *)cs_hdr.ch_magic, CS_MAGIC);
	cs_hdr.ch_vers = CS_VERSION;
	
	/*  Position the pointers in the input files.
	 *  NOTE:  This is temporary, as we're using SCO files as input, and
	 *  they have all 256 chars defined.
	 */
	ega_ptr += EGA_OFFSET;
	vga_ptr += VGA_OFFSET;

	memcpy(cs_hdr.ch_cset.cs_ega, ega_ptr, EGA_SIZE);
	memcpy(cs_hdr.ch_cset.cs_vga, vga_ptr, VGA_SIZE);

	/*  Fill in the overlays  */
	if (read(ega_x_fd, cs_hdr.ch_cset.cs_ega, 154) < 154)
		printf("EGA overlay failed\n");

	if (read(vga_x_fd, cs_hdr.ch_cset.cs_vga, 176) < 176)
		printf("VGA overlay failed\n");

	close(ega_x_fd);
	close(vga_x_fd);

	/*  Write the structure to the file  */
	if (write(ofd, &cs_hdr, sizeof(CS_HDR)) < sizeof(CS_HDR))
	{
		printf("Unable to write structure to output file\n");
		exit(1);
	}

	close(ofd);
	return 0;
}
