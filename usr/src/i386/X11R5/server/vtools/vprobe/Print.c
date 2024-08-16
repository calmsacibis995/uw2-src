/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)vtools:vprobe/Print.c	1.7"

/*
 * Copyright 1993 by David Wexelblat <dwex@goblin.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of David Wexelblat not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  David Wexelblat makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * DAVID WEXELBLAT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL DAVID WEXELBLAT BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* $XFree86: mit/server/ddx/x386/SuperProbe/Print.c,v 2.9 1993/10/07 13:54:49 dawes Exp $ */

#include "Probe.h"

#ifdef __STDC__
# define CONST const
#else
# define CONST
#endif

CONST char *SVGA_Names[NUM_VENDORS+1][CHPS_PER_VENDOR] = 
{
/* dummy */	{ "" },	
/* AcuMos */	{ "AcuMos AVGA2" },
/* Ahead */	{ "Ahead (chipset unknown)",
		  "Ahead V5000 Version A", "Ahead V5000 Version B" },
/* ATI */	{ /*  "ATI (chipset unknown)", **** chipset unknown is confusing */
          "ATI ",
		  "ATI 18800", "ATI 18800-1",
		  "ATI 28800-2", "ATI 28800-4", "ATI 28800-5", 
		  "ATI 28800-A", "ATI 28800-C" },
/* AL */	{ "Avance Logic 2101" },
/* CT */	{ "Chips & Tech (chipset unknown)",
		  "Chips & Tech 82c450", "Chips & Tech 82c451", 
		  "Chips & Tech 82c452", "Chips & Tech 82c453",
		  "Chips & Tech 82c455", "Chips & Tech 82c456",
		  "Chips & Tech 82c457", "Chips & Tech F65510",
		  "Chips & Tech F65520", "Chips & Tech F65530" },
/* Cirrus */	{ "Cirrus (chipset unknown)",
		  "Cirrus CL-GD 510/520", "Cirrus CL-GD 610/620",
		  "Cirrus Video7 OEM",
		  "Cirrus CL-GD5402", "Cirrus CL-GD5402 Rev 1",
		  "Cirrus CL-GD5420", "Cirrus CL-GD5420 Rev 1",
		  "Cirrus CL-GD5422", "Cirrus CL-GD5424", 
		  "Cirrus CL-GD5426", "Cirrus CL-GD5428",
		  "Cirrus CL-GD6205", "Cirrus CL-GD6215",
		  "Cirrus CL-GD6225", "Cirrus CL-GD6235",
		  "Cirrus CL-GD6410", 
		  "Cirrus CL-GD6420A", "Cirrus CL-GD6420B",
		  "Cirrus CL-GD5434" },
/* Compaq */	{ "Compaq (chipset unknown)",
		  "Compaq Advanced VGA", 
		  "Compaq QVision 1024", "Compaq QVision 1280" },
/* Genoa */	{ "Genoa GVGA 6100",
		  "Genoa GVGA 6200/6300", "Genoa GVGA 6400/6600" },
/* MX */	{ "MX 68010" },
/* NCR */	{ "NCR (chipset unknown)",
		  "NCR 77C21", "NCR 77C22", "NCR 77C22E", "NCR 77C22E+" },
/* Oak */	{ "Oak (chipset unknown)",
		  "Oak OTI037C", 
		  "Oak OTI-057", "Oak OTI-067", "Oak OTI-077" },
/* Primus */	{ "Primus P2000" },
/* Realtek */	{ "Realtek RT 3106" },
/* S3 */	{ "S3 (chipset unknown)",
		  "S3 86C911", "S3 86C924",
		  "S3 86C801, A or B-step", 
		  "S3 86C801, C-step", "S3 86C801, D-step",
		  "S3 86C805, A or B-step", 
		  "S3 86C805, C-step", "S3 86C805, D-step",
		  "S3 86C928, A,B,C, or D-step", "S3 86C928, E-step",
		  "S3 86C928PCI" , "S3 86C928, G-step",
		  "S3 Vision864", "S3 Vision964"},
/* Trident */	{ "Trident (chipset unknown)",
		  "Trident 8800BR", "Trident 8800CS",
		  "Trident 8900B", "Trident 8900C", "Trident 8900CL",
		  "Trident 9000", "Trident LCD9100", "Trident LX9200" },
/* Tseng */	{ "Tseng ET3000", "Tseng ET4000", "Tseng ET4000/W32",
                  "Tseng ET4000/W32i", 
				  "Tseng ET4000/W32i Revision B",
                  "Tseng ET4000/W32p Revision A",
                  "Tseng ET4000/W32p Revision B",
                  "Tseng ET4000/W32p Revision C" },
/* Video7 */	{ "Video7 (chipset unknown)",
		  "Video7 VEGA", "Video7 FastWrite/VRAM",
		  "Video7 VRAM II", "Video7 1024i" },
/* WD */	{ "WD/Paradise PVGA1", "WD/Paradise 90C00", 
		  "WD/Paradise 90C10", "WD/Paradise 90C11",
		  "WD/Paradise 90C20", "WD/Paradise 90C20A",
		  "WD/Paradise 90C22", "WD/Paradise 90C24", 
		  "WD/Paradise 90C26",
		  "WD/Paradise 90C30", "WD/Paradise 90C31" },
/* Yamaha */	{ "Yamaha 6388 VPDC" },
};

static CONST char *Herc_Names[] = 
{
	"",		/* indices start at 1 */
	"Standard",
	"Plus",
	"InColor",
};

CONST char *RamDac_Names[] =
{
	"Generic 8-bit pseudo-color",
	"Avance Logc ALG1101",
	"Diamond SS2410",
	"Sierra 15-bit HiColor",
	"Sierra 15/16-bit HiColor",
	"AcuMos ADAC1 15/16/24-bit DAC",
	"ATI 68830/75 15/16/24-bit DAC",
	"Cirrus Logic Built-in 8-bit pseudo-color DAC",
	"Cirrus Logic Built-in 15/16/24-bit DAC",
	"AT&T 20C490 15/16/24-bit DAC",
	"AT&T 20C491 15/16/24-bit DAC with gamma correction",
	"AT&T 20C492 15/16/18-bit DAC with gamma correction",
	"AT&T 20C493 15/16/18-bit DAC",
	"AT&T 20C497 24-bit wide, 8-bit pseudo-color DAC",
	"BrookTree Bt485 24-bit TrueColor DAC w/cursor,pixel-interleave",
	"BrookTree Bt484 24-bit TrueColor DAC"
};

CONST char *CoProc_Names[NUM_CP_TYPES][CHPS_PER_CPTYPE] = 
{
/* 8514 */	{ "8514/A (or true clone)",
		  "ATI Mach-8", "ATI Mach-32",
		  "Chips & Technologies 82C480" , "ATI Mach-64" },
};

#ifdef __STDC__
void Print_SVGA_Name(int Chipset)
#else
void Print_SVGA_Name(Chipset)
int Chipset;
#endif
{
	int vendor = SVGA_VENDOR(Chipset);
	int chip = SVGA_CHIP(Chipset);
	printf("\tCHIPSET:  %s\n", SVGA_Names[vendor][chip]);
	if ((!chip) && (Chip_data != 0xFF))
	{
		printf("\t\tSignature data: %02x \n", 
		       Chip_data);
	}
}

#ifdef __STDC__
void Print_Herc_Name(int Chipset)
#else
void Print_Herc_Name(Chipset)
int Chipset;
#endif
{
	int chip = HERC_CHIP(Chipset);
	printf("\tCHIPSET:  %s\n", Herc_Names[chip]);
}

#ifdef __STDC__
void Print_RamDac_Name(int RamDac)
#else
void Print_RamDac_Name(RamDac)
int RamDac;
#endif
{
	printf("\t RAMDAC:  %s\n", RamDac_Names[DAC_CHIP(RamDac)]);
	if (RamDac & DAC_8BIT)
	{
		printf("\t\t (with 8-bit wide lookup tables)\n");
	}
	else
	{
		printf("\t\t  (with 6-bit wide lookup tables ");
		printf("(or in 6-bit mode))\n");
	}
	if (RamDac & DAC_6_8_PROGRAM)
	{
		printf("\t\t  (programmable for 6/8-bit wide lookup tables)\n");
	}
}

#ifdef __STDC__ 
void Print_CoProc_Name(int CoProc)
#else
void Print_CoProc_Name(CoProc)
int CoProc;
#endif
{
	int class = COPROC_CLASS(CoProc);
	int chip = COPROC_CHIP(CoProc);
	printf("\t\tCHIPSET:  %s\n", CoProc_Names[class][chip]);
}

#ifdef __STDC__
void Print_Memory(int memory)
#else
void Print_Memory(memory)
int memory;
#endif
{
	if (memory>0) {
		printf("\t MEMORY:  %dK\n", memory);
	}
	else
	{
		printf("\t MEMORY:  Cannot determine Size of Video Memory\n");
	}
}
