/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)vtools:vprobe/Probe.h	1.9"


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

/* $XFree86: mit/server/ddx/x386/SuperProbe/Probe.h,v 2.9 1993/10/07 13:54:50 dawes Exp $ */

#define VERSION 	1
#define PATCHLEV 	0
#define PRINT_VERSION	printf("\n%s Version %d.%d\n",MyName,VERSION,PATCHLEV)

/*
 * Includes
 */
#if defined(__STDC__) && defined(__GNUC__)
#define inline __inline__
#endif
#ifdef _POSIX_SOURCE
#include <sys/types.h>
#endif
#include <stdio.h>
#ifndef	MACH386
#include <unistd.h>
#include <stdlib.h>
#endif	/* MACH386 */
#include <ctype.h>
#if defined(SYSV) || defined(SVR4) || defined(linux)
# include <string.h>
# include <memory.h>
#else
#ifdef _MINIX
# include <string.h>
#else
# include <strings.h>
# define strchr(a,b) 	index((a),(b))
# define strrchr(a,b) 	rindex((a),(b))
#endif /* _MINIX */
#endif

/*
 * Types
 */
typedef unsigned char Byte;
typedef unsigned short Word;
typedef unsigned long Long;
typedef int Bool;
#define FALSE 0
#define TRUE 1

typedef struct {
	Word	lo, hi;
} Range;

typedef Bool (*ProbeFunc)(int *);
typedef int (*MemoryFunc)();

typedef struct {
	char		*name;		/* Chipset vendor/class name 	*/
	ProbeFunc	f;		/* Probe function		*/
	MemoryFunc	m;		/* function to detect Memory	*/
	Word		*ports;		/* List of ports used by probe	*/
	int		num_ports;	/* Number of ports in list	*/
	Bool		bit16;		/* Does probe use 16-bit ports?	*/
	Bool		uses_bios;	/* Does probe read the BIOS?	*/
	Bool		check_coproc;	/* Check for attached coprocessor */
} Chip_Descriptor;

/*
 * Prototypes
 */

/* OS_xxxx.c */
#ifdef __STDC__
int OpenVideo(void);
void CloseVideo(void);
int ReadBIOS(const unsigned, Byte *, const int);
int EnableIOPorts(const int, const Word *);
int DisableIOPorts(const int, const Word *);
void ShortSleep(const int Delay);
#else /* __STDC__ */
int OpenVideo();
void CloseVideo();
int ReadBIOS();
int EnableIOPorts();
int DisableIOPorts();
void ShortSleep();
#endif /* __STDC__ */

/* Utils.c */
#ifdef __STDC__
Byte inp(Word);
Word inpw(Word);
void outp(Word, Byte);
void outpw(Word, Word);
Byte rdinx(Word, Byte);
void wrinx(Word, Byte, Byte);
void modinx(Word, Byte, Byte, Byte);
Bool tstrg(Word, Byte);
Bool testinx2(Word, Byte, Byte);
Bool testinx(Word, Byte);
void dactopel(void);
Byte dactocomm(void);
Bool Excluded(Range *, Chip_Descriptor *, Bool);
int StrCaseCmp(char *, char *);
unsigned int StrToUL(const char *);
#else /* __STDC__ */
Byte inp();
Word inpw();
void outp();
void outpw();
Byte rdinx();
void wrinx();
void modinx();
Bool tstrg();
Bool testinx2();
Bool testinx();
void dactopel();
Byte dactocomm();
Bool Excluded();
int StrCaseCmp();
unsigned int StrToUL();
#endif /* __STDC__ */

/*
 * Ident functions
 */
#ifdef __STDC__
void Probe_RamDac(int, int *);
Bool Probe_MDA(int *);
Bool Probe_Herc(int *);
Bool Probe_CGA(int *);
Bool Probe_VGA(int *);
/* SVGA */
Bool Probe_Tseng(int *);
Bool Probe_WD(int *);
Bool Probe_CT(int *);
Bool Probe_Video7(int *);
Bool Probe_Genoa(int *);
Bool Probe_Trident(int *);
Bool Probe_Oak(int *);
Bool Probe_Cirrus(int *);
Bool Probe_Ahead(int *);
Bool Probe_ATI(int *);
Bool Probe_S3(int *);
Bool Probe_AL(int *);
Bool Probe_Yamaha(int *);
Bool Probe_NCR(int *);
Bool Probe_AcuMos(int *);
Bool Probe_MX(int *);
Bool Probe_Primus(int *);
Bool Probe_RealTek(int *);
Bool Probe_Compaq(int *);
/* CoProc */
Bool Probe_8514(int *);
Bool Probe_ATIMach(int *);
#else /* __STDC__ */
void Probe_RamDac();
Bool Probe_MDA();
Bool Probe_Herc();
Bool Probe_CGA();
Bool Probe_VGA();
/* SVGA */
Bool Probe_Tseng();
Bool Probe_WD();
Bool Probe_CT();
Bool Probe_Video7();
Bool Probe_Genoa();
Bool Probe_Trident();
Bool Probe_Oak();
Bool Probe_Cirrus();
Bool Probe_Ahead();
Bool Probe_ATI();
Bool Probe_S3();
Bool Probe_AL();
Bool Probe_Yamaha();
Bool Probe_NCR();
Bool Probe_AcuMos();
Bool Probe_MX();
Bool Probe_Primus();
Bool Probe_RealTek();
Bool Probe_Compaq();
/* CoProc */
Bool Probe_8514();
Bool Probe_ATIMach();
#endif /* __STDC__ */

/*
 * Print functions
 */
#ifdef __STDC__
void Print_SVGA_Name(int);
void Print_Herc_Name(int);
void Print_RamDac_Name(int);
void Print_CoProc_Name(int);
#else /* __STDC__ */
void Print_SVGA_Name();
void Print_Herc_Name();
void Print_RamDac_Name();
void Print_CoProc_Name();
#endif /* __STDC__ */

#ifndef __STDC__
#define const /**/
#endif

/*
 * Globals
 */
extern char MyName[];
extern Word vgaIOBase;
extern Bool Verbose;
extern Byte Chip_data;
extern Byte *Bios_Base;
extern Bool AssumeEGA;

extern Chip_Descriptor VGA_Descriptor;

extern Chip_Descriptor AL_Descriptor;
extern Chip_Descriptor ATI_Descriptor;
extern Chip_Descriptor AcuMos_Descriptor;
extern Chip_Descriptor Ahead_Descriptor;
extern Chip_Descriptor CT_Descriptor;
extern Chip_Descriptor Cirrus_Descriptor;
extern Chip_Descriptor Compaq_Descriptor;
extern Chip_Descriptor Genoa_Descriptor;
extern Chip_Descriptor MX_Descriptor;
extern Chip_Descriptor NCR_Descriptor;
extern Chip_Descriptor Oak_Descriptor;
extern Chip_Descriptor Primus_Descriptor;
extern Chip_Descriptor RealTek_Descriptor;
extern Chip_Descriptor S3_Descriptor;
extern Chip_Descriptor Trident_Descriptor;
extern Chip_Descriptor Tseng_Descriptor;
extern Chip_Descriptor Video7_Descriptor;
extern Chip_Descriptor WD_Descriptor;
extern Chip_Descriptor Yamaha_Descriptor;

extern Chip_Descriptor IBM8514_Descriptor;
extern Chip_Descriptor ATIMach_Descriptor;

/*
 * Useful macros
 */
/* VGA */
#define COPYRIGHT_BASE	(0x1E)
#define CRTC_IDX	(vgaIOBase+0x04)
#define CRTC_REG	(vgaIOBase+0x05)
/* 8514 */
#define SUBSYS_CNTL	0x42E8
#define ERR_TERM	0x92E8
#define GPCTRL_ENAB	0x4000
#define GPCTRL_RESET	0x8000
#define CHPTEST_NORMAL	0x1000
#define ROM_ADDR_1	0x52EE
#define DESTX_DIASTP	0x8EE8
#define READ_SRC_X	0xDAEE
#define GP_STAT		0x9AE8
#define GPBUSY		0x0200
/* ATIMach */
#define MISC_OPTIONS    0x36EE
#define CONFIG_STATUS_1 0x12EE
#define	M64_SCRATCH_REG0	0x42EC
#define M64_MEM_CNTL		0x52EC

/*
 * RAMDAC Types
 */
#define DAC_STANDARD	0	/* Standard 8-bit pseudo-color DAC */
#define DAC_ALG1101	1	/* Avance Logic ALG1101 */
#define DAC_SS24	2	/* Diamond SS2410 */
#define DAC_SIERRA15	3	/* Sierra 15-bit HiColor */
#define DAC_SIERRA15_16	4	/* Sierra 15/16-bit HiColor */
#define DAC_ACUMOS	5	/* AcuMos ADAC1 15/16/24-bit DAC */
#define DAC_ATI		6	/* ATI 68830/75 15/16/24-bit DAC */
#define DAC_CIRRUSA	7	/* Cirrus 5420 8-bit pseudo-color DAC */
#define DAC_CIRRUSB	8	/* Cirrus 542{2,4,6} 15/16/24-bit DAC */
#define DAC_ATT490	9	/* AT&T 20C490 15/16/24-bit DAC */
#define DAC_ATT491	10	/* AT&T 20C491 15/16/24-bit DAC w/gamma corr */
#define DAC_ATT492	11	/* AT&T 20C492 15/16/18-bit DAC w/gamma corr */
#define DAC_ATT493	12	/* AT&T 20C493 15/16/18-bit DAC */
#define DAC_ATT497	13	/* AT&T 20C497 8-bit PseudoColor,24-bit wide */
#define DAC_BT485	14	/* BrookTree 485 RAMDAC */
#define DAC_BT484	15	/* BrookTree 484 RAMDAC */

#define DAC_6_8_PROGRAM	0x40	/* RAMDAC programmable for 6/8-bit tables */
#define DAC_8BIT	0x80	/* RAMDAC with 8-bit wide lookup tables */

#define DAC_CHIP(x) ((x) & ~(DAC_8BIT|DAC_6_8_PROGRAM))


/*
 * Base chipset classes
 */
#define CHIP_MDA	0	/* Monochrome Display Adapter */
#define CHIP_CGA	1	/* Color Display Adapter */
#define CHIP_EGA	2	/* Enhanced Graphics Adapter */
#define CHIP_PGC	3	/* Professional Graphics Controller */
#define CHIP_VGA	4	/* Video Graphics Array */
#define CHIP_MCGA	5	/* MultiColor Graphics Array */
#define CHIP_COPROC	6	/* Graphics Coprocessor */

/*
 * Hercules cards
 */
#define HERC_TYPE(n)	(CHIP_MDA | ((n) << 8))
#define CHIP_HERC_STD	HERC_TYPE(1)
#define CHIP_HERC_PLUS	HERC_TYPE(2)
#define CHIP_HERC_COL	HERC_TYPE(3)

/*
 * SVGA cards
 */
#define SVGA_TYPE(v,n)	(((v) << 16) | ((n) << 8) | CHIP_VGA)
#define V_ACUMOS	1
#define V_AHEAD		2
#define V_ATI		3
#define V_AL		4
#define V_CT		5
#define V_CIRRUS	6
#define V_COMPAQ	7
#define V_GENOA		8
#define V_MX		9
#define V_NCR		10
#define V_OAK		11
#define V_PRIMUS	12
#define V_REALTEK	13
#define V_S3		14
#define V_TRIDENT	15
#define V_TSENG		16
#define V_VIDEO7	17
#define V_WD		18
#define V_YAMAHA	19

#define NUM_VENDORS	19
#define CHPS_PER_VENDOR	20

#define CHIP_ACUMOS	SVGA_TYPE(V_ACUMOS,0)	/* AcuMos AVGA2 	*/
#define CHIP_AHEAD_UNK	SVGA_TYPE(V_AHEAD,0)	/* Ahead unknown	*/
#define CHIP_AHEAD_A	SVGA_TYPE(V_AHEAD,1)	/* Ahead V5000 Version A*/
#define CHIP_AHEAD_B	SVGA_TYPE(V_AHEAD,2)	/* Ahead V5000 Version B*/
#define CHIP_ATI_UNK	SVGA_TYPE(V_ATI,0)	/* ATI unknown		*/
#define CHIP_ATI18800	SVGA_TYPE(V_ATI,1)	/* ATI 18800 		*/
#define CHIP_ATI18800_1	SVGA_TYPE(V_ATI,2)	/* ATI 18800-1 		*/
#define CHIP_ATI28800_2	SVGA_TYPE(V_ATI,3)	/* ATI 28800-2 		*/
#define CHIP_ATI28800_4	SVGA_TYPE(V_ATI,4)	/* ATI 28800-4		*/
#define CHIP_ATI28800_5	SVGA_TYPE(V_ATI,5)	/* ATI 28800-5		*/
#define CHIP_ATI28800_A	SVGA_TYPE(V_ATI,6)	/* ATI 28800-A		*/
#define CHIP_ATI28800_C	SVGA_TYPE(V_ATI,7)	/* ATI 28800-C (XLR?)	*/
#define CHIP_AL2101	SVGA_TYPE(V_AL,0)	/* Avance Logic 2101	*/
#define CHIP_CT_UNKNOWN	SVGA_TYPE(V_CT,0)	/* C&T unknown		*/
#define CHIP_CT450	SVGA_TYPE(V_CT,1)	/* C&T 82c450		*/
#define CHIP_CT451	SVGA_TYPE(V_CT,2)	/* C&T 82c451		*/
#define CHIP_CT452	SVGA_TYPE(V_CT,3)	/* C&T 82c452		*/
#define CHIP_CT453	SVGA_TYPE(V_CT,4)	/* C&T 82c453		*/
#define CHIP_CT455	SVGA_TYPE(V_CT,5)	/* C&T 82c455		*/
#define CHIP_CT456	SVGA_TYPE(V_CT,6)	/* C&T 82c456		*/
#define CHIP_CT457	SVGA_TYPE(V_CT,7)	/* C&T 82c457		*/
#define CHIP_CTF65510	SVGA_TYPE(V_CT,8)	/* C&T F65510		*/
#define CHIP_CTF65520	SVGA_TYPE(V_CT,9)	/* C&T F65520		*/
#define CHIP_CTF65530	SVGA_TYPE(V_CT,10)	/* C&T F65530		*/
#define CHIP_CL_UNKNOWN	SVGA_TYPE(V_CIRRUS,0)	/* Cirrus unknown	*/
#define CHIP_CL510	SVGA_TYPE(V_CIRRUS,1)	/* Cirrus CL-GD 510/520	*/
#define CHIP_CL610	SVGA_TYPE(V_CIRRUS,2)	/* Cirrus CL-GD 610/620	*/
#define CHIP_CLV7	SVGA_TYPE(V_CIRRUS,3)	/* Cirrus Video 7 OEM	*/
#define CHIP_CL5402	SVGA_TYPE(V_CIRRUS,4)	/* Cirrus 5402		*/
#define CHIP_CL5402R1	SVGA_TYPE(V_CIRRUS,5)	/* Cirrus 5402 rev 1	*/
#define CHIP_CL5420	SVGA_TYPE(V_CIRRUS,6)	/* Cirrus 5420		*/
#define CHIP_CL5420R1	SVGA_TYPE(V_CIRRUS,7)	/* Cirrus 5420 rev 1	*/
#define CHIP_CL5422	SVGA_TYPE(V_CIRRUS,8)	/* Cirrus 5422		*/
#define CHIP_CL5424	SVGA_TYPE(V_CIRRUS,9)	/* Cirrus 5424		*/
#define CHIP_CL5426	SVGA_TYPE(V_CIRRUS,10)	/* Cirrus 5426		*/
#define CHIP_CL5428	SVGA_TYPE(V_CIRRUS,11)	/* Cirrus 5428		*/
#define CHIP_CL6205	SVGA_TYPE(V_CIRRUS,12)	/* Cirrus 6205		*/
#define CHIP_CL6215	SVGA_TYPE(V_CIRRUS,13)	/* Cirrus 6215		*/
#define CHIP_CL6225	SVGA_TYPE(V_CIRRUS,14)	/* Cirrus 6225		*/
#define CHIP_CL6235	SVGA_TYPE(V_CIRRUS,15)	/* Cirrus 6235		*/
#define CHIP_CL6410	SVGA_TYPE(V_CIRRUS,16)	/* Cirrus 6410		*/
#define CHIP_CL6420A	SVGA_TYPE(V_CIRRUS,17)	/* Cirrus 6420A		*/
#define CHIP_CL6420B	SVGA_TYPE(V_CIRRUS,18)	/* Cirrus 6420B		*/
#define CHIP_CL5434	SVGA_TYPE(V_CIRRUS,19)	/* Cirrus 5434		*/
#define CHIP_CPQ_UNK	SVGA_TYPE(V_COMPAQ,0)	/* Compaq unknown	*/
#define CHIP_CPQ_AVGA	SVGA_TYPE(V_COMPAQ,1)	/* Compaq Advanced VGA	*/
#define CHIP_CPQ_Q1024	SVGA_TYPE(V_COMPAQ,2)	/* Compaq QVision/1024	*/
#define CHIP_CPQ_Q1280	SVGA_TYPE(V_COMPAQ,3)	/* Compaq QVision/1280	*/
#define CHIP_G_6100	SVGA_TYPE(V_GENOA,0)	/* Genoa GVGA 6100	*/
#define CHIP_G_6200	SVGA_TYPE(V_GENOA,1)	/* Genoa GVGA 6200	*/
#define CHIP_G_6400	SVGA_TYPE(V_GENOA,2)	/* Genoa GVGA 6400	*/
#define CHIP_MX68010	SVGA_TYPE(V_MX,0)	/* MX 68010		*/
#define CHIP_NCR_UNK	SVGA_TYPE(V_NCR,0)	/* NCR unknown		*/
#define CHIP_NCR77C21	SVGA_TYPE(V_NCR,1)	/* NCR 77C21		*/
#define CHIP_NCR77C22	SVGA_TYPE(V_NCR,2)	/* NCR 77C22		*/
#define CHIP_NCR77C22E	SVGA_TYPE(V_NCR,3)	/* NCR 77C22E		*/
#define CHIP_NCR77C22EP	SVGA_TYPE(V_NCR,4)	/* NCR 77C22E+		*/
#define CHIP_OAK_UNK	SVGA_TYPE(V_OAK,0)	/* OAK unknown		*/
#define CHIP_OAK037C	SVGA_TYPE(V_OAK,1)	/* OAK OTI037C		*/
#define CHIP_OAK057	SVGA_TYPE(V_OAK,2)	/* OAK OTI-057		*/
#define CHIP_OAK067	SVGA_TYPE(V_OAK,3)	/* OAK OTI-067		*/
#define CHIP_OAK077	SVGA_TYPE(V_OAK,4)	/* OAK OTI-077		*/
#define CHIP_P2000	SVGA_TYPE(V_PRIMUS,0)	/* Primus P2000		*/
#define CHIP_REALTEK	SVGA_TYPE(V_REALTEK,0)	/* Realtek RT 3106	*/
#define CHIP_S3_UNKNOWN	SVGA_TYPE(V_S3,0)	/* S3 unknown		*/
#define CHIP_S3_911	SVGA_TYPE(V_S3,1)	/* S3 86c911		*/
#define CHIP_S3_924	SVGA_TYPE(V_S3,2)	/* S3 86c924 or 911A	*/
#define CHIP_S3_801B	SVGA_TYPE(V_S3,3)	/* S3 86c801 A-B step	*/
#define CHIP_S3_801C	SVGA_TYPE(V_S3,4)	/* S3 86c801 C step	*/
#define CHIP_S3_801D	SVGA_TYPE(V_S3,5)	/* S3 86c801 D step	*/
#define CHIP_S3_805B	SVGA_TYPE(V_S3,6)	/* S3 86c805 A-B step	*/
#define CHIP_S3_805C	SVGA_TYPE(V_S3,7)	/* S3 86c805 C step	*/
#define CHIP_S3_805D	SVGA_TYPE(V_S3,8)	/* S3 86c805 D step	*/
#define CHIP_S3_928D	SVGA_TYPE(V_S3,9)	/* S3 86c928 A-D step	*/
#define CHIP_S3_928E	SVGA_TYPE(V_S3,10)	/* S3 86c928 E-step	*/
#define CHIP_S3_928P	SVGA_TYPE(V_S3,11)	/* S3 86c928PCI		*/
#define CHIP_S3_928G    SVGA_TYPE(V_S3,12)      /* S3 86c928 G-step     */
#define CHIP_S3_864     SVGA_TYPE(V_S3,13)      /* S3 86c864 */
#define CHIP_S3_964     SVGA_TYPE(V_S3,14)      /* S3 86c964 */
#define CHIP_TVGA_UNK	SVGA_TYPE(V_TRIDENT,0)	/* Trident unknown	*/
#define CHIP_TVGA8800BR	SVGA_TYPE(V_TRIDENT,1)	/* Trident 8800BR	*/
#define CHIP_TVGA8800CS	SVGA_TYPE(V_TRIDENT,2)	/* Trident 8800CS	*/
#define CHIP_TVGA8900B	SVGA_TYPE(V_TRIDENT,3)	/* Trident 8900B	*/
#define CHIP_TVGA8900C	SVGA_TYPE(V_TRIDENT,4)	/* Trident 8900C	*/
#define CHIP_TVGA8900CL	SVGA_TYPE(V_TRIDENT,5)	/* Trident 8900CL	*/
#define CHIP_TVGA9000	SVGA_TYPE(V_TRIDENT,6)	/* Trident 9000		*/
#define CHIP_TVGA9100	SVGA_TYPE(V_TRIDENT,7)	/* Trident LCD9100	*/
#define CHIP_TVGA9200	SVGA_TYPE(V_TRIDENT,8)	/* Trident LX9200	*/
#define CHIP_ET3000	SVGA_TYPE(V_TSENG,0)	/* Tseng ET3000		*/
#define CHIP_ET4000	SVGA_TYPE(V_TSENG,1)	/* Tseng ET4000		*/
#define CHIP_ET4000_W32	SVGA_TYPE(V_TSENG,2)	/* Tseng ET4000/W32	*/
#define CHIP_ET4000_W32i       SVGA_TYPE(V_TSENG,3)/* Tseng ET4000/W32i*/
#define CHIP_ET4000_W32i_REVB  SVGA_TYPE(V_TSENG,4)/* Tseng ET4000/W32i RevB*/
#define CHIP_ET4000_W32p_REVA  SVGA_TYPE(V_TSENG,5)/* Tseng ET4000/W32p RevA*/
#define CHIP_ET4000_W32p_REVB  SVGA_TYPE(V_TSENG,6)/* Tseng ET4000/W32p RevB*/
#define CHIP_ET4000_W32p_REVC SVGA_TYPE(V_TSENG,7)/* Tseng ET4000/W32p RevC*/
#define CHIP_V7_UNKNOWN	SVGA_TYPE(V_VIDEO7,0)	/* Video7 unknown	*/
#define CHIP_V7_VEGA	SVGA_TYPE(V_VIDEO7,1)	/* Video7 VEGA		*/
#define CHIP_V7_FWRITE	SVGA_TYPE(V_VIDEO7,2)	/* Video7 Fastwrite/VRAM*/
#define CHIP_V7_VRAM2	SVGA_TYPE(V_VIDEO7,3)	/* Video7 VRAM II	*/
#define CHIP_V7_1024i	SVGA_TYPE(V_VIDEO7,4)	/* Video7 1024i		*/
#define CHIP_WD_PVGA1	SVGA_TYPE(V_WD,0)	/* WD PVGA1		*/
#define CHIP_WD_90C00	SVGA_TYPE(V_WD,1)	/* WD 90C00		*/
#define CHIP_WD_90C10	SVGA_TYPE(V_WD,2)	/* WD 90C10		*/
#define CHIP_WD_90C11	SVGA_TYPE(V_WD,3)	/* WD 90C11		*/
#define CHIP_WD_90C20	SVGA_TYPE(V_WD,4)	/* WD 90C20		*/
#define CHIP_WD_90C20A	SVGA_TYPE(V_WD,5)	/* WD 90C20A		*/
#define CHIP_WD_90C22	SVGA_TYPE(V_WD,6)	/* WD 90C22		*/
#define CHIP_WD_90C24	SVGA_TYPE(V_WD,7)	/* WD 90C24		*/
#define CHIP_WD_90C26	SVGA_TYPE(V_WD,8)	/* WD 90C26		*/
#define CHIP_WD_90C30	SVGA_TYPE(V_WD,9)	/* WD 90C30		*/
#define CHIP_WD_90C31	SVGA_TYPE(V_WD,10)	/* WD 90C31		*/
#define CHIP_YAMAHA6388	SVGA_TYPE(V_YAMAHA,0)	/* Yamaha 6388 VPDC	*/

/*
 * Graphics Coprocessors
 */
#define COPROC_TYPE(c,n)	(((c) << 16) | ((n) << 8) | CHIP_COPROC)
#define C_8514		0
#define C_XGA		1

#define NUM_CP_TYPES	2
#define CHPS_PER_CPTYPE	6

#define CHIP_8514	COPROC_TYPE(C_8514,0)	/* 8514/A or true clone */
#define CHIP_MACH8	COPROC_TYPE(C_8514,1)	/* ATI Mach-8		*/
#define CHIP_MACH32	COPROC_TYPE(C_8514,2)	/* ATI Mach-32		*/
#define CHIP_CT480	COPROC_TYPE(C_8514,3)	/* C&T 82c480		*/
#define CHIP_MACH64	COPROC_TYPE(C_8514,4)	/* ATI Mach-64		*/

#define NUM_COPROC_TYPE	1
#define CHPS_PER_TYPE	6

/*
 * Useful macros
 */
#define IS_MDA(c)	((c) == CHIP_MDA)
#define IS_HERC(c)	((((c) & 0xFF) == CHIP_MDA) && ((c) != CHIP_MDA))
#define HERC_CHIP(c)	(((c) >> 8) & 0xFF)
#define IS_CGA(c)	((c) == CHIP_CGA)
#define IS_EGA(c)	((c) == CHIP_EGA)
#define IS_PGC(c)	((c) == CHIP_PGC)
#define IS_VGA(c)	((c) == CHIP_VGA)
#define IS_SVGA(c)	((((c) & 0xFF) == CHIP_VGA) && ((c) != CHIP_VGA))
#define SVGA_VENDOR(c)	(((c) >> 16) & 0xFF)
#define SVGA_CHIP(c)	(((c) >> 8) & 0xFF)
#define IS_MCGA(c) 	((c) == CHIP_MCGA)
#define IS_COPROC(c)	(((c) & 0xFF) == CHIP_COPROC)
#define COPROC_CLASS(c)	(((c) >> 16) & 0xFF)
#define COPROC_CHIP(c)	(((c) >> 8) & 0xFF)

/*
 * Memory detection Functions
 */
extern int NullEntry ();
extern int CirrusMemory ();
extern int TsengMemory ();
extern int S3Memory ();
extern int ATIMachMemory();
extern int TridentMemory ();
extern int WDMemory ();

